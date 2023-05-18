#pragma once

#include <open62541/nodeids.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#define CHECK(func, message)                                                                 \
    do {                                                                                     \
        auto retval = (func);                                                                \
        if (retval != UA_STATUSCODE_GOOD) {                                                  \
            UA_ServerConfig_clean(config);                                                   \
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, #message ": %s (" #func ")", \
                        UA_StatusCode_name(retval));                                         \
            return EXIT_FAILURE;                                                             \
        }                                                                                    \
    } while (0)

static char locale[] = "en-US";

static std::mutex access_ua_server_mutex;

struct Client {
    std::string name;
    virtual ~Client() = default;

    virtual bool connected() {
        return false;
    }
};

template <typename Type>
struct Datatype {
    Type value;
    const UA_DataType* datatype;
};

template <>
struct Datatype<UA_String> {
    std::string raw_data;
    UA_String value;
    const UA_DataType* datatype;
    explicit Datatype(const char* value)
        : raw_data{value}, value{UA_STRING(raw_data.data())}, datatype{&UA_TYPES[UA_TYPES_STRING]} {}
};

template <>
struct Datatype<UA_Double> {
    UA_Double value;
    const UA_DataType* datatype;
    explicit Datatype(double value) : value{value}, datatype{&UA_TYPES[UA_TYPES_DOUBLE]} {}
};

template <>
struct Datatype<UA_Float> {
    UA_Float value;
    const UA_DataType* datatype;
    explicit Datatype(float value) : value{value}, datatype{&UA_TYPES[UA_TYPES_FLOAT]} {}
};

template <>
struct Datatype<UA_Int32> {
    UA_Int32 value;
    const UA_DataType* datatype;
    explicit Datatype(int32_t value) : value{value}, datatype{&UA_TYPES[UA_TYPES_INT32]} {}
};

template <>
struct Datatype<UA_Int64> {
    UA_Int64 value;
    const UA_DataType* datatype;
    explicit Datatype(int64_t value) : value{value}, datatype{&UA_TYPES[UA_TYPES_INT64]} {}
};

template <>
struct Datatype<UA_UInt32> {
    UA_UInt32 value;
    const UA_DataType* datatype;
    explicit Datatype(uint32_t value) : value{value}, datatype{&UA_TYPES[UA_TYPES_UINT32]} {}
};

template <>
struct Datatype<UA_UInt64> {
    UA_UInt64 value;
    const UA_DataType* datatype;
    explicit Datatype(uint64_t value) : value{value}, datatype{&UA_TYPES[UA_TYPES_UINT64]} {}
};

template <>
struct Datatype<UA_LocalizedText> {
    std::string raw_data;
    UA_LocalizedText value;
    const UA_DataType* datatype;
    explicit Datatype(std::string text)
        : raw_data{std::move(text)},
          value{UA_LOCALIZEDTEXT(locale, raw_data.data())},
          datatype{&UA_TYPES[UA_TYPES_STRING]} {}
};

template <>
struct Datatype<UA_EnumValueType> {
    std::string _enum_name;
    std::string enum_description;
    UA_EnumValueType value;
    const UA_DataType* datatype;
    Datatype(int64_t value, std::string enum_name)
        : _enum_name{std::move(enum_name)},
          enum_description{},
          value{value, UA_LOCALIZEDTEXT(locale, _enum_name.data()), UA_LOCALIZEDTEXT(locale, enum_description.data())},
          datatype{&UA_TYPES[UA_TYPES_ENUMVALUETYPE]} {}
};

using ReadCallback = UA_StatusCode (*)(UA_Server*, const UA_NodeId*, void*, const UA_NodeId*, void*, UA_Boolean,
                                       const UA_NumericRange*, UA_DataValue*);
using WriteCallback = UA_StatusCode (*)(UA_Server*, const UA_NodeId*, void*, const UA_NodeId*, void*,
                                        const UA_NumericRange*, const UA_DataValue*);

template <typename Type>
UA_NodeId addVariableNode(UA_Server* server, void* nodeContext, char* name, std::optional<UA_NodeId> parent,
                          std::optional<Datatype<Type>> value, std::optional<ReadCallback> read_callback,
                          std::optional<WriteCallback> write_callback, uint32_t count) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT(locale, name);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | (write_callback.has_value() ? UA_ACCESSLEVELMASK_WRITE : 0);

    UA_QualifiedName ua_name = UA_QUALIFIEDNAME(1u, name);
    UA_NodeId parentReferenceNodeId =
        UA_NODEID_NUMERIC(0u, parent.has_value() ? UA_NS0ID_HASCOMPONENT : UA_NS0ID_ORGANIZES);

    UA_NodeId variableTypeNodeId =
        UA_NODEID_NUMERIC(0u, value.has_value() ? UA_NS0ID_PROPERTYTYPE : UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource data_source{};
    if (read_callback.has_value()) {
        data_source.read = read_callback.value();
    }
    if (write_callback.has_value()) {
        data_source.write = write_callback.value();
    }
    if (!parent.has_value()) {
        parent = std::make_optional<UA_NodeId>(UA_NODEID_NUMERIC(0u, UA_NS0ID_OBJECTSFOLDER));
    }
    if (count != 0) {
        attr.arrayDimensions = &count;
        attr.arrayDimensionsSize = 1;
        attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
        attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    }

    std::scoped_lock<std::mutex> guard(access_ua_server_mutex);

    UA_NodeId out_node_id;
    if (value.has_value()) {
        // TODO: add support for array properties
        assert(count == 0 && "Properties with type array are not supported yet!");
        UA_Variant_setScalar(&attr.value, &value.value().value, value.value().datatype);
        UA_Server_addVariableNode(server, UA_NODEID_NULL, parent.value(), parentReferenceNodeId, ua_name,
                                  variableTypeNodeId, attr, nullptr, &out_node_id);
    } else {
        UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL, parent.value(), parentReferenceNodeId, ua_name,
                                            variableTypeNodeId, attr, data_source, nullptr, &out_node_id);
    }
    UA_Server_setNodeContext(server, out_node_id, nodeContext);

    return out_node_id;
}

inline UA_NodeId addObjectNode(UA_Server* server, char* name, std::optional<UA_NodeId> parent, bool folder = false) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT(locale, name);

    const UA_QualifiedName ua_name = UA_QUALIFIEDNAME(1u, name);
    const UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0u, UA_NS0ID_ORGANIZES);
    const UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0u, folder ? UA_NS0ID_FOLDERTYPE : UA_NS0ID_BASEOBJECTTYPE);

    if (!parent.has_value()) {
        parent = std::make_optional<UA_NodeId>(UA_NODEID_NUMERIC(0u, UA_NS0ID_OBJECTSFOLDER));
    }

    std::scoped_lock<std::mutex> guard(access_ua_server_mutex);

    UA_NodeId out_node_id;
    UA_Server_addObjectNode(server, UA_NODEID_NULL, parent.value(), parentReferenceNodeId, ua_name, variableTypeNodeId,
                            attr, nullptr, &out_node_id);
    return out_node_id;
}

inline UA_StatusCode delete_node(UA_Server* server, const UA_NodeId nodeId, UA_Boolean deleteReferences) {
    std::scoped_lock<std::mutex> guard(access_ua_server_mutex);

    return UA_Server_deleteNode(server, nodeId, deleteReferences);
}

static UA_INLINE UA_ByteString loadFile(const char* const path) {
    UA_ByteString fileContents = UA_STRING_NULL;

    /* Open the file */
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        errno = 0; /* We read errno also from the tcp layer... */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte*)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if (fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if (read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);

    return fileContents;
}

static UA_INLINE UA_StatusCode writeFile(const char* const path, const UA_ByteString buffer) {
    FILE* fp = NULL;

    fp = fopen(path, "wb");
    if (fp == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    for (UA_UInt32 bufIndex = 0; bufIndex < buffer.length; bufIndex++) {
        int retVal = fputc(buffer.data[bufIndex], fp);
        if (retVal == EOF) {
            fclose(fp);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    fclose(fp);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode addAllEndpoints(UA_ServerConfig* config, const nlohmann::basic_json<>& server_config,
                              bool populate_none_endpoint) {
    // populate endpoints
    // only populate none endpoint if populate_none_endpoint is true

    for (size_t i = 0; i < config->securityPoliciesSize; ++i) {
        if (UA_String_equal(&UA_SECURITY_POLICY_NONE_URI, &config->securityPolicies[i].policyUri) &&
            populate_none_endpoint) {
            UA_StatusCode retval =
                UA_ServerConfig_addEndpoint(config, config->securityPolicies[i].policyUri, UA_MESSAGESECURITYMODE_NONE);
            if (retval != UA_STATUSCODE_GOOD)
                return retval;
        } else if (!UA_String_equal(&UA_SECURITY_POLICY_NONE_URI, &config->securityPolicies[i].policyUri) &&
                   server_config.contains("SecurityModes")) {
            for (const auto& mode : server_config["SecurityModes"]) {
                const auto mode_name = mode.get<std::string>();

                if (mode_name == "Sign") {
                    UA_StatusCode retval = UA_ServerConfig_addEndpoint(config, config->securityPolicies[i].policyUri,
                                                                       UA_MESSAGESECURITYMODE_SIGN);
                    if (retval != UA_STATUSCODE_GOOD)
                        return retval;
                } else if (mode_name == "Sign & Encrypt") {
                    UA_StatusCode retval = UA_ServerConfig_addEndpoint(config, config->securityPolicies[i].policyUri,
                                                                       UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
                    if (retval != UA_STATUSCODE_GOOD)
                        return retval;
                }
            }
        }
    }

    return UA_STATUSCODE_GOOD;
}