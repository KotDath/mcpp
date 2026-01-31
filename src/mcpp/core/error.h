#ifndef MCPP_CORE_ERROR_H
#define MCPP_CORE_ERROR_H

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace mcpp::core {

// Type alias for JSON values
using JsonValue = nlohmann::json;

// Standard JSON-RPC 2.0 error codes (from spec)
// https://www.jsonrpc.org/specification

/**
 * Invalid JSON was received by the server.
 * An error occurred on the server while parsing the JSON text.
 */
constexpr int PARSE_ERROR = -32700;

/**
 * The JSON sent is not a valid Request object.
 */
constexpr int INVALID_REQUEST = -32600;

/**
 * The method does not exist / is not available.
 */
constexpr int METHOD_NOT_FOUND = -32601;

/**
 * Invalid method parameter(s).
 */
constexpr int INVALID_PARAMS = -32602;

/**
 * Internal JSON-RPC error.
 */
constexpr int INTERNAL_ERROR = -32603;

/**
 * JSON-RPC 2.0 Error object
 *
 * Represents an error response in JSON-RPC.
 * Per the spec, errors always have a code and message,
 * and may optionally include additional data.
 *
 * Example JSON:
 * {"code": -32601, "message": "Method not found", "data": "available_methods: [foo, bar]"}
 */
struct JsonRpcError {
    int code;
    std::string message;
    std::optional<JsonValue> data;

    /**
     * Serialize this error to a JSON value
     */
    JsonValue to_json() const {
        JsonValue j;
        j["code"] = code;
        j["message"] = message;
        if (data.has_value()) {
            j["data"] = *data;
        }
        return j;
    }

    /**
     * Serialize this error to a JSON string
     */
    std::string to_string() const {
        return to_json().dump();
    }

    /**
     * Parse a JSON value into a JsonRpcError
     *
     * @param j JSON value to parse
     * @return Parsed error, or nullopt if parsing fails
     */
    static std::optional<JsonRpcError> from_json(const JsonValue& j);

    /**
     * Create a standard JSON-RPC error
     */
    static JsonRpcError parse_error(const std::string& details = "") {
        JsonRpcError e{PARSE_ERROR, "Parse error"};
        if (!details.empty()) {
            e.data = details;
        }
        return e;
    }

    static JsonRpcError invalid_request(const std::string& details = "") {
        JsonRpcError e{INVALID_REQUEST, "Invalid Request"};
        if (!details.empty()) {
            e.data = details;
        }
        return e;
    }

    static JsonRpcError method_not_found(const std::string& method = "") {
        JsonRpcError e{METHOD_NOT_FOUND, "Method not found"};
        if (!method.empty()) {
            e.data = "Method: " + method;
        }
        return e;
    }

    static JsonRpcError invalid_params(const std::string& details = "") {
        JsonRpcError e{INVALID_PARAMS, "Invalid params"};
        if (!details.empty()) {
            e.data = details;
        }
        return e;
    }

    static JsonRpcError internal_error(const std::string& details = "") {
        JsonRpcError e{INTERNAL_ERROR, "Internal error"};
        if (!details.empty()) {
            e.data = details;
        }
        return e;
    }
};

} // namespace mcpp::core

#endif // MCPP_CORE_ERROR_H
