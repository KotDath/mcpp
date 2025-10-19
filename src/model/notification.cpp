#include "mcpp/model/notification.h"
#include "mcpp/core/json_rpc_message.h"

namespace mcpp::model {

// InitializedNotification implementation
std::unique_ptr<core::JsonRpcNotification> InitializedNotification::to_json_rpc_notification() const {
    return std::make_unique<core::JsonRpcNotification>(get_method(), nlohmann::json{});
}

// CancelledNotification implementation
CancelledNotification::CancelledNotification(core::JsonRpcRequest::IdType request_id)
    : request_id_(request_id) {
}

std::unique_ptr<core::JsonRpcNotification> CancelledNotification::to_json_rpc_notification() const {
    nlohmann::json params;
    params["requestId"] = core::id_to_json(request_id_);
    return std::make_unique<core::JsonRpcNotification>(get_method(), params);
}

bool CancelledNotification::is_valid() const {
    return true; // All request IDs are valid for cancellation
}

// ProgressNotification::Params implementation
nlohmann::json ProgressNotification::Params::to_json() const {
    nlohmann::json j;
    j["requestId"] = core::id_to_json(request_id);
    j["progress"] = progress;
    if (total) {
        j["total"] = *total;
    }
    return j;
}

ProgressNotification::Params ProgressNotification::Params::from_json(const nlohmann::json& j) {
    Params params;

    // Convert JSON to variant ID type using helper function
    params.request_id = core::json_to_id(j.at("requestId"));

    params.progress = j.at("progress");
    if (j.contains("total")) {
        params.total = j["total"];
    }
    return params;
}

// ProgressNotification implementation
ProgressNotification::ProgressNotification(const Params& params)
    : params_(params) {
}

std::unique_ptr<core::JsonRpcNotification> ProgressNotification::to_json_rpc_notification() const {
    return std::make_unique<core::JsonRpcNotification>(get_method(), params_.to_json());
}

bool ProgressNotification::is_valid() const {
    return params_.progress.is_object() || params_.progress.is_number();
}

// MessageNotification::Params implementation
std::string MessageNotification::Params::level_to_string(Level level) {
    switch (level) {
        case Level::Debug: return "debug";
        case Level::Info: return "info";
        case Level::Warning: return "warning";
        case Level::Error: return "error";
        default: return "info";
    }
}

MessageNotification::Level MessageNotification::Params::string_to_level(const std::string& str) {
    if (str == "debug") return Level::Debug;
    if (str == "info") return Level::Info;
    if (str == "warning") return Level::Warning;
    if (str == "error") return Level::Error;
    return Level::Info; // Default
}

nlohmann::json MessageNotification::Params::to_json() const {
    nlohmann::json j;
    j["level"] = level_to_string(level);
    j["message"] = message;
    if (logger) {
        j["logger"] = *logger;
    }
    return j;
}

MessageNotification::Params MessageNotification::Params::from_json(const nlohmann::json& j) {
    Params params;
    params.level = string_to_level(j.at("level").get<std::string>());
    params.message = j.at("message").get<std::string>();
    if (j.contains("logger")) {
        params.logger = j["logger"];
    }
    return params;
}

// MessageNotification implementation
MessageNotification::MessageNotification(const Params& params)
    : params_(params) {
}

std::unique_ptr<core::JsonRpcNotification> MessageNotification::to_json_rpc_notification() const {
    return std::make_unique<core::JsonRpcNotification>(get_method(), params_.to_json());
}

bool MessageNotification::is_valid() const {
    return !params_.message.empty();
}

} // namespace mcpp::model