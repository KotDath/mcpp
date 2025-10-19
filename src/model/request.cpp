#include "mcpp/model/request.h"
#include <stdexcept>

namespace mcpp::model {

// InitializeRequest::Params implementation
nlohmann::json InitializeRequest::Params::to_json() const {
    nlohmann::json j;
    j["protocolVersion"] = protocol_version;
    j["capabilities"] = capabilities;
    if (client_info) {
        j["clientInfo"] = nlohmann::json::parse(*client_info);
    }
    if (trace) {
        j["trace"] = *trace;
    }
    return j;
}

InitializeRequest::Params InitializeRequest::Params::from_json(const nlohmann::json& j) {
    Params params;
    params.protocol_version = j.at("protocolVersion").get<std::string>();
    params.capabilities = j.at("capabilities");
    if (j.contains("clientInfo")) {
        params.client_info = j["clientInfo"].dump();
    }
    if (j.contains("trace")) {
        params.trace = j["trace"];
    }
    return params;
}

// InitializeRequest implementation
InitializeRequest::InitializeRequest(const Params& params)
    : params_(params) {
}

std::unique_ptr<core::JsonRpcRequest> InitializeRequest::to_json_rpc_request() const {
    return std::make_unique<core::JsonRpcRequest>(
        static_cast<int64_t>(1), // Use ID 1 for initialize
        get_method(),
        params_.to_json()
    );
}

bool InitializeRequest::is_valid() const {
    return !params_.protocol_version.empty() && params_.capabilities.is_object();
}

// PingRequest implementation
std::unique_ptr<core::JsonRpcRequest> PingRequest::to_json_rpc_request() const {
    return std::make_unique<core::JsonRpcRequest>(
        static_cast<int64_t>(2), // Use ID 2 for ping
        get_method(),
        nlohmann::json{}
    );
}

// ListToolsRequest implementation
std::unique_ptr<core::JsonRpcRequest> ListToolsRequest::to_json_rpc_request() const {
    return std::make_unique<core::JsonRpcRequest>(
        static_cast<int64_t>(3), // Use ID 3 for list tools
        get_method(),
        nlohmann::json{}
    );
}

// CallToolRequest implementation
CallToolRequest::CallToolRequest(const std::string& name, const nlohmann::json& arguments)
    : name_(name), arguments_(arguments) {
}

std::unique_ptr<core::JsonRpcRequest> CallToolRequest::to_json_rpc_request() const {
    nlohmann::json params;
    params["name"] = name_;
    if (!arguments_.is_null() && !arguments_.empty()) {
        params["arguments"] = arguments_;
    }

    return std::make_unique<core::JsonRpcRequest>(
        static_cast<int64_t>(4), // Use ID 4 for tool call
        get_method(),
        params
    );
}

bool CallToolRequest::is_valid() const {
    return !name_.empty();
}

} // namespace mcpp::model