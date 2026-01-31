#ifndef MCPP_CORE_JSON_RPC_H
#define MCPP_CORE_JSON_RPC_H

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

namespace mcpp::core {

// Type alias for JSON values - using nlohmann::json
using JsonValue = nlohmann::json;

// Request ID can be either a number or a string per JSON-RPC 2.0 spec
using RequestId = std::variant<int64_t, std::string>;

// Forward declaration for JsonRpcError (defined in error.h)
struct JsonRpcError;

/**
 * JSON-RPC 2.0 Request
 *
 * Represents a JSON-RPC request message with method name and optional parameters.
 * Requests always have an ID and expect a response.
 *
 * Example JSON:
 * {"jsonrpc": "2.0", "id": 1, "method": "subtract", "params": [42, 23]}
 */
struct JsonRpcRequest {
    std::string jsonrpc = "2.0";
    RequestId id;
    std::string method;
    JsonValue params = nullptr;

    /**
     * Serialize this request to a JSON value
     */
    JsonValue to_json() const {
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
        j["method"] = method;
        if (params != nullptr) {
            j["params"] = params;
        }
        return j;
    }

    /**
     * Serialize this request to a JSON string
     */
    std::string to_string() const {
        return to_json().dump();
    }
};

/**
 * JSON-RPC 2.0 Response
 *
 * Represents a JSON-RPC response message.
 * A response has either a result (success) or an error (failure), never both.
 *
 * Example JSON (success):
 * {"jsonrpc": "2.0", "result": 19, "id": 1}
 *
 * Example JSON (error):
 * {"jsonrpc": "2.0", "error": {"code": -32601, "message": "Method not found"}, "id": 1}
 */
struct JsonRpcResponse {
    std::string jsonrpc = "2.0";
    RequestId id;
    std::optional<JsonValue> result;
    std::optional<JsonRpcError> error;

    /**
     * Parse a JSON value into a JsonRpcResponse
     *
     * @param j JSON value to parse
     * @return Parsed response, or nullopt if parsing fails
     */
    static std::optional<JsonRpcResponse> from_json(const JsonValue& j);

    /**
     * Serialize this response to a JSON value
     */
    JsonValue to_json() const;

    /**
     * Serialize this response to a JSON string
     */
    std::string to_string() const {
        return to_json().dump();
    }

    /**
     * Check if this is an error response
     */
    bool is_error() const {
        return error.has_value();
    }

    /**
     * Check if this is a successful response
     */
    bool is_success() const {
        return result.has_value() && !error.has_value();
    }
};

/**
 * JSON-RPC 2.0 Notification
 *
 * Represents a JSON-RPC notification message.
 * Notifications are like requests but have no ID and do not expect a response.
 *
 * Example JSON:
 * {"jsonrpc": "2.0", "method": "update", "params": {"value": 42}}
 */
struct JsonRpcNotification {
    std::string jsonrpc = "2.0";
    std::string method;
    JsonValue params = nullptr;

    /**
     * Serialize this notification to a JSON value
     */
    JsonValue to_json() const {
        JsonValue j;
        j["jsonrpc"] = jsonrpc;
        j["method"] = method;
        if (params != nullptr) {
            j["params"] = params;
        }
        return j;
    }

    /**
     * Serialize this notification to a JSON string
     */
    std::string to_string() const {
        return to_json().dump();
    }
};

} // namespace mcpp::core

#endif // MCPP_CORE_JSON_RPC_H
