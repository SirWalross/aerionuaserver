#pragma once

#include <fmt/format.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>

#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "r3.h"
#include "wrapper.h"

struct RobotNode {
    UA_NodeId node;
    std::vector<RobotNode> children;
    std::optional<R3::Command> read_command;
    std::optional<R3::Command> write_command;
    std::size_t count;
    std::string datatype;
    std::string name;

    explicit RobotNode(UA_NodeId node) : node{node}, count{0} {}

    RobotNode const* get_node(UA_UInt16 namespace_id, UA_UInt32 identifier) const {
        for (const auto& child_node : children) {
            if (child_node.node.namespaceIndex == namespace_id &&
                child_node.node.identifierType == UA_NODEIDTYPE_NUMERIC &&
                child_node.node.identifier.numeric == identifier) {
                return &child_node;
            }
        }
        for (const auto& child_node : children) {
            const auto result = child_node.get_node(namespace_id, identifier);
            if (result != nullptr) {
                return result;
            }
        }
        return nullptr;
    }

    [[nodiscard]] RobotNode* contains(const std::string child_name) {
        for (std::size_t i = 0; i < children.size(); i++) {
            if (children[i].name == child_name) {
                return &children[i];
            }
        }
        return nullptr;
    }
};

struct Robot : public Client {
    std::string name;
    R3 r3;
    RobotNode node;

    Robot(std::string name, std::string ip, int port) : name{std::move(name)}, r3(std::move(ip), port), node{{}} {}

    bool connected() {
        return r3.connected;
    }
};

auto format_read_command(const R3::Command& read_command, std::size_t j = 0) -> std::pair<std::string, std::string> {
    return {fmt::format("{};{};{}", read_command.mecha_no, read_command.task_slot_no,
                        fmt::format(fmt::runtime(read_command.command), fmt::arg("i", read_command.id),
                                    fmt::arg("i16", 16 * (read_command.id - 1)), fmt::arg("j", j))),
            fmt::format(fmt::runtime(read_command.match), fmt::arg("i", read_command.id),
                        fmt::arg("i1", read_command.id - 1), fmt::arg("i2", 2 * (read_command.id - 1)),
                        fmt::arg("i3", 3 * (read_command.id - 1)))};
}

template <typename T>
auto format_write_command(const R3::Command& write_command, T value) -> std::string {
    return fmt::format("{};{};{}", write_command.mecha_no, write_command.task_slot_no,
                       fmt::format(fmt::runtime(write_command.command), fmt::arg("i", write_command.id),
                                   fmt::arg("i16", 16 * (write_command.id - 1)), fmt::arg("value", value)));
}

auto format_name(const std::string& name, uint16_t id) -> std::string {
    return fmt::format(fmt::runtime(name), fmt::arg("i", id), fmt::arg("first16", (id - 1) * 16),
                       fmt::arg("last16", id * 16 - 1));
}

static UA_StatusCode read_robot_value(UA_Server* server, const UA_NodeId* sessionId, void* sessionContext,
                                      const UA_NodeId* nodeId, void* nodeContext, UA_Boolean sourceTimeStamp,
                                      const UA_NumericRange* range, UA_DataValue* dataValue) {
    auto robot = static_cast<Robot*>(nodeContext);
    const RobotNode* node;
    if (robot && (node = robot->node.get_node(nodeId->namespaceIndex, nodeId->identifier.numeric)) != nullptr &&
        node->read_command.has_value()) {
        if (!robot->r3.connected) {
            return UA_STATUSCODE_BADDEVICEFAILURE;
        }
        const auto id = node->read_command.value().id;
        const auto [read_command, match] = format_read_command(node->read_command.value());
        if (node->datatype == "Double") {
            auto value = robot->r3.get<double>(read_command, match);
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
        } else if (node->datatype == "Float") {
            auto value = robot->r3.get<float>(read_command, match);
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_FLOAT]);
        } else if (node->datatype == "Int32") {
            auto value = robot->r3.get<int32_t>(read_command, match);
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_INT32]);
        } else if (node->datatype == "HexInt32") {
            auto value = robot->r3.get_hex<int32_t>(read_command, match);
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_INT32]);
        } else if (node->datatype == "Int64") {
            auto value = robot->r3.get<int64_t>(read_command, match);
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_INT64]);
        } else if (node->datatype == "UInt32") {
            auto value = robot->r3.get<uint32_t>(read_command, match);
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_UINT32]);
        } else if (node->datatype == "UInt64") {
            auto value = robot->r3.get<uint64_t>(read_command, match);
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_UINT64]);
        } else if (node->datatype == "Bool") {
            auto value = robot->r3.get<bool>(read_command, match, node->read_command.value().position);
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_BOOLEAN]);
        } else if (node->datatype == "String") {
            auto data = robot->r3.get<std::string>(read_command, match);
            auto value = UA_STRING(data.data());
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_STRING]);
        } else if (node->datatype == "LocalizedText") {
            auto data = robot->r3.get<std::string>(read_command, match);
            auto value = UA_LOCALIZEDTEXT(locale, data.data());
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        } else if (node->datatype == "Enum") {
            auto data = robot->r3.get<std::string>(read_command, match);
            std::string enum_string;
            int64_t enum_value = -1;
            for (const auto& tuple : node->read_command->cases) {
                if ((std::get<0>(tuple) == "Default" && enum_value == -1) ||
                    RE2::PartialMatch(data, std::get<0>(tuple))) {
                    enum_string = std::get<1>(tuple);
                    enum_value = std::get<2>(tuple);
                }
            }
            auto value = Datatype<UA_EnumValueType>(enum_value, enum_string);
            UA_Variant_setScalarCopy(&dataValue->value, &value.value, &UA_TYPES[UA_TYPES_ENUMVALUETYPE]);
        } else {
            throw std::runtime_error{"Invalid data type"};
        }
    } else {
        double value = 0;
        UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
    }
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode read_robot_array_value(UA_Server* server, const UA_NodeId* sessionId, void* sessionContext,
                                            const UA_NodeId* nodeId, void* nodeContext, UA_Boolean sourceTimeStamp,
                                            const UA_NumericRange* range, UA_DataValue* dataValue) {
    const auto robot = static_cast<Robot*>(nodeContext);
    const RobotNode* node;
    if (robot && (node = robot->node.get_node(nodeId->namespaceIndex, nodeId->identifier.numeric)) != nullptr &&
        node->read_command.has_value()) {
        if (!robot->r3.connected) {
            return UA_STATUSCODE_BADDEVICEFAILURE;
        }
        if (node->datatype == "Position" || node->datatype == "Joint") {
            const auto [read_command, match] = format_read_command(node->read_command.value());
            if (node->datatype == "Position") {
                std::array<double, 10> position{};
                robot->r3.get_position(read_command, match, position.data(), position.size());
                UA_Variant_setArrayCopy(&dataValue->value, position.data(), position.size(),
                                        &UA_TYPES[UA_TYPES_DOUBLE]);
            } else {
                std::array<double, 8> position{};
                robot->r3.get_position(read_command, match, position.data(), position.size());
                UA_Variant_setArrayCopy(&dataValue->value, position.data(), position.size(),
                                        &UA_TYPES[UA_TYPES_DOUBLE]);
            }
        } else {
            if (node->datatype == "Double") {
                // double array
                std::vector<double> values(node->count);
                for (std::size_t i = 0; i < node->count; i++) {
                    const auto [read_command, match] = format_read_command(node->read_command.value(), i + 1);
                    values[i] = robot->r3.get<double>(read_command, match);
                }
                UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_DOUBLE]);
            } else if (node->datatype == "Int32") {
                // int32 array
                std::vector<int32_t> values(node->count);
                for (std::size_t i = 0; i < node->count; i++) {
                    const auto [read_command, match] = format_read_command(node->read_command.value(), i + 1);
                    values[i] = robot->r3.get<int32_t>(read_command, match);
                }
                UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_INT32]);
            } else if (node->datatype == "String") {
                // string array
                std::vector<UA_String> values{};
                values.reserve(node->count);
                for (std::size_t i = 0; i < node->count; i++) {
                    const auto [read_command, match] = format_read_command(node->read_command.value(), i + 1);
                    values.emplace_back(UA_STRING_ALLOC(robot->r3.get<std::string>(read_command, match).data()));
                }
                UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_STRING]);
            } else {
                throw std::runtime_error{"Invalid data type"};
            }
        }
    } else {
        const double value = 0;
        UA_Variant_setArrayCopy(&dataValue->value, &value, 1, &UA_TYPES[UA_TYPES_DOUBLE]);
    }
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode write_robot_value(UA_Server* server, const UA_NodeId* sessionId, void* sessionContext,
                                       const UA_NodeId* nodeId, void* nodeContext, const UA_NumericRange* range,
                                       const UA_DataValue* dataValue) {
    const auto robot = static_cast<Robot*>(nodeContext);
    const RobotNode* node;
    if (robot && (node = robot->node.get_node(nodeId->namespaceIndex, nodeId->identifier.numeric)) != nullptr &&
        node->write_command.has_value()) {
        if (!robot->r3.connected || dataValue->value.arrayLength != 0) {
            return UA_STATUSCODE_BADDEVICEFAILURE;
        }
        std::string command;
        if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_DOUBLE].typeKind) {
            command = format_write_command(node->write_command.value(), *static_cast<double*>(dataValue->value.data));
        } else if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_INT32].typeKind) {
            command = format_write_command(node->write_command.value(), *static_cast<int32_t*>(dataValue->value.data));
        } else if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_UINT32].typeKind) {
            command = format_write_command(node->write_command.value(), *static_cast<uint32_t*>(dataValue->value.data));
        } else {
            return UA_STATUSCODE_BADDEVICEFAILURE;
        }
        return robot->r3.execute(command) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADDEVICEFAILURE;
    }
    return UA_STATUSCODE_GOOD;
}

inline void parse_robot_node(Robot* robot, UA_Server* server, RobotNode* parent, const nlohmann::basic_json<>& node,
                             int mecha_no, int task_slot_no, uint16_t id = 0) {
    const auto type = node["Type"].get<std::string>();
    auto name = format_name(node["Name"].get<std::string>(), id);
    if (type == "Folder") {
        if (name == "AdditionalComponents" && mecha_no != 1) {
            // only show AdditionalComponents on first mecha
            return;
        }

        parent->children.emplace_back(
            addObjectNode(server, name.data(), parent->node,
                          !node.contains("DisplayType") || node["DisplayType"].get<std::string>() != "Object"));
        parent->children.back().name = name;
        uint16_t count = 0;
        if (node["Count"].is_object()) {
            const auto [read_command, match] =
                format_read_command({node["Count"]["Command"].get<std::string>(),
                                     node["Count"]["Match"].get<std::string>(), mecha_no, task_slot_no});
            if (node["Count"]["Datatype"].get<std::string>() == "BitCount") {
                count = __builtin_popcount(robot->r3.get_hex<uint16_t>(read_command, match));
            } else if (node["Count"]["Datatype"].get<std::string>() == "HexUInt") {
                count = robot->r3.get_hex<uint16_t>(read_command, match);
            } else {
                count = robot->r3.get<uint16_t>(read_command, match);
            }
        } else {
            count = node["Count"].get<uint16_t>();
        }
        for (uint16_t i = 0; i < count; i++) {
            parse_robot_node(robot, server, &parent->children.back(), node["FolderChild"], mecha_no, task_slot_no,
                             i + 1);
            if (name == "MotionDevices") {
                mecha_no++;
            } else if (name == "TaskControls") {
                task_slot_no++;
            }
        }
    } else if (type == "Object") {
        if (node.contains("Condition")) {
            const auto [read_command, match] =
                format_read_command({node["Condition"]["Command"].get<std::string>(),
                                     node["Condition"]["Match"].get<std::string>(), mecha_no, task_slot_no, id});
            if (!RE2::PartialMatch(robot->r3.get(read_command), match)) {
                return;
            }
        }
        parent->children.emplace_back(addObjectNode(server, name.data(), parent->node, false));
        parent->children.back().name = name;
        for (const auto& child_node : node["Children"]) {
            parse_robot_node(robot, server, &parent->children.back(), child_node, mecha_no, task_slot_no, id);
        }
    } else if (type == "Property") {
        std::string value;  // always of type string
        if (node.contains("Value")) {
            value = fmt::format(fmt::runtime(node["Value"].get<std::string>()), fmt::arg("i", id));
        } else if (name == "Model" && RE2::PartialMatch(parent->name, "MotionDevice_") &&
                   parent->name != "MotionDevice_1") {
            // all mechas except mecha 1 have name "USER"
            value = "USER";
        } else {
            const auto [read_command, match] =
                format_read_command({node["ReadCommand"]["Command"].get<std::string>(),
                                     node["ReadCommand"]["Match"].get<std::string>(), mecha_no, task_slot_no, id});
            value = robot->r3.get<std::string>(read_command, match);
        }
        // TODO: differentiate between UA_TYPES_STRING and UA_TYPES_LOCALIZEDTEXT
        auto value_type_obj = Datatype<UA_String>(value.data());
        parent->children.emplace_back(
            addVariableNode<UA_String>(server, robot, name.data(), {parent->node}, {value_type_obj}, {}, {}, 0u));
        parent->children.back().name = name;
    } else if (type == "EnumProperty") {
        const auto [read_command, match] =
            format_read_command({node["ReadCommand"]["Command"].get<std::string>(),
                                 node["ReadCommand"]["Match"].get<std::string>(), mecha_no, task_slot_no, id});
        auto answer = robot->r3.get<std::string>(read_command, match);
        std::string enum_string;
        int64_t enum_value = -1;
        for (const auto& it : node["Cases"].items()) {
            if (RE2::PartialMatch(answer, it.key())) {
                if (name == "MotionProfile" && mecha_no == 1 && id == 3 && RE2::PartialMatch(answer, "^[rR][hH]")) {
                    // special case for j3 axis on rh robots
                    enum_string = "LINEAR";
                    enum_value = 3;
                    break;
                }
                if (name == "MotionDeviceCategory" && mecha_no != 1) {
                    // all mechas except mecha 1 are of type other
                    enum_string = "OTHER";
                    enum_value = 0;
                    break;
                }
                enum_string = it.value()["EnumString"].get<std::string>();
                enum_value = it.value()["Value"].get<int64_t>();
                break;
            }
        }
        if (enum_value == -1) {
            enum_string = node["Cases"]["Default"]["EnumString"].get<std::string>();
            enum_value = node["Cases"]["Default"]["Value"].get<int64_t>();
        }
        auto value_type_obj = Datatype<UA_EnumValueType>(enum_value, enum_string);
        parent->children.emplace_back(
            addVariableNode<UA_EnumValueType>(server, robot, name.data(), {parent->node}, {value_type_obj}, {}, {}, 0));
        parent->children.back().name = name;
    } else {
        uint32_t count = node.contains("Count") ? node["Count"].get<uint32_t>() : 0;

        // value or enum value
        if (node.contains("Writeable") && node["Writeable"].get<bool>() && count == 0) {
            // read- and writeable (only for non array types for now)
            parent->children.emplace_back(addVariableNode<UA_String>(server, robot, name.data(), {parent->node}, {},
                                                                     read_robot_value, write_robot_value, count));

            // save write command
            auto write_command = node["WriteCommand"]["Command"].get<std::string>();
            parent->children.back().write_command = std::make_optional<R3::Command>(write_command);
            parent->children.back().write_command->mecha_no = mecha_no;
            parent->children.back().write_command->id = id;
        } else {
            // only readable
            parent->children.emplace_back(
                addVariableNode<UA_String>(server, robot, name.data(), {parent->node}, {},
                                           (count == 0) ? read_robot_value : read_robot_array_value, {}, count));
        }

        // save read command
        auto read_command = node["ReadCommand"]["Command"].get<std::string>();
        auto match = node["ReadCommand"]["Match"].get<std::string>();
        auto datatype = node["Datatype"].get<std::string>();
        parent->children.back().read_command = std::make_optional<R3::Command>(read_command, match);
        parent->children.back().read_command->mecha_no = mecha_no;
        parent->children.back().read_command->task_slot_no = task_slot_no;
        parent->children.back().read_command->id = id;
        parent->children.back().datatype = datatype;
        parent->children.back().count = count;
        parent->children.back().name = name;

        // save enum values
        if (node.contains("Cases")) {
            // enum value
            for (const auto& it : node["Cases"].items()) {
                parent->children.back().read_command->cases.emplace_back(
                    it.key(), it.value()["EnumString"].get<std::string>(), it.value()["Value"].get<int64_t>());
            }
        }
    }
}

inline void parse_robot_user_node(Robot* robot, UA_Server* server, RobotNode* base_node,
                                  const nlohmann::basic_json<>& node) {
    std::string name = node["Name"].get<std::string>().data();
    std::string parent = node["Parent"].get<std::string>().data();

    // parent names are split with '/'
    // also non-existent parents are created as object nodes
    std::stringstream stream{parent};
    std::string new_parent{};
    auto parent_node = base_node;
    while (std::getline(stream, new_parent, '/')) {
        // check if new_parent already exists
        auto new_parent_node = parent_node->contains(new_parent);

        if (new_parent_node == nullptr) {
            // create parent node
            parent_node->children.emplace_back(addObjectNode(server, new_parent.data(), parent_node->node, false));
            parent_node->children.back().name = new_parent;
            new_parent_node = &parent_node->children.back();
        }
        parent_node = new_parent_node;
    }

    const auto datatype = node["Datatype"].get<std::string>();
    const uint32_t count =
        (datatype == "Position")
            ? 10
            : ((datatype == "Joint") ? 8 : (node.contains("Count") ? node["Count"].get<uint32_t>() : 0));

    if (node.contains("Writeable") && node["Writeable"].get<bool>() && count == 0) {
        // read- and writeable (only for non array types for now)
        parent_node->children.emplace_back(addVariableNode<UA_String>(server, robot, name.data(), {parent_node->node},
                                                                      {}, read_robot_value, write_robot_value, count));

        // save write command
        auto write_command = node["WriteCommand"]["Command"].get<std::string>();

        // check if write command contains mecha no and task_slot_not
        int mecha_no = 1;
        int task_slot_no = 1;
        RE2::PartialMatch(write_command, "^(\\d{1,2});(\\d{1,2});", &mecha_no, &task_slot_no);

        parent_node->children.back().write_command = std::make_optional<R3::Command>(write_command);
        parent_node->children.back().write_command->mecha_no = mecha_no;
        parent_node->children.back().write_command->task_slot_no = task_slot_no;
    } else {
        // only readable
        parent_node->children.emplace_back(
            addVariableNode<UA_String>(server, robot, name.data(), {parent_node->node}, {},
                                       (count == 0) ? read_robot_value : read_robot_array_value, {}, count));
    }

    // save read command
    auto read_command = node["ReadCommand"]["Command"].get<std::string>();

    int mecha_no = 1;
    int task_slot_no = 1;
    RE2::PartialMatch(read_command, "^(\\d{1,2});(\\d{1,2});", &mecha_no, &task_slot_no);

    auto match = node["ReadCommand"]["Match"].get<std::string>();
    parent_node->children.back().read_command = std::make_optional<R3::Command>(read_command, match);
    parent_node->children.back().read_command->mecha_no = mecha_no;
    parent_node->children.back().read_command->task_slot_no = task_slot_no;
    parent_node->children.back().datatype = datatype;
    parent_node->children.back().count = count;
}

inline void create_robot_node(Robot* robot, UA_Server* server, const char* client_file_name) {
    std::ifstream specs("specifications/robot-specification.json");
    nlohmann::json data = nlohmann::json::parse(specs, nullptr, true, true);

    robot->node.node = addObjectNode(server, robot->name.data(), {}, false);

    for (const auto& node : data["Nodes"]) {
        parse_robot_node(robot, server, &robot->node, node, 1, 1);
    }

    std::ifstream client_file(client_file_name);
    nlohmann::json clients_data = nlohmann::json::parse(client_file, nullptr, true, true);

    for (const auto& client_node : clients_data["Clients"]) {
        if (client_node["Name"] == robot->name && client_node.contains("UserNodes")) {
            for (const auto& user_node : client_node["UserNodes"]) {
                parse_robot_user_node(robot, server, &robot->node, user_node);
            }
        }
    }
    UA_LOG_INFO(&file_logger, UA_LOGCATEGORY_USERLAND, "Created robot node");
}