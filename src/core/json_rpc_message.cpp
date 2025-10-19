#include "mcpp/core/json_rpc_message.h"
#include <stdexcept>

namespace mcpp::core {

// JsonRpcError implementation
nlohmann::json JsonRpcError::to_json() const {
    nlohmann::json j;
    j["code"] = static_cast<int>(code);
    j["message"] = message;
    if (data) {
        j["data"] = *data;
    }
    return j;
}

JsonRpcError JsonRpcError::from_json(const nlohmann::json& j) {
    JsonRpcError error;
    error.code = static_cast<JsonRpcErrorCode>(j.at("code").get<int>());
    error.message = j.at("message").get<std::string>();
    if (j.contains("data")) {
        error.data = j["data"];
    }
    return error;
}

// JsonRpcMessage base implementation
bool JsonRpcMessage::validate_base(const nlohmann::json& j) const {
    if (!j.contains("jsonrpc") || j["jsonrpc"] != JSON_RPC_VERSION) {
        return false;
    }
    return true;
}

// Helper function to convert JSON to IdType
JsonRpcRequest::IdType json_to_id(const nlohmann::json& j) {
    if (j.is_null()) {
        return nullptr;
    } else if (j.is_string()) {
        return j.get<std::string>();
    } else if (j.is_number_integer()) {
        return j.get<int64_t>();
    }
    throw std::invalid_argument("Invalid ID type");
}

// Helper function to convert IdType to JSON
nlohmann::json id_to_json(const JsonRpcRequest::IdType& id) {
    if (std::holds_alternative<std::nullptr_t>(id)) {
        return nullptr;
    } else if (std::holds_alternative<std::string>(id)) {
        return std::get<std::string>(id);
    } else if (std::holds_alternative<int64_t>(id)) {
        return std::get<int64_t>(id);
    }
    return nullptr;
}

// Helper function to convert JSON to Response IdType
JsonRpcResponse::IdType json_to_response_id(const nlohmann::json& j) {
    if (j.is_string()) {
        return j.get<std::string>();
    } else if (j.is_number_integer()) {
        return j.get<int64_t>();
    }
    throw std::invalid_argument("Invalid Response ID type");
}

// Helper function to convert Response IdType to JSON
nlohmann::json response_id_to_json(const JsonRpcResponse::IdType& id) {
    if (std::holds_alternative<std::string>(id)) {
        return std::get<std::string>(id);
    } else if (std::holds_alternative<int64_t>(id)) {
        return std::get<int64_t>(id);
    }
    throw std::invalid_argument("Invalid Response ID type");
}

std::unique_ptr<JsonRpcMessage> JsonRpcMessage::from_json(const nlohmann::json& j) {
    if (!j.contains("jsonrpc") || j["jsonrpc"] != JSON_RPC_VERSION) {
        throw std::invalid_argument("Invalid JSON-RPC version");
    }

    if (j.contains("id")) {
        if (j.contains("error")) {
            // Error response
            auto error = JsonRpcError::from_json(j["error"]);
            return std::make_unique<JsonRpcResponse>(
                json_to_response_id(j["id"]), error);
        } else if (j.contains("result")) {
            // Success response
            return std::make_unique<JsonRpcResponse>(
                json_to_response_id(j["id"]), j["result"]);
        } else if (j.contains("method")) {
            // Request (has id and method)
            JsonRpcRequest::IdType id = json_to_id(j["id"]);
            nlohmann::json params = j.contains("params") ? j["params"] : nlohmann::json{};
            return std::make_unique<JsonRpcRequest>(
                id, j["method"].get<std::string>(), params);
        } else {
            throw std::invalid_argument("Invalid JSON-RPC message with ID");
        }
    } else if (j.contains("method")) {
        // Notification (no id, has method)
        nlohmann::json params = j.contains("params") ? j["params"] : nlohmann::json{};
        return std::make_unique<JsonRpcNotification>(
            j["method"].get<std::string>(), params);
    } else {
        throw std::invalid_argument("Invalid JSON-RPC message structure");
    }
}

// JsonRpcRequest implementation
JsonRpcRequest::JsonRpcRequest(const IdType& id, const std::string& method,
                               const nlohmann::json& params)
    : id_(id), method_(method), params_(params) {
}

nlohmann::json JsonRpcRequest::to_json() const {
    nlohmann::json j;
    j["jsonrpc"] = JSON_RPC_VERSION;
    j["id"] = id_to_json(id_);
    j["method"] = method_;
    if (!params_.is_null() && !params_.empty()) {
        j["params"] = params_;
    }
    return j;
}

bool JsonRpcRequest::is_valid() const {
    if (method_.empty()) {
        return false;
    }

    // ID should not be null for requests
    if (std::holds_alternative<std::nullptr_t>(id_)) {
        return false;
    }

    return true;
}

// JsonRpcResponse implementation
JsonRpcResponse::JsonRpcResponse(const IdType& id, const nlohmann::json& result)
    : id_(id), result_(result) {
}

JsonRpcResponse::JsonRpcResponse(const IdType& id, const JsonRpcError& error)
    : id_(id), result_(nlohmann::json{}), error_(error) {
}

nlohmann::json JsonRpcResponse::to_json() const {
    nlohmann::json j;
    j["jsonrpc"] = JSON_RPC_VERSION;
    j["id"] = response_id_to_json(id_);

    if (error_) {
        j["error"] = error_->to_json();
    } else {
        j["result"] = result_;
    }

    return j;
}

bool JsonRpcResponse::is_valid() const {
    if (error_) {
        return true; // Error response is valid if error is present
    }
    return true; // Success response is always valid
}

// JsonRpcNotification implementation
JsonRpcNotification::JsonRpcNotification(const std::string& method,
                                         const nlohmann::json& params)
    : method_(method), params_(params) {
}

nlohmann::json JsonRpcNotification::to_json() const {
    nlohmann::json j;
    j["jsonrpc"] = JSON_RPC_VERSION;
    j["method"] = method_;
    if (!params_.is_null() && !params_.empty()) {
        j["params"] = params_;
    }
    return j;
}

bool JsonRpcNotification::is_valid() const {
    return !method_.empty();
}

} // namespace mcpp::core