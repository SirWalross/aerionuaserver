#pragma once

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

#include "slmp.h"
#include "wrapper.h"

struct PLCNode {
    UA_NodeId node;
    std::vector<PLCNode> children;
    std::optional<SLMP::Command> read_command;
    std::string datatype;
    std::size_t count;
    std::string name;
    bool writeable;

    explicit PLCNode(UA_NodeId node) : node{node}, count{0}, writeable{false} {}

    PLCNode const* get_node(UA_UInt16 namespace_id, UA_UInt32 identifier) const {
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

    [[nodiscard]] PLCNode* contains(const std::string child_name) {
        for (auto& i : children) {
            if (i.name == child_name) {
                return &i;
            }
        }
        return nullptr;
    }
};

struct PLC : public Client {
    std::string name;
    SLMP slmp;
    PLCNode node;

    PLC(std::string name, std::string ip, int port, uint8_t network_no, uint8_t station_no, uint16_t module_io,
        uint8_t multidrop_station_no)
        : name{std::move(name)},
          slmp(std::move(ip), port, network_no, station_no, module_io, multidrop_station_no),
          node{{}} {}

    bool connected() {
        return slmp.connected;
    }
};

static UA_StatusCode read_plc_value(UA_Server* server, const UA_NodeId* sessionId, void* sessionContext,
                                    const UA_NodeId* nodeId, void* nodeContext, UA_Boolean sourceTimeStamp,
                                    const UA_NumericRange* range, UA_DataValue* dataValue) {
    const auto plc = static_cast<PLC*>(nodeContext);
    const PLCNode* node;
    if (plc && (node = plc->node.get_node(nodeId->namespaceIndex, nodeId->identifier.numeric)) != nullptr &&
        node->read_command.has_value()) {
        if (!plc->slmp.connected) {
            return UA_STATUSCODE_BADDEVICEFAILURE;
        }
        if (node->datatype == "Bool") {
            const auto value = plc->slmp.get<bool>(node->read_command.value());
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_BOOLEAN]);
        } else if (node->datatype == "Word") {
            const auto value = plc->slmp.get<uint16_t>(node->read_command.value());
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_UINT16]);
        } else if (node->datatype == "DWord") {
            const auto value = plc->slmp.get<uint32_t>(node->read_command.value());
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_UINT32]);
        } else if (node->datatype == "Int") {
            const auto value = plc->slmp.get<int16_t>(node->read_command.value());
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_INT16]);
        } else if (node->datatype == "DInt") {
            const auto value = plc->slmp.get<int32_t>(node->read_command.value());
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_INT32]);
        } else if (node->datatype == "Float") {
            const auto value = plc->slmp.get<float>(node->read_command.value());
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_FLOAT]);
        } else if (node->datatype == "Double") {
            const auto value = plc->slmp.get<double>(node->read_command.value());
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
        } else if (node->datatype == "String") {
            auto data = plc->slmp.get<std::string>(node->read_command.value());
            const auto value = UA_STRING(data.data());
            UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_STRING]);
        } else {
            throw std::runtime_error{"Invalid data type"};
        }
    } else {
        const double value = 0;
        UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
    }
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode write_plc_value(UA_Server* server, const UA_NodeId* sessionId, void* sessionContext,
                                     const UA_NodeId* nodeId, void* nodeContext, const UA_NumericRange* range,
                                     const UA_DataValue* dataValue) {
    const auto plc = static_cast<PLC*>(nodeContext);
    const PLCNode* node;
    if (plc && (node = plc->node.get_node(nodeId->namespaceIndex, nodeId->identifier.numeric)) != nullptr &&
        node->writeable && node->read_command.has_value()) {
        if (!plc->slmp.connected || dataValue->value.arrayLength != 0) {
            return UA_STATUSCODE_BADDEVICEFAILURE;
        }
        bool answer;
        if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_BOOLEAN].typeKind) {
            answer = plc->slmp.write(node->read_command.value(), *static_cast<uint8_t*>(dataValue->value.data));
        } else if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_DOUBLE].typeKind) {
            answer = plc->slmp.write(node->read_command.value(), *static_cast<double*>(dataValue->value.data));
        } else if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_FLOAT].typeKind) {
            answer = plc->slmp.write(node->read_command.value(), *static_cast<float*>(dataValue->value.data));
        } else if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_INT32].typeKind) {
            answer = plc->slmp.write(node->read_command.value(), *static_cast<int32_t*>(dataValue->value.data));
        } else if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_UINT32].typeKind) {
            answer = plc->slmp.write(node->read_command.value(), *static_cast<uint32_t*>(dataValue->value.data));
        } else if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_INT16].typeKind) {
            answer = plc->slmp.write(node->read_command.value(), *static_cast<int16_t*>(dataValue->value.data));
        } else if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_UINT16].typeKind) {
            answer = plc->slmp.write(node->read_command.value(), *static_cast<uint16_t*>(dataValue->value.data));
        } else if (dataValue->value.type->typeKind == UA_TYPES[UA_TYPES_STRING].typeKind) {
            // writing of strings only supported for global labels
            UA_String ua_string = *static_cast<UA_String*>(dataValue->value.data);
            std::string data(reinterpret_cast<const char*>(ua_string.data), ua_string.length);
            answer = plc->slmp.write(node->read_command.value(), data);
        } else {
            return UA_STATUSCODE_BADDEVICEFAILURE;
        }
        return answer ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADDEVICEFAILURE;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode read_plc_array_value(UA_Server* server, const UA_NodeId* sessionId, void* sessionContext,
                                          const UA_NodeId* nodeId, void* nodeContext, UA_Boolean sourceTimeStamp,
                                          const UA_NumericRange* range, UA_DataValue* dataValue) {
    const auto plc = static_cast<PLC*>(nodeContext);
    const PLCNode* node;
    if (plc && (node = plc->node.get_node(nodeId->namespaceIndex, nodeId->identifier.numeric)) != nullptr &&
        node->read_command.has_value()) {
        if (!plc->slmp.connected) {
            return UA_STATUSCODE_BADDEVICEFAILURE;
        }
        if (node->datatype == "Bool") {
            std::vector<uint8_t> values(node->count);
            plc->slmp.get<uint8_t>(node->read_command.value(), values);
            UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_BOOLEAN]);
        } else if (node->datatype == "Word") {
            std::vector<uint16_t> values(node->count);
            plc->slmp.get<uint16_t>(node->read_command.value(), values);
            UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_UINT16]);
        } else if (node->datatype == "DWord") {
            std::vector<uint32_t> values(node->count);
            plc->slmp.get<uint32_t>(node->read_command.value(), values);
            UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_UINT32]);
        } else if (node->datatype == "Int") {
            std::vector<int16_t> values(node->count);
            plc->slmp.get<int16_t>(node->read_command.value(), values);
            UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_INT16]);
        } else if (node->datatype == "DInt") {
            std::vector<int32_t> values(node->count);
            plc->slmp.get<int32_t>(node->read_command.value(), values);
            UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_INT32]);
        } else if (node->datatype == "Float") {
            std::vector<float> values(node->count);
            plc->slmp.get<float>(node->read_command.value(), values);
            UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_FLOAT]);
        } else if (node->datatype == "Double") {
            std::vector<double> values(node->count);
            plc->slmp.get<double>(node->read_command.value(), values);
            UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_DOUBLE]);
        } else if (node->datatype == "String") {
            std::vector<std::string> strings(node->count);
            plc->slmp.get<std::string>(node->read_command.value(), strings);
            std::vector<UA_String> values(node->count);
            for (std::size_t i = 0; i < strings.size(); i++) {
                values[i] = UA_STRING(strings[i].data());
            }
            UA_Variant_setArrayCopy(&dataValue->value, values.data(), values.size(), &UA_TYPES[UA_TYPES_STRING]);
        } else {
            throw std::runtime_error{"Invalid data type"};
        }
    } else {
        const double value = 0;
        UA_Variant_setArrayCopy(&dataValue->value, &value, 1, &UA_TYPES[UA_TYPES_DOUBLE]);
    }
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

inline void parse_plc_node(PLC* plc, UA_Server* server, PLCNode* parent, const nlohmann::basic_json<>& node,
                           int id = 0) {
    const auto type = node["Type"].get<std::string>();
    std::string name = node["Name"].get<std::string>();
    const uint32_t count = node.contains("Count") ? node["Count"].get<uint32_t>() : 0;
    if (type == "Object") {
        auto existing_object = parent->contains(name);
        if (existing_object == nullptr) {
            parent->children.emplace_back(addObjectNode(server, name.data(), parent->node, false));
            parent->children.back().name = name;
            existing_object = &parent->children.back();
        }
        for (const auto& child_node : node["Children"]) {
            parse_plc_node(plc, server, existing_object, child_node, id);
        }
    } else if (type == "Property") {
        const auto [device, device_extension] =
            SLMP::Command::convert_device_name(node["ReadCommand"]["Device"].get<std::string>());
        const SLMP::Command command(
            device, device_extension, node["ReadCommand"]["Head no"],
            node["ReadCommand"].contains("Length") ? node["ReadCommand"]["Length"].get<uint16_t>() : 1u);
        std::string value = plc->slmp.get<std::string>(command);
        auto datatype_obj = Datatype<UA_String>(value.data());
        parent->children.emplace_back(
            addVariableNode<UA_String>(server, plc, name.data(), {parent->node}, {datatype_obj}, {}, {}, 0));
    } else if (type == "Device" || type == "GlobalLabel") {
        // value or enum value
        if (node.contains("Writeable") && node["Writeable"].get<bool>() && count <= 1) {
            // read- and writeable (only for non array types for now)
            parent->children.emplace_back(addVariableNode<UA_String>(server, plc, name.data(), {parent->node}, {},
                                                                     read_plc_value, write_plc_value, count));
            parent->children.back().writeable = true;
        } else {
            // only readable
            parent->children.emplace_back(
                addVariableNode<UA_String>(server, plc, name.data(), {parent->node}, {},
                                           (count <= 1) ? read_plc_value : read_plc_array_value, {}, count));
        }

        // save read command
        if (type == "Device") {
            const auto [device, device_extension] =
                SLMP::Command::convert_device_name(node["ReadCommand"]["Device"].get<std::string>());
            if (device == SLMP::Device::None) {
                UA_LOG_WARNING(&file_logger, UA_LOGCATEGORY_USERLAND, "Device '%s' is not a valid slmp device",
                               node["ReadCommand"]["Device"].get<std::string>().c_str());
                return;
            }
            parent->children.back().read_command = std::make_optional<SLMP::Command>(
                device, device_extension, node["ReadCommand"]["Head no"].get<uint32_t>(),
                node["ReadCommand"].contains("Length") ? node["ReadCommand"]["Length"].get<uint16_t>() : 1u);
            parent->children.back().datatype = node["Datatype"];
            parent->children.back().count = count;
        } else if (type == "GlobalLabel") {
            // save read command
            parent->children.back().read_command =
                std::make_optional<SLMP::Command>(node["ReadCommand"]["Label"].get<std::string>());
            parent->children.back().datatype = node["Datatype"];
            parent->children.back().count = count;
        }
    }
}

inline void parse_plc_user_node(PLC* plc, UA_Server* server, PLCNode* base_node, const nlohmann::basic_json<>& node,
                                int id = 0) {
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

    const auto type = node["Type"].get<std::string>();
    const uint32_t count = node.contains("Count") ? node["Count"].get<uint32_t>() : 0;

    if (node.contains("Writeable") && node["Writeable"].get<bool>() && count <= 1) {
        // read- and writeable (only for non array types for now)
        parent_node->children.emplace_back(addVariableNode<UA_String>(server, plc, name.data(), {parent_node->node}, {},
                                                                      read_plc_value, write_plc_value, count));
        parent_node->children.back().writeable = true;
    } else {
        // only readable
        parent_node->children.emplace_back(
            addVariableNode<UA_String>(server, plc, name.data(), {parent_node->node}, {},
                                       (count <= 1) ? read_plc_value : read_plc_array_value, {}, count));
    }

    // save read command
    if (type == "Device") {
        const auto [device, device_extension] =
            SLMP::Command::convert_device_name(node["ReadCommand"]["Device"].get<std::string>());

        if (device == SLMP::Device::None) {
            UA_LOG_WARNING(&file_logger, UA_LOGCATEGORY_USERLAND, "Device '%s' is not a valid slmp device",
                           node["ReadCommand"]["Device"].get<std::string>().c_str());
            return;
        }
        parent_node->children.back().read_command = std::make_optional<SLMP::Command>(
            device, device_extension, node["ReadCommand"]["Head no"].get<uint32_t>(),
            node["ReadCommand"].contains("Length") ? node["ReadCommand"]["Length"].get<uint16_t>() : 1u);
        parent_node->children.back().datatype = node["Datatype"];
        parent_node->children.back().count = count;
    } else if (type == "GlobalLabel") {
        // save read command
        parent_node->children.back().read_command =
            std::make_optional<SLMP::Command>(node["ReadCommand"]["Label"].get<std::string>());
        parent_node->children.back().datatype = node["Datatype"];
        parent_node->children.back().count = count;
    }
}

inline void create_plc_node(PLC* plc, UA_Server* server, const char* client_file_name) {
    std::ifstream specs("specifications/plc-specification.json");
    nlohmann::json data = nlohmann::json::parse(specs, nullptr, true, true);

    plc->node.node = addObjectNode(server, plc->name.data(), {}, false);

    for (const auto& node : data["Nodes"]) {
        parse_plc_node(plc, server, &plc->node, node);
    }

    std::ifstream client_file(client_file_name);
    nlohmann::json clients_data = nlohmann::json::parse(client_file, nullptr, true, true);

    for (const auto& client_node : clients_data["Clients"]) {
        if (client_node["Name"] == plc->name && client_node.contains("UserNodes")) {
            for (const auto& user_node : client_node["UserNodes"]) {
                parse_plc_user_node(plc, server, &plc->node, user_node);
            }
        }
    }
}