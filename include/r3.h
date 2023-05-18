#pragma once

#include <fmt/format.h>
#include <open62541/plugin/log_stdout.h>
#include <re2/re2.h>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <loguru/loguru.hpp>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "socket.h"

#ifdef _MSC_VER
#include <intrin.h>
#define __builtin_popcount __popcnt
#endif

class R3 {
   public:
    struct Command {
        std::string command;
        std::string match;
        int mecha_no;
        int task_slot_no;
        int position;
        int id;
        std::vector<std::tuple<std::string, std::string, int64_t>> cases;

        Command(std::string command, std::string match, int mecha_no, int task_slot_no, int id)
            : command(std::move(command)),
              match(std::move(match)),
              mecha_no(mecha_no),
              task_slot_no(task_slot_no),
              position{0},
              id{id} {}

        Command(std::string command, std::string match, int mecha_no, int task_slot_no)
            : Command(std::move(command), std::move(match), 1, 1, 0) {}

        Command(std::string command, std::string match) : Command(std::move(command), std::move(match), 1, 1) {}

        explicit Command(std::string command) : Command(std::move(command), "") {}
    };

    explicit R3(std::string addr, int port = 10001) noexcept
        : ip_addr{std::move(addr)}, socket(ip_addr.data(), port), buffer(new char[size]) {
        memset(buffer, 0, size * sizeof(char));
    }

    void connect() {
        const auto result = socket.connect();
        if (!result.has_value()) {
#ifdef WIN32
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Couldn't connect to robot at address '%s:%d' %d",
                        socket.addr, socket.port, WSAGetLastError());
#else
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Couldn't connect to robot at address '%s:%d'",
                        socket.addr, socket.port);
#endif
            this->disconnect();
            return;
        }
        UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Connected with robot at address '%s:%d'", socket.addr,
                    socket.port);
        connected = true;
    }

    void disconnect() {
        UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Disconnected");
        connected = false;
    }

    template <typename Type>
    Type get_hex(const std::string& read_command, const std::string& match);

    std::string get(const std::string& read_command);

    template <typename Type>
    Type get(const std::string& read_command, const std::string& match);

    template <typename Type>
    Type get(const std::string& read_command, const std::string& match, int position);

    ~R3() {
        delete[] buffer;
    }

    volatile bool connected = false;

    bool execute(std::string command) {
        return get_answer(command).has_value();
    }

    void get_position(const std::string& read_command, const std::string& match, double* array, std::size_t array_size);

   private:
    std::string ip_addr;
    Socket socket;
    char* buffer;
    static constexpr ::std::size_t size = 400;

    std::optional<const char*> get_answer(const std::string& command) {
        const std::lock_guard<std::mutex> lock(this->mutex);
        const auto send_result = socket.send(command.data(), command.size());
        if (!send_result.has_value()) {
            this->disconnect();
            return {};
        }
        const auto recv_result = socket.recv(static_cast<void*>(buffer), size);
        if (!recv_result.has_value()) {
            this->disconnect();
            return {};
        }
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "'%s' -> '%s'\n", command.data(), buffer);

        if (strncmp(buffer, "QoK", 3) != 0 && strncmp(buffer, "Qok", 3) != 0) {
            return {};
        }
        return buffer + 3;
    }
    std::mutex mutex;
};

template <>
inline std::string R3::get<std::string>(const std::string& read_command, const std::string& match) {
    std::string value;
    const auto answer = get_answer(read_command);
    if (answer.has_value()) {
        ERROR_CONTEXT("PartialMatch", static_cast<const char*>(answer.value()));
        ERROR_CONTEXT("\tMatch", match.c_str());
        assert(RE2::PartialMatch(answer.value(), match, &value) && "partial match failed");
    }
    return value;
}

inline std::string R3::get(const std::string& read_command) {
    return get_answer(read_command).value_or("");
}

template <typename T>
inline T R3::get_hex(const std::string& read_command, const std::string& match) {
    const auto answer = get_answer(read_command);
    T value = 0;
    if (answer.has_value()) {
        ERROR_CONTEXT("PartialMatch", static_cast<const char*>(answer.value()));
        ERROR_CONTEXT("\tMatch", match.c_str());
        assert(RE2::PartialMatch(answer.value(), match, RE2::Hex(&value)) && "partial match failed");
    }
    return value;
}

template <>
inline bool R3::get<bool>(const std::string& read_command, const std::string& match, int position) {
    if (match.empty()) {
        // if match is empty just check if any answer is returned at all
        const auto answer = get_answer(read_command);
        return answer.has_value() && answer.value()[0] != 0;
    } else {
        const auto value = this->get_hex<int32_t>(read_command, match);
        return (value & (1 << position)) != 0;
    }
}

template <>
inline double R3::get<double>(const std::string& read_command, const std::string& match) {
    const auto value = this->get<std::string>(read_command, match);
    return value.empty() ? 0.0 : std::stod(value, nullptr);
}

template <>
inline float R3::get<float>(const std::string& read_command, const std::string& match) {
    const auto value = this->get<std::string>(read_command, match);
    return value.empty() ? 0.0F : std::stof(value, nullptr);
}

template <typename Type>
Type R3::get(const std::string& read_command, const std::string& match) {
    const auto answer = get_answer(read_command);
    Type value = 0;
    if (answer.has_value()) {
        ERROR_CONTEXT("PartialMatch", static_cast<const char*>(answer.value()));
        ERROR_CONTEXT("\tMatch", match.c_str());
        assert(RE2::PartialMatch(answer.value(), match, &value) && "partial match failed");
    }
    return value;
}

inline void R3::get_position(const std::string& read_command, const std::string& match, double* array,
                             std::size_t array_size) {
    const auto answer = this->get<std::string>(read_command, match);
    const char* start = answer.c_str() + 1;
    char* end{};
    std::size_t index = 0;
    while (start < (answer.c_str() + answer.length())) {
        array[index] = std::strtod(start, &end);
        start = end + 1;
        if (*end == ',') {
            index++;
        } else if (*end == ')') {
            // always place '...)(fl1, fl2) if present at the end of array
            index = array_size - 2;
            start++;
        }
        if (index >= array_size) {
            return;
        }
    }
}