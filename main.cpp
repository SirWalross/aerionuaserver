#define NOMINMAX

#include <open62541/plugin/accesscontrol.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <open62541/util.h>

#include <chrono>
#include <cmath>
#include <csignal>
#include <filesystem>
#include <future>
#include <memory>
#include <thread>

#include "efsw/efsw.hpp"
#include "fmt/format.h"
#include "loguru/loguru.hpp"
#include "reproc++/reproc.hpp"
#include "tray/tray.h"
#include "zmq.hpp"

void log(void* logContext, UA_LogLevel level, UA_LogCategory category, const char* msg, va_list args) {
    static std::mutex mutex;
    std::scoped_lock<std::mutex> guard(mutex);

    // for now just ignore log_category
    static char logbuf[500];
    const int log_level = -level + 2;  // convert from UA_LogLevel to loglevel used by loguru
    auto pos = vsnprintf(logbuf, sizeof(logbuf), msg, args);
    if (pos < 0) {
        LOG_F(ERROR, "Log message too long to log");
        return;
    }
    VLOG_F(log_level, logbuf);
}

static volatile UA_Boolean running = true;
static volatile bool restart_clients = false;
static UA_Logger file_logger{log, nullptr, nullptr};

#ifdef WIN32
#include "ShlObj.h"
#endif

#include "plc.h"
#include "robot.h"
#include "wrapper.h"

std::unique_ptr<reproc::process> gui_process;
zmq::context_t context{1};
zmq::socket_t gui_socket{context, zmq::socket_type::req};

static void stopHandler(int sign) {
    UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;  // shutdown server
}

UA_StatusCode run_server(UA_Server* server) {
    const UA_StatusCode retval = UA_Server_run(server, &running);

    UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_SERVER, "Server shutdown with retval %u", retval);
    running = false;

    UA_Server_delete(server);
    return retval;
}

void send_msg_to_gui(std::string message, bool blocking = false) {
    static std::mutex mutex;
    static zmq::message_t request;
    if (gui_process) {
        std::scoped_lock<std::mutex> guard(mutex);
        auto send_result =
            gui_socket.send(zmq::buffer(message), blocking ? zmq::send_flags::none : zmq::send_flags::dontwait);
        if (!send_result.has_value()) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Send error");
            return;
        }
        auto recv_result = gui_socket.recv(request, zmq::recv_flags::none);
        if (!recv_result.has_value()) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Recv error");
            return;
        }
    }
}

void send_update_to_gui(std::string name, bool device_running, bool blocking = false) {
    send_msg_to_gui(fmt::format("{{\"device_update\": {{\"{}\": {}}}}}", name, device_running), blocking);
}

class Clients : public efsw::FileWatchListener {
   public:
    Clients(UA_Server* server, std::string client_file, std::string client_file_directory)
        : client_file{std::move(client_file)},
          client_file_directory{std::move(client_file_directory)},
          server{server},
          filewatcher{},
          watchid{-1},
          clients{},
          threads{},
          last_change_event{} {}

    virtual ~Clients() {
        if (watchid != -1) {
            filewatcher.removeWatch(watchid);
        }
        threads.resize(0);
        UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Shutdown client threads");
    }

    void run() {
        watchid = filewatcher.addWatch(client_file_directory, this, false);
        filewatcher.watch();
        run_clients();
    }

    void handleFileAction(efsw::WatchID _, const std::string& dir, const std::string& filename, efsw::Action action,
                          std::string oldFilename) override {
        using namespace std::literals;
        if (action == efsw::Actions::Modified && filename == "clients.json" &&
            (std::chrono::system_clock::now() - last_change_event) > 10ms) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Detected change in file %s", filename.c_str());
            run_clients();
            last_change_event = std::chrono::system_clock::now();
        } else if (action == efsw::Actions::Modified && filename == "clients.json") {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Change in file %s ignored", filename.c_str());
        }
    }

   private:
    void run_clients() {
        try {
            std::scoped_lock<std::mutex> guard(change_event_mutex);
            send_msg_to_gui("{\"clear_devices\": {}}", false);
            restart_clients = true;
            if (threads.size() > 0) {
                UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Restarting devices");
                for (const auto& thread : threads) {
                    thread.wait();
                }
                UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Threads finished");
            }
            threads.resize(0);
            clients.resize(0);
            restart_clients = false;

            std::ifstream file(client_file);

            nlohmann::json client_nodes = nlohmann::json::parse(file, nullptr, true, true);

            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Found %zu client(s)", client_nodes["Clients"].size());

            clients.reserve(client_nodes["Clients"].size());
            threads.reserve(client_nodes["Clients"].size());

            for (const auto& client_node : client_nodes["Clients"]) {
                if (client_node["Type"] == "Robot") {
                    clients.push_back(std::make_unique<Robot>(client_node["Name"].get<std::string>(),
                                                              client_node["Ip"].get<std::string>(),
                                                              client_node["Port"].get<int>()));
                    threads.push_back(std::async(std::launch::async, &Clients::run_robot, this,
                                                 dynamic_cast<Robot*>(clients.back().get())));
                    UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Created robot %s (%s:%d)",
                                client_node["Name"].get<std::string>().data(),
                                client_node["Ip"].get<std::string>().data(), client_node["Port"].get<int>());
                } else if (client_node["Type"] == "PLC") {
                    clients.push_back(std::make_unique<PLC>(
                        client_node["Name"].get<std::string>(), client_node["Ip"].get<std::string>(),
                        client_node["Port"].get<int>(), client_node["Destination network No."].get<uint8_t>(),
                        client_node["Destination station No."].get<uint8_t>(),
                        client_node["Destination Module I/O"].get<uint16_t>(),
                        client_node["Destination multidrop station No."].get<uint8_t>()));
                    threads.push_back(std::async(std::launch::async, &Clients::run_plc, this,
                                                 dynamic_cast<PLC*>(clients.back().get())));
                    UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Created plc %s (%s:%d)",
                                client_node["Name"].get<std::string>().data(),
                                client_node["Ip"].get<std::string>().data(), client_node["Port"].get<int>());
                } else {
                    UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Invalid device type %s of device %s",
                                client_node["Type"].get<std::string>().data(),
                                client_node["Name"].get<std::string>().data());
                }
            }
        } catch (std::exception& e) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "run_clients failed with: %s\n", e.what());
            throw e;
        }
    }

    void run_robot(Robot* robot) {
        try {
            while (running && !restart_clients) {
                robot->r3.connect();
                if (robot->r3.connected) {
                    std::cout << "Hallo from " << robot->name << "\n";
                    create_robot_node(robot, server, client_file.c_str());

                    send_update_to_gui(robot->name, true);

                    while (robot->r3.connected && running && !restart_clients) {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    if (running) {
                        delete_node(server, robot->node.node, true);
                    }
                }

                send_update_to_gui(robot->name, false);

                if (running && !restart_clients) {
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
            }
        } catch (const std::exception& e) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_CLIENT, "running device '%s' threw exception %s",
                        robot->name.c_str(), e.what());
        } catch (const std::string& e) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_CLIENT, "running device '%s' threw exception %s",
                        robot->name.c_str(), e.c_str());
        } catch (...) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_CLIENT, "running device '%s' threw exception",
                        robot->name.c_str());
        }
    }

    void run_plc(PLC* plc) {
        try {
            while (running && !restart_clients) {
                plc->slmp.connect();
                if (plc->slmp.connected) {
                    create_plc_node(plc, server, client_file.c_str());

                    send_update_to_gui(plc->name, true);

                    while (plc->slmp.connected && running && !restart_clients) {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    if (running) {
                        delete_node(server, plc->node.node, true);
                    }
                }

                send_update_to_gui(plc->name, false);

                if (running && !restart_clients) {
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
            }
        } catch (const std::exception& e) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_CLIENT, "running device '%s' threw exception %s",
                        plc->name.c_str(), e.what());
        } catch (const std::string& e) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_CLIENT, "running device '%s' threw exception %s",
                        plc->name.c_str(), e.c_str());
        } catch (...) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_CLIENT, "running device '%s' threw exception", plc->name.c_str());
        }
    }
    std::string client_file;
    std::string client_file_directory;
    UA_Server* server;
    efsw::FileWatcher filewatcher;
    efsw::WatchID watchid;
    std::vector<std::unique_ptr<Client>> clients;
    std::vector<std::future<void>> threads;
    std::mutex change_event_mutex;
    std::chrono::time_point<std::chrono::system_clock> last_change_event;

    friend class Tray;
};

class Tray {
   public:
    Tray(const Clients* clients, std::string server_address)
        : clients{clients},
          server_address{std::move(server_address)},
          tooltip{"aerionuaserver"},
          tray_menu_items{
              {"aerionuaserver", 0, 0, 0, start_gui, this}, {"-"}, {"Exit", 0, 0, 0, quit, this}, {nullptr}} {}

    void run() {
        retval = std::async(std::launch::async, &Tray::run_sync, this);
    }

   private:
    void run_sync() {
        if (tray_init(&tray) < 0) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Failed to create tray");
            return;
        }
        while (tray_loop(1) == 0)
            ;
    }

    static void quit(struct tray_menu* item) {
        UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Pressed quit");
        running = false;
        tray_exit();

        // shutdown gui process
        if (gui_process) {
            reproc::stop_actions stop = {{reproc::stop::noop, reproc::milliseconds(0)},
                                         {reproc::stop::terminate, reproc::milliseconds(5000)},
                                         {reproc::stop::kill, reproc::milliseconds(2000)}};
            auto [status, ec] = gui_process->stop(stop);

            if (ec) {
                UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "gui process failed to cleanly shutdown (%s)",
                            ec.message().data());
            } else {
                UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "shutdown gui process");
            }
            gui_process = nullptr;
        }
    }

    static void start_gui(struct tray_menu* item) {
        using namespace std::chrono_literals;

        UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Opening gui");

        if (gui_process) {
            // check if process is still running
            auto [code, ec] = gui_process->poll(reproc::event::exit, 10ms);

            if (code == 0 && !ec) {
                UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "gui process is still running");
                return;
            } else {
                // cleanly shutdown process
                UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "gui process is not running anymore");
                reproc::stop_actions stop = {{reproc::stop::noop, reproc::milliseconds(0)},
                                             {reproc::stop::terminate, reproc::milliseconds(5000)},
                                             {reproc::stop::kill, reproc::milliseconds(2000)}};
                auto [status, ec] = gui_process->stop(stop);

                if (ec) {
                    UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "gui process failed to cleanly shutdown (%s)",
                                ec.message().data());
                }
            }
        }
        gui_process = std::make_unique<reproc::process>();

        std::array<std::string, 1> args{"aerionuaserver-gui"};
        reproc::options options;
        options.redirect.out.type = reproc::redirect::parent;  // rerout stdout of gui to stdout of server
        auto ec = gui_process->start(args, options);

        if (ec) {
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "gui process did not start up correctly (%s)",
                        ec.message().data());
            return;
        } else {
            // send updated data
            send_msg_to_gui("{\"clear_devices\": {}}", true);
            for (const auto& client : static_cast<Tray*>(item->context)->clients->clients) {
                send_update_to_gui(client->name, client->connected(), true);
            }
            send_msg_to_gui(fmt::format("{{\"server_update\": {{\"running\": {}, \"address\": \"{}\"}}}}", running,
                                        static_cast<Tray*>(item->context)->server_address),
                            true);
        }
    }

    const Clients* clients;
    const std::string server_address;
    std::future<void> retval;

    char tooltip[17];

    struct tray_menu tray_menu_items[4];

    struct tray tray = {
#if defined(_WIN32) || defined(_WIN64)
        "icon.ico",
#elif defined(__linux__) || defined(linux) || defined(__linux)
        "icon.png",
#endif
        tooltip, tray_menu_items};
};

int main(int argc, char** argv) {
    try {
        signal(SIGINT, stopHandler);
        signal(SIGTERM, stopHandler);

        // init logger
        loguru::init(argc, argv);

#if defined(WIN32) && !defined(TEST)
        PWSTR path;
        std::string client_file_directory{};
        if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path) == S_OK) {
            std::filesystem::current_path(std::filesystem::path(path) / std::filesystem::path("Aerion OPC-UA Server"));
            client_file_directory =
                (std::filesystem::path(path) / std::filesystem::path("Aerion OPC-UA Server")).string();
        } else {
            throw "roaming folder does not exist";
        }
        loguru::add_file("server.log", loguru::Append, loguru::Verbosity_MAX);
        CoTaskMemFree(path);

        std::string client_file =
            (std::filesystem::path(client_file_directory) / std::filesystem::path("clients.json")).string();
#elif !defined(WIN32) && !defined(TEST)
        auto config_dir = std::filesystem::path(std::getenv("HOME")) / std::filesystem::path(".aerionuaserver");
        std::filesystem::create_directory(config_dir);
        std::filesystem::current_path(config_dir);
        loguru::add_file("server.log", loguru::Append, loguru::Verbosity_MAX);

        std::string client_file_directory = config_dir.string();
        std::string client_file = (config_dir / std::filesystem::path("clients.json")).string();
#elif defined(TEST)
        loguru::add_file("server.log", loguru::Append, loguru::Verbosity_MAX);
        std::string client_file =
            (std::filesystem::current_path() / std::filesystem::path("tests") / std::filesystem::path("clients.json"))
                .string();
        std::string client_file_directory = (std::filesystem::current_path() / std::filesystem::path("tests")).string();
        std::filesystem::current_path("tests");
#endif
        auto trusted_client_files_dir = std::filesystem::path("server") / std::filesystem::path("trusted");

        // hostname
        char hostname[64];  // dns spec limits hostname to 63 chars
        UA_gethostname(hostname, sizeof(hostname));

        if (!std::filesystem::exists("clients.json")) {
            // create file clients.json
            std::ofstream ofs("clients.json");
            ofs << "{\"Clients\": []}";
            ofs.close();
        }
        if (!std::filesystem::exists("server.json")) {
            // create file server.json
            std::ofstream ofs("server.json");
            ofs << "{}";
            ofs.close();
        }

        if (!std::filesystem::exists("server") || !std::filesystem::exists(trusted_client_files_dir)) {
            // create folder server
            std::filesystem::create_directories(trusted_client_files_dir);
        }
        auto certificate_file = std::filesystem::path("server") / std::filesystem::path("server.crt.der");
        auto private_key_file = std::filesystem::path("server") / std::filesystem::path("server.key.der");

        // load/generate certificates
        UA_ByteString certificate = UA_BYTESTRING_NULL;
        UA_ByteString privateKey = UA_BYTESTRING_NULL;

        if (std::filesystem::exists(certificate_file) && std::filesystem::exists(private_key_file)) {
            // certificates exists
            certificate = loadFile(certificate_file.string().c_str());
            privateKey = loadFile(private_key_file.string().c_str());
        } else {
            // generate certificate
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Generating new certificate");
            UA_String subject[3] = {UA_STRING_STATIC("C=DE"), UA_STRING_STATIC("O=aerionuaserver"),
                                    UA_STRING_ALLOC(fmt::format("CN=Open62541Server@{}", hostname).data())};
            UA_UInt32 lenSubject = 3;
            UA_String subjectAltName[2] = {UA_STRING_ALLOC(fmt::format("DNS:{}", hostname).data()),
                                           UA_STRING_STATIC("URI:urn:open62541.server.application")};
            UA_UInt32 lenSubjectAltName = 2;
            UA_StatusCode statusCertGen =
                UA_CreateCertificate(UA_Log_Stdout, subject, lenSubject, subjectAltName, lenSubjectAltName, 0,
                                     UA_CERTIFICATEFORMAT_DER, &privateKey, &certificate);

            if (statusCertGen != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Generating Certificate failed: %s",
                            UA_StatusCode_name(statusCertGen));
                return EXIT_FAILURE;
            } else {
                writeFile(certificate_file.string().c_str(), certificate);
                writeFile(private_key_file.string().c_str(), privateKey);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Written certificates to server folder");
            }
        }

        // zmq socket
        gui_socket.set(zmq::sockopt::rcvtimeo, 500);
        gui_socket.bind("tcp://*:5555");

        UA_Server* server = UA_Server_new();
        UA_ServerConfig* config = UA_Server_getConfig(server);

        std::ifstream server_config_file("server.json");

        nlohmann::json server_config = nlohmann::json::parse(server_config_file, nullptr, true, true);

        const auto port = server_config.contains("Port") ? server_config["Port"].get<int>() : 4840;

        CHECK(UA_ServerConfig_setBasics(config), "Setting basic config failed");

        // add network layer
        CHECK(UA_ServerConfig_addNetworkLayerTCP(config, port, 0U, 0U), "Adding network layer failed");

        CHECK(UA_ServerConfig_addSecurityPolicyNone(config, &certificate), "Adding security policy None failed");

        auto contained_none = !server_config.contains("SecurityPolicies");
        if (server_config.contains("SecurityPolicies")) {
            for (const auto& policy : server_config["SecurityPolicies"]) {
                const auto policy_name = policy.get<std::string>();
                if (policy_name == "Basic128Rsa15") {
                    CHECK(UA_ServerConfig_addSecurityPolicyBasic128Rsa15(config, &certificate, &privateKey),
                          "Adding security policy Basic128Rsa15 failed");
                } else if (policy_name == "Basic256") {
                    CHECK(UA_ServerConfig_addSecurityPolicyBasic256(config, &certificate, &privateKey),
                          "Adding security policy Basic256 failed");
                } else if (policy_name == "Basic256Sha256") {
                    CHECK(UA_ServerConfig_addSecurityPolicyBasic256Sha256(config, &certificate, &privateKey),
                          "Adding security policy Basic256Sha256 failed");
                } else if (policy_name == "Aes128Sha256RsaOaep") {
                    CHECK(UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(config, &certificate, &privateKey),
                          "Adding security policy Aes128Sha256RsaOaep failed");
                } else if (policy_name == "None") {
                    contained_none = true;
                }
            }
        }

        if (!contained_none) {
            // use none policy only for discovery
            config->securityPolicyNoneDiscoveryOnly = true;
        }

        // server access control
        CHECK(
            UA_AccessControl_default(config, true, NULL,
                                     &config->securityPolicies[config->securityPoliciesSize - 1].policyUri, 0, nullptr),
            "Setting access control failed");

        // add endpoints
        CHECK(addAllEndpoints(config, server_config, contained_none), "Adding endpoints failed");

        // client certificate verification, if not EnforceClientCertificateVerification, all client certificates are
        // allowed
        if (server_config.contains("EnforceClientCertificateVerification") &&
            server_config["EnforceClientCertificateVerification"].get<bool>()) {
            std::vector<UA_ByteString> certificate_trustlist;
            certificate_trustlist.reserve(10);

            for (auto const& client_certificate : std::filesystem::directory_iterator(trusted_client_files_dir)) {
                certificate_trustlist.push_back(loadFile(client_certificate.path().string().c_str()));
            }

            CHECK(UA_CertificateVerification_Trustlist(&config->certificateVerification, certificate_trustlist.data(),
                                                       certificate_trustlist.size(), nullptr, 0, nullptr, 0),
                  "Adding certificate Verification failed");
        }

        config->logger = file_logger;  // set file logger

#if !defined(WIN32) && !defined(TEST)
        // for linux, specification files are in /etc/aerionuaserver/specifications
        std::filesystem::current_path("/etc/aerionuaserver");
#endif

#if defined(TEST)
        // for tests, specification files are in root directory
        std::filesystem::current_path("..");
        std::cout << std::filesystem::current_path() << "\n";
#endif

        auto retval = std::async(std::launch::async, run_server, server);

        Clients clients(server, client_file, client_file_directory);

        // start tray
        Tray tray(&clients, fmt::format("opc.tcp://{}:{}/", hostname, port));

        tray.run();

        clients.run();

        retval.wait();

        if (gui_process) {
            // cleanly shutdown process
            reproc::stop_actions stop = {{reproc::stop::noop, reproc::milliseconds(0)},
                                         {reproc::stop::terminate, reproc::milliseconds(5000)},
                                         {reproc::stop::kill, reproc::milliseconds(2000)}};
            auto [status, ec] = gui_process->stop(stop);

            if (ec) {
                UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "gui process failed to cleanly shutdown (%s)",
                            ec.message().data());
            } else {
                UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "shutdown gui process");
            }
            gui_process = nullptr;
        }

        std::cout << "shutdown\n";
        return retval.get() == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
    }
}
