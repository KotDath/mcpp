#include "json_rpc.h"
#include "error.h"

#include <optional>

namespace mcpp::core {

namespace detail {

// Helper to parse RequestId from JSON
// Per JSON-RPC 2.0 spec, id can be a number, string, or null (for error responses)
std::optional<RequestId> parse_request_id(const JsonValue& j) {
    if (j.is_number_integer()) {
        return j.get<int64_t>();
    } else if (j.is_string()) {
        return j.get<std::string>();
    } else if (j.is_null()) {
        // null ID is used in error responses when the request ID couldn't be determined
        // We use 0 as a sentinel value for null IDs
        return static_cast<int64_t>(0);
    }
    return std::nullopt;
}

} // namespace detail

// JsonRpcResponse implementation

std::optional<JsonRpcResponse> JsonRpcResponse::from_json(const JsonValue& j) {
    try {
        // Check for jsonrpc field
        if (!j.contains("jsonrpc") || !j["jsonrpc"].is_string()) {
            return std::nullopt;
        }
        std::string jsonrpc_version = j["jsonrpc"].get<std::string>();
        if (jsonrpc_version != "2.0") {
            return std::nullopt;
        }

        // Check for id field
        if (!j.contains("id")) {
            return std::nullopt;
        }
        auto id_opt = detail::parse_request_id(j["id"]);
        if (!id_opt) {
            return std::nullopt;
        }

        JsonRpcResponse response;
        response.jsonrpc = jsonrpc_version;
        response.id = *id_opt;

        // Check for result OR error (mutually exclusive per spec)
        bool has_result = j.contains("result");
        bool has_error = j.contains("error");

        if (has_result && has_error) {
            // Both present - invalid per spec
            return std::nullopt;
        }

        if (has_result) {
            response.result = j["result"];
        } else if (has_error) {
            // Parse error object
            const JsonValue& error_json = j["error"];
            if (!error_json.is_object()) {
                return std::nullopt;
            }

            // Use JsonRpcError::from_json if available
            auto error_opt = JsonRpcError::from_json(error_json);
            if (!error_opt) {
                return std::nullopt;
            }
            response.error = *error_opt;
        } else {
            // Neither result nor error present - invalid response
            return std::nullopt;
        }

        return response;

    } catch (const nlohmann::json::exception&) {
        // JSON parsing error - return nullopt
        return std::nullopt;
    } catch (const std::exception&) {
        // Other exception - return nullopt
        return std::nullopt;
    }
}

JsonValue JsonRpcResponse::to_json() const {
    JsonValue j;
    j["jsonrpc"] = jsonrpc;
    j["id"] = std::visit([](auto&& v) -> JsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int64_t>) {
            return v;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return v;
        }
        return JsonValue();
    }, id);

    if (error.has_value()) {
        j["error"] = error->to_json();
    } else if (result.has_value()) {
        j["result"] = *result;
    } else {
        // Should not happen - response must have either result or error
        j["result"] = nullptr;
    }

    return j;
}

// JsonRpcError implementation

std::optional<JsonRpcError> JsonRpcError::from_json(const JsonValue& j) {
    try {
        // Check for required fields
        if (!j.contains("code") || !j["code"].is_number()) {
            return std::nullopt;
        }
        if (!j.contains("message") || !j["message"].is_string()) {
            return std::nullopt;
        }

        JsonRpcError error;
        error.code = j["code"].get<int>();
        error.message = j["message"].get<std::string>();

        // Optional data field
        if (j.contains("data")) {
            error.data = j["data"];
        }

        return error;

    } catch (const nlohmann::json::exception&) {
        // JSON parsing error - return nullopt
        return std::nullopt;
    } catch (const std::exception&) {
        // Other exception - return nullopt
        return std::nullopt;
    }
}

} // namespace mcpp::core
