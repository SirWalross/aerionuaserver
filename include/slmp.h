#pragma once

#include <fmt/format.h>
#include <open62541/plugin/log_stdout.h>
#include <re2/re2.h>

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <string>
#include <tcb/span.hpp>
#include <type_traits>
#include <vector>

#include "socket.h"

#define SLMP_SHIFT_UINT8_T(a) static_cast<std::byte>(a)
#define SLMP_SHIFT_UINT16_T(a) static_cast<std::byte>(a), static_cast<std::byte>((static_cast<uint16_t>(a) >> 8) & 0xff)

class SLMP {
   public:
    enum class RequestCommand : uint16_t {
        Read = 0x0401,
        Write = 0x1401,
        ArrayLabelRead = 0x041A,
        ArrayLabelWrite = 0x141A,
        RandomLabelRead = 0x041c,
        RandomLabelWrite = 0x141b
    };

    enum class DeviceExtension : uint16_t {
        None = 0x0100,
        CPUNo1 = 0x03E0,
        CPUNo2 = 0x03E1,
        CPUNo3 = 0x03E2,
        CPUNo4 = 0x03E3,
        // or manually specify the start io number 00-FF
    };

    enum class ResponseCommand : uint8_t { Read = 0x01, LabelRead = 0x02, RandomLabelRead = 0x03 };

    enum class Subcommand : uint16_t {
        Bit = 0x0001,       // 3 bytes for head_device no, 1 byte for device
        BitLong = 0x0003,   // 4 bytes for head_device no, 2 bytes for device
        Word = 0x0000,      // 3 bytes for head_device no, 1 byte for device
        WordLong = 0x0002,  // 4 bytes for head_device no, 2 bytes for device
        WordLongDeviceExtension = 0x0082
    };

    enum class WriteType : uint16_t { Bit, Word };

    enum class Device : uint16_t {
        None = 0x00,
        SM = 0x91,
        SD = 0xA9,
        X = 0x9C,
        Y = 0x9D,
        M = 0x90,
        L = 0x92,
        F = 0x93,
        V = 0x94,
        B = 0xA0,
        D = 0xA8,
        W = 0xB4,
        TS = 0xC1,
        TC = 0xC0,
        TN = 0xC2,
        SB = 0xA1,
        SW = 0xB5,
        DX = 0xA2,
        DY = 0xA3,
        Z = 0xCC,
        R = 0xAF,
        ZR = 0xB0,
        G = 0x00AB,
        HG = 0x002E
    };

    enum class Serialnumber : uint16_t {
        None = 0x0050,
    };

    enum class NetworkNumber : uint8_t { A = 0x00 };

    enum class StationNumber : uint8_t { A = 0xFF };

    enum class DestinationModule : uint16_t {
        CPU = 0x03FF,
        CPU_Multi_1 = 0x03E0,
        CPU_Multi_2 = 0x03E1,
        CPU_Multi_3 = 0x03E2,
        CPU_Multi_4 = 0x03E3,
    };

    enum class DestinationStation : uint8_t { A = 0x00 };

    enum class MonitoringTimer : uint16_t {
        None = 0x0000,
    };

    enum class Endcode : uint16_t {
        Success = 0x0000,
        InvalidEndCode = 0x0001,
        UnableToWrite = 0x0055,
        WrongCommand = 0xC059,
        WrongFormat = 0xC05C,
        WrongLength = 0xC061,
        Busy = 0xCEE0,
        ExceedReqLength = 0xCEE1,
        ExceedRespLength = 0xCEE2,
        ServerNotFound = 0xCF10,
        WrongConfigItem = 0xCF20,
        PrmIDNotFound = 0xCF30,
        NotStartExclusiveWrite = 0xCF31,
        RelayFailure = 0xCF70,
        TimeoutError = 0xCF71,
        CANAppNotPermittedRead = 0xCCC7,
        CANAppWriteOnly = 0xCCC8,
        CANAppReadOnly = 0xCCC9,
        CANAppUndefinedObjectAccess = 0xCCCA,
        CANAppNotPermittedPDOMapping = 0xCCCB,
        CANAppExceedPDOMapping = 0xCCCC,
        CANAppNotExistSubIndex = 0xCCD3,
        CANAppWrongParameter = 0xCCD4,
        CANAppMoreOverParameterRange = 0xCCD5,
        CANAppLessOverParameterRange = 0xCCD6,
        CANAppTransOrStoreError = 0xCCDA,
        CANAppOtherError = 0xCCFF,
        OtherNetworkError = 0xCF00,
        DataFragmentShortage = 0xCF40,
        DataFragmentDup = 0xCF41,
        DataFragmentLost = 0xCF43,
        DataFragmentNotSupport = 0xCF44,
        InvalidGlobalLabel = 0x40C0
    };

    struct Command {
        bool is_label;
        Device device;
        DeviceExtension device_extension;
        std::string label;
        uint32_t head_no;
        uint16_t length;

        Command(std::string label)
            : is_label{true},
              device{Device::None},
              device_extension{DeviceExtension::None},
              label{std::move(label)},
              head_no{},
              length{} {}

        Command(Device device, DeviceExtension device_extension, uint32_t head_no, uint16_t length)
            : is_label{false},
              device{device},
              device_extension{device_extension},
              label{},
              head_no{head_no},
              length{length} {}

        static std::pair<Device, DeviceExtension> convert_device_name(std::string device_name) {
            if (device_name == "SM")
                return {Device::SM, DeviceExtension::None};
            if (device_name == "SD")
                return {Device::SD, DeviceExtension::None};
            if (device_name == "X")
                return {Device::X, DeviceExtension::None};
            if (device_name == "Y")
                return {Device::Y, DeviceExtension::None};
            if (device_name == "M")
                return {Device::M, DeviceExtension::None};
            if (device_name == "L")
                return {Device::L, DeviceExtension::None};
            if (device_name == "F")
                return {Device::F, DeviceExtension::None};
            if (device_name == "V")
                return {Device::V, DeviceExtension::None};
            if (device_name == "B")
                return {Device::B, DeviceExtension::None};
            if (device_name == "D")
                return {Device::D, DeviceExtension::None};
            if (device_name == "W")
                return {Device::W, DeviceExtension::None};
            if (device_name == "TS")
                return {Device::TS, DeviceExtension::None};
            if (device_name == "TC")
                return {Device::TC, DeviceExtension::None};
            if (device_name == "TN")
                return {Device::TN, DeviceExtension::None};
            if (device_name == "SB")
                return {Device::SB, DeviceExtension::None};
            if (device_name == "SW")
                return {Device::SW, DeviceExtension::None};
            if (device_name == "DX")
                return {Device::DX, DeviceExtension::None};
            if (device_name == "DY")
                return {Device::DY, DeviceExtension::None};
            if (device_name == "Z")
                return {Device::Z, DeviceExtension::None};
            if (device_name == "R")
                return {Device::R, DeviceExtension::None};
            if (device_name == "ZR")
                return {Device::ZR, DeviceExtension::None};
            if (device_name == "U3E0")
                return {Device::G, DeviceExtension::CPUNo1};
            if (device_name == "U3E1")
                return {Device::G, DeviceExtension::CPUNo2};
            if (device_name == "U3E2")
                return {Device::G, DeviceExtension::CPUNo3};
            if (device_name == "U3E3")
                return {Device::G, DeviceExtension::CPUNo4};

            int32_t start_io_number = 0;
            if (RE2::PartialMatch(device_name, "^U([0-9A-Fa-f]{2,3})", RE2::Hex(&start_io_number)))
                return {Device::G, static_cast<DeviceExtension>(start_io_number)};

            return {Device::None, DeviceExtension::None};
        }
    };

    SLMP(std::string addr, int port, uint8_t network_no, uint8_t station_no, uint16_t module_io,
         uint8_t multidrop_station_no)
        : connected{false},
          ip_addr{std::move(addr)},
          socket(ip_addr.data(), port),
          buffer(buffer_size),
          request_data{SLMP_SHIFT_UINT16_T(Serialnumber::None),
                       SLMP_SHIFT_UINT8_T(network_no),
                       SLMP_SHIFT_UINT8_T(station_no),
                       SLMP_SHIFT_UINT16_T(module_io),
                       SLMP_SHIFT_UINT8_T(multidrop_station_no),
                       SLMP_SHIFT_UINT16_T(0),
                       SLMP_SHIFT_UINT16_T(MonitoringTimer::None),
                       SLMP_SHIFT_UINT16_T(0),
                       SLMP_SHIFT_UINT16_T(0)} {
        header_size = request_data.size();  // command and subcommand are counted as part of header
        request_data.reserve(request_data_size);
    }

    void disconnect() {
        UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Disconnected");
        connected = false;
    }

    void connect() {
        const auto result = socket.connect();
        if (!result.has_value()) {
#ifdef WIN32
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Couldn't connect to plc at address '%s:%d' %d",
                        socket.addr, socket.port, WSAGetLastError());
#else
            UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Couldn't connect to plc at address '%s:%d'",
                        socket.addr, socket.port);
#endif
            this->disconnect();
            return;
        }
        UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Connected with plc at address '%s:%d'", socket.addr,
                    socket.port);
        connected = true;
    }

    ~SLMP() {}

    std::optional<std::pair<std::byte*, std::size_t>> read_request(Device device, DeviceExtension device_extension,
                                                                   uint32_t head_no, uint16_t count) {
        auto subcommand = Subcommand::Word;  // default to 0000 subcommand instead of 0002
        if (device_extension != DeviceExtension::None) {
            subcommand = Subcommand::WordLongDeviceExtension;  // default to 0082 subcommand instead of 0080
            request_data.push_back(std::byte{0});
            request_data.push_back(std::byte{0});
            request_data.push_back(static_cast<std::byte>(head_no));
            request_data.push_back(static_cast<std::byte>(head_no >> 8));
            request_data.push_back(static_cast<std::byte>(head_no >> 16));
            request_data.push_back(static_cast<std::byte>(head_no >> 24));
            request_data.push_back(static_cast<std::byte>(device));
            request_data.push_back(static_cast<std::byte>(static_cast<uint16_t>(device) >> 8));
            request_data.push_back(std::byte{0});
            request_data.push_back(std::byte{0});
            request_data.push_back(static_cast<std::byte>(device_extension));
            request_data.push_back(static_cast<std::byte>(static_cast<uint16_t>(device_extension) >> 8));
            if (device_extension == DeviceExtension::CPUNo1 || device_extension == DeviceExtension::CPUNo2 ||
                device_extension == DeviceExtension::CPUNo3 || device_extension == DeviceExtension::CPUNo4) {
                // cpu buffer memory access
                request_data.push_back(std::byte{0xFA});
            } else {
                // module access
                request_data.push_back(std::byte{0xF8});
            }
        } else {
            request_data.push_back(static_cast<std::byte>(head_no));
            request_data.push_back(static_cast<std::byte>(head_no >> 8));
            request_data.push_back(static_cast<std::byte>(head_no >> 16));
            request_data.push_back(static_cast<std::byte>(device));
        }
        request_data.push_back(static_cast<std::byte>(count));
        request_data.push_back(static_cast<std::byte>(count >> 8));

        const auto retval = request(RequestCommand::Read, subcommand);

        if (retval.has_value() && buffer.size() >= 12) {
            const auto end_code = std::to_integer<uint32_t>(buffer[9]) + (std::to_integer<uint32_t>(buffer[10]) << 8);

            if (end_code != 0) {
                return {};
            }

            return {{&buffer[11], buffer.size() - 12}};
        }
        return {};
    }

    template <class T, WriteType write_type>
    std::optional<int32_t> write_request(Device device, DeviceExtension device_extension, uint32_t head_no,
                                         tcb::span<T> data) {
        const auto write_data_size = [&]() {
            if constexpr (write_type == WriteType::Bit) {
                return (data.size() * sizeof(T) + 1) / 2;  // to round up
            } else {
                return data.size() * sizeof(T);
            }
        }();
        const auto device_count = [&]() {
            if constexpr (write_type == WriteType::Word) {
                return (data.size() * sizeof(T) + 1) / 2;  // to round up
            } else {
                return data.size() * sizeof(T);
            }
        }();

        assert((write_data_size + header_size + 6) <= request_data_size && "Write request too big");

        auto subcommand = write_type == WriteType::Word
                              ? Subcommand::Word
                              : Subcommand::Bit;  // default to 0000/0001 subcommand instead of 0002/0003
        if (device_extension != DeviceExtension::None) {
            assert(write_type == WriteType::Word && "Writing with device extension is only allowed for word type");
            subcommand = Subcommand::WordLongDeviceExtension;  // default to 0082 subcommand instead of 0080
            request_data.push_back(std::byte{0});
            request_data.push_back(std::byte{0});
            request_data.push_back(static_cast<std::byte>(head_no));
            request_data.push_back(static_cast<std::byte>(head_no >> 8));
            request_data.push_back(static_cast<std::byte>(head_no >> 16));
            request_data.push_back(static_cast<std::byte>(head_no >> 24));
            request_data.push_back(static_cast<std::byte>(device));
            request_data.push_back(static_cast<std::byte>(static_cast<uint16_t>(device) >> 8));
            request_data.push_back(std::byte{0});
            request_data.push_back(std::byte{0});
            request_data.push_back(static_cast<std::byte>(device_extension));
            request_data.push_back(static_cast<std::byte>(static_cast<uint16_t>(device_extension) >> 8));
            if (device_extension == DeviceExtension::CPUNo1 || device_extension == DeviceExtension::CPUNo2 ||
                device_extension == DeviceExtension::CPUNo3 || device_extension == DeviceExtension::CPUNo4) {
                // cpu buffer memory access
                request_data.push_back(std::byte{0xFA});
            } else {
                // module access
                request_data.push_back(std::byte{0xF8});
            }
        } else {
            request_data.push_back(static_cast<std::byte>(head_no));
            request_data.push_back(static_cast<std::byte>(head_no >> 8));
            request_data.push_back(static_cast<std::byte>(head_no >> 16));
            request_data.push_back(static_cast<std::byte>(device));
        }
        request_data.push_back(static_cast<std::byte>(device_count));
        request_data.push_back(static_cast<std::byte>(device_count >> 8));

        if constexpr (write_type == WriteType::Bit) {
            for (std::size_t i = 0; i < write_data_size; i++) {
                uint8_t value = 0;
                for (std::size_t j = 0; j < 2 && j < write_data_size; j++) {
                    value += data[i * 2 + j] << (4 - j * 4);
                }
                request_data.push_back(static_cast<std::byte>(value));
            }
        } else {
            request_data.resize(request_data.size() + write_data_size);
            std::memcpy(&request_data.back() - write_data_size + 1, data.data(), data.size() * sizeof(T));
        }
        const auto retval = request(RequestCommand::Write, subcommand);
        if (retval.has_value() && buffer.size() >= 10) {
            const auto end_code = std::to_integer<uint32_t>(buffer[9]) + (std::to_integer<uint32_t>(buffer[10]) << 8);

            if (end_code != 0) {
                return {};
            }

            return 1;
        }
        return {};
    }

    template <class T>
    std::optional<int32_t> label_array_read_request(std::string label_name, tcb::span<T> label_data) {
        std::vector<std::string> label_names;
        label_names.reserve(label_data.size());
        for (std::size_t i = 0; i < label_data.size(); i++) {
            label_names.emplace_back(fmt::format(fmt::runtime(label_name + "[{}]"), i));
        }
        return label_read_request(label_names, label_data);
    }

    template <class T>
    std::optional<int32_t> label_read_request(const tcb::span<std::string> label_names, tcb::span<T> label_data) {
        request_data.push_back(static_cast<std::byte>(label_names.size()));
        request_data.push_back(static_cast<std::byte>(label_names.size() >> 8));
        request_data.push_back(std::byte{0});
        request_data.push_back(std::byte{0});

        for (std::size_t i = 0; i < label_names.size(); i++) {
            request_data.push_back(static_cast<std::byte>(label_names[i].size()));
            request_data.push_back(static_cast<std::byte>(label_names[i].size() >> 8));
            for (std::size_t j = 0; j < label_names[i].size(); j++) {
                request_data.push_back(static_cast<std::byte>(label_names[i][j]));
                request_data.push_back(std::byte{0});
            }
        }
        const auto retval = request(RequestCommand::RandomLabelRead, Subcommand::Word);

        if (retval.has_value()) {
            return response(RequestCommand::RandomLabelRead, Subcommand::Word, label_data, buffer.size());
        }
        return retval;
    }

    template <class T>
    std::optional<int32_t> label_write_request(const tcb::span<std::string> label_names, tcb::span<T> label_data) {
        auto write_data_length = 2 * ((sizeof(T) + 1) / 2);  // to round uint8_t up to 2

        request_data.push_back(static_cast<std::byte>(label_names.size()));
        request_data.push_back(static_cast<std::byte>(label_names.size() >> 8));
        request_data.push_back(std::byte{0});
        request_data.push_back(std::byte{0});

        for (std::size_t i = 0; i < label_names.size(); i++) {
            request_data.push_back(static_cast<std::byte>(label_names[i].size()));
            request_data.push_back(static_cast<std::byte>(label_names[i].size() >> 8));
            for (char j : label_names[i]) {
                request_data.push_back(static_cast<std::byte>(j));
                request_data.push_back(std::byte{0});
            }
            if constexpr (std::is_same_v<T, std::string>) {
                // 1 or 2 `0 bytes` at the end of the string
                write_data_length = (label_data[i].size() + 2) / 2 * 2;
            }
            request_data.push_back(static_cast<std::byte>(write_data_length));
            request_data.push_back(static_cast<std::byte>(write_data_length >> 8));
            request_data.resize(request_data.size() + write_data_length);
            if constexpr (std::is_same_v<T, std::string>) {
                std::memcpy(&request_data.back() - write_data_length + 1, label_data[i].data(), label_data[i].size());
            } else {
                std::memcpy(&request_data.back() - write_data_length + 1, &label_data[i], sizeof(T));
            }
        }
        return request(RequestCommand::RandomLabelWrite, Subcommand::Word);
    }

    volatile bool connected;

    template <typename Type>
    Type get(const Command& command);

    template <typename Type>
    void get(const Command& command, tcb::span<Type> data);

    template <typename Type>
    bool write(const Command& command, Type value);

   private:
    template <class T>
    std::optional<int32_t> response(RequestCommand command, Subcommand subcommand, tcb::span<T> read_data,
                                    std::size_t response_length) {
        if (response_length < 11) {
            return {};
        }

        const auto end_code = std::to_integer<uint32_t>(buffer[9]) + (std::to_integer<uint32_t>(buffer[10]) << 8);

        if (end_code != 0) {
            return {};
        }

        std::size_t i = 0;
        if (command == RequestCommand::Read) {
            if (subcommand == Subcommand::Word || subcommand == Subcommand::WordLong ||
                subcommand == Subcommand::WordLongDeviceExtension) {
                if (response_length < (11 + read_data.size() * sizeof(T))) {
                    return {};
                }
                std::memcpy(read_data.data(), buffer.data() + 11, read_data.size() * sizeof(T));
            } else if constexpr (!std::is_same_v<T, std::string>) {
                if (response_length < 12) {
                    return {};
                }

                for (i = 0; i < (response_length - 11) * 2; i++) {
                    read_data[i] = static_cast<T>((buffer[11 + i / 2] >> ((i % 2) ? 0 : 4)) & std::byte{0x0f});
                }
            }
        } else if (command == RequestCommand::RandomLabelRead) {
            if (response_length < 13) {
                return {};
            }

            const std::size_t label_count =
                std::to_integer<uint32_t>(buffer[11]) + (std::to_integer<uint32_t>(buffer[12]) << 8);
            std::size_t start_index = 13;
            for (i = 0; i < label_count; i++) {
                if (response_length < (start_index + 2)) {
                    return {};
                }

                const std::size_t data_size = std::to_integer<uint32_t>(buffer[start_index + 1]) +
                                              (std::to_integer<uint32_t>(buffer[start_index + 2]) << 8);

                if (response_length < (start_index + 2 + data_size)) {
                    return {};
                }

                if constexpr (std::is_same_v<T, std::string>) {
                    // std::string
                    read_data[i].assign(reinterpret_cast<char*>(&buffer.front() + start_index + 3), data_size);
                } else {
                    std::memcpy(&read_data[i], &buffer.front() + start_index + 3, sizeof(T));
                }
                start_index += 3 + data_size;
            }
        }
        return static_cast<int>(i);
    }

    std::string ip_addr;
    Socket socket;
    std::size_t buffer_size = 400;
    std::vector<std::byte> buffer;
    std::size_t request_data_size = 1296;
    std::size_t header_size;
    std::vector<std::byte> request_data;

    std::optional<int> request(RequestCommand command, Subcommand subcommand) {
        request_data[7] = static_cast<std::byte>(request_data.size() - header_size + 6);
        request_data[8] = static_cast<std::byte>((request_data.size() - header_size + 6) >> 8);
        request_data[11] = static_cast<std::byte>(command);
        request_data[12] = static_cast<std::byte>(static_cast<uint16_t>(command) >> 8);
        request_data[13] = static_cast<std::byte>(subcommand);
        request_data[14] = static_cast<std::byte>(static_cast<uint16_t>(subcommand) >> 8);
        const auto send_result = socket.send(request_data.data(), request_data.size());

        if (!send_result.has_value()) {
            request_data.resize(header_size);
            this->disconnect();
            return {};
        }
        const auto recv_result = socket.recv(buffer.data(), buffer.size());
        if (!recv_result.has_value()) {
            request_data.resize(header_size);
            this->disconnect();
            return {};
        }

        request_data.resize(header_size);
        return recv_result;
    }
};

template <typename Type>
void SLMP::get(const Command& command, tcb::span<Type> data) {
    if (command.is_label) {
        label_array_read_request(command.label, data);
    } else {
        const auto count = [&]() {
            if constexpr (std::is_same_v<Type, std::string>) {
                return data.size() * ((command.length + 1) / 2);
            } else if constexpr (std::is_same_v<Type, uint8_t>) {
                // array of booleans
                return (data.size() + 15) / 16;
            } else {
                return data.size() * ((sizeof(Type) + 1) / sizeof(uint16_t));
            }
        }();
        const auto answer = read_request(command.device, command.device_extension, command.head_no, count);
        if (answer.has_value()) {
            if constexpr (std::is_same_v<Type, uint8_t>) {
                // array of booleans
                if (answer.value().second < ((data.size() + 7) / 8)) {
                    return;
                }

                std::size_t index = 0;
                for (auto& value : data) {
                    if (index % 8 < 4) {
                        value = (static_cast<uint8_t>(answer.value().first[index / 8]) & (0x01 << (index % 4))) != 0;
                    } else {
                        value = (static_cast<uint8_t>(answer.value().first[index / 8]) & (0x10 << (index % 4))) != 0;
                    }
                    index++;
                }
            } else if constexpr (std::is_same_v<Type, std::string>) {
                if (answer.value().second < (data.size() * command.length)) {
                    return;
                }
                for (std::size_t i = 0; i < data.size(); i++) {
                    data[i].assign(reinterpret_cast<char*>(answer.value().first + i * command.length), command.length);
                }
            } else {
                if (answer.value().second < (sizeof(Type) * data.size())) {
                    return;
                }
                std::memcpy(data.data(), answer.value().first, sizeof(Type) * data.size());
            }
        }
    }
}

template <>
inline std::string SLMP::get<std::string>(const Command& command) {
    if (command.is_label) {
        std::string data;
        std::string label_name = command.label;  // :( have to copy label
        label_read_request<std::string>(tcb::span<std::string>{&label_name, 1}, tcb::span<std::string>{&data, 1});
        return data;
    } else {
        const auto answer =
            read_request(command.device, command.device_extension, command.head_no, (command.length + 1) / 2);
        if (answer.has_value()) {
            return {reinterpret_cast<char*>(answer.value().first), answer.value().second};
        }
    }
    return "";
}

template <>
inline bool SLMP::get<bool>(const Command& command) {
    return SLMP::get<uint16_t>(command) != 0;
}

template <typename Type>
Type SLMP::get(const Command& command) {
    Type value{};
    if (command.is_label) {
        std::string label_name = command.label;  // :( have to copy label
        label_read_request<Type>(tcb::span<std::string>{&label_name, 1}, tcb::span<Type>{&value, 1});
    } else {
        const auto data = read_request(command.device, command.device_extension, command.head_no,
                                       (sizeof(Type) + 1) / sizeof(uint16_t));
        if (data.has_value() && data.value().second >= sizeof(Type)) {
            std::memcpy(&value, data.value().first, sizeof(Type));
        }
    }
    return value;
}

template <>
inline bool SLMP::write<uint8_t>(const Command& command, uint8_t value) {
    if (command.is_label) {
        std::string label_name = command.label;  // :( have to copy label
        const auto answer =
            label_write_request<uint8_t>(tcb::span<std::string>{&label_name, 1}, tcb::span<uint8_t>{&value, 1});
        return answer.has_value();
    } else {
        const auto answer = write_request<uint8_t, SLMP::WriteType::Bit>(
            command.device, command.device_extension, command.head_no, tcb::span<uint8_t>{&value, 1});
        return answer.has_value();
    }
}

template <typename Type>
bool SLMP::write(const Command& command, Type value) {
    if (command.is_label) {
        std::string label_name = command.label;  // :( have to copy label
        const auto answer =
            label_write_request<Type>(tcb::span<std::string>{&label_name, 1}, tcb::span<Type>{&value, 1});
        return answer.has_value();
    } else {
        const auto answer = write_request<Type, SLMP::WriteType::Word>(command.device, command.device_extension,
                                                                       command.head_no, tcb::span<Type>{&value, 1});
        return answer.has_value();
    }
}