#include "mcpp/model/response.h"

namespace mcpp::model {

// InitializeResponse::Result implementation
nlohmann::json InitializeResponse::Result::to_json() const {
    nlohmann::json j;
    j["protocolVersion"] = protocol_version;
    j["capabilities"] = capabilities;
    if (server_info) {
        j["serverInfo"] = nlohmann::json::parse(*server_info);
    }
    if (trace) {
        j["trace"] = *trace;
    }
    return j;
}

InitializeResponse::Result InitializeResponse::Result::from_json(const nlohmann::json& j) {
    Result result;
    result.protocol_version = j.at("protocolVersion").get<std::string>();
    result.capabilities = j.at("capabilities");
    if (j.contains("serverInfo")) {
        result.server_info = j["serverInfo"].dump();
    }
    if (j.contains("trace")) {
        result.trace = j["trace"];
    }
    return result;
}

// InitializeResponse implementation
InitializeResponse::InitializeResponse(const Result& result,
                                       const core::JsonRpcResponse::IdType& id)
    : result_(result), id_(id) {
}

std::unique_ptr<core::JsonRpcResponse> InitializeResponse::to_json_rpc_response() const {
    return std::make_unique<core::JsonRpcResponse>(id_, result_.to_json());
}

bool InitializeResponse::is_valid() const {
    return !result_.protocol_version.empty() && result_.capabilities.is_object();
}

// PingResponse implementation
PingResponse::PingResponse(const core::JsonRpcResponse::IdType& id)
    : id_(id) {
}

std::unique_ptr<core::JsonRpcResponse> PingResponse::to_json_rpc_response() const {
    return std::make_unique<core::JsonRpcResponse>(id_, nlohmann::json{});
}

// ListToolsResponse implementation
ListToolsResponse::ListToolsResponse(const nlohmann::json& tools,
                                     const core::JsonRpcResponse::IdType& id)
    : tools_(tools), id_(id) {
}

std::unique_ptr<core::JsonRpcResponse> ListToolsResponse::to_json_rpc_response() const {
    nlohmann::json result;
    result["tools"] = tools_;
    return std::make_unique<core::JsonRpcResponse>(id_, result);
}

bool ListToolsResponse::is_valid() const {
    return tools_.is_array();
}

// CallToolResponse implementation
CallToolResponse::CallToolResponse(const nlohmann::json& result,
                                   const core::JsonRpcResponse::IdType& id)
    : result_(result), id_(id) {
}

std::unique_ptr<core::JsonRpcResponse> CallToolResponse::to_json_rpc_response() const {
    return std::make_unique<core::JsonRpcResponse>(id_, result_);
}

bool CallToolResponse::is_valid() const {
    return result_.is_object();
}

} // namespace mcpp::model