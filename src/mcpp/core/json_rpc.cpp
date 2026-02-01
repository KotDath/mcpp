// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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

// JsonRpcRequest implementation

RequestId JsonRpcRequest::extract_request_id(std::string_view raw_json) {
    // Try to find "id" field using string search (works even if JSON is malformed)
    size_t id_pos = raw_json.find("\"id\"");
    if (id_pos == std::string_view::npos) {
        return RequestId(); // null ID
    }

    // Find the colon after "id"
    size_t colon_pos = raw_json.find(':', id_pos);
    if (colon_pos == std::string_view::npos) {
        return RequestId(); // null ID
    }

    // Skip whitespace after colon
    size_t value_start = colon_pos + 1;
    while (value_start < raw_json.size() && std::isspace(static_cast<unsigned char>(raw_json[value_start]))) {
        ++value_start;
    }

    if (value_start >= raw_json.size()) {
        return RequestId(); // null ID
    }

    // Extract the ID value based on type
    char first_char = raw_json[value_start];

    // Null ID: "id": null
    if (first_char == 'n') {
        // Check for "null"
        if (raw_json.substr(value_start, 4) == "null") {
            return RequestId(); // null ID
        }
        return RequestId(); // malformed, return null
    }

    // String ID: "id": "value"
    if (first_char == '"') {
        size_t closing_quote = raw_json.find('"', value_start + 1);
        if (closing_quote != std::string_view::npos) {
            std::string id_str(raw_json.substr(value_start + 1, closing_quote - value_start - 1));
            return RequestId(id_str);
        }
        return RequestId(); // malformed string, return null
    }

    // Numeric ID: "id": 42
    if (first_char == '-' || (first_char >= '0' && first_char <= '9')) {
        std::string num_str;
        size_t num_end = value_start;

        // Extract digits (handle negative numbers)
        while (num_end < raw_json.size()) {
            char c = raw_json[num_end];
            if (c == '-' || (c >= '0' && c <= '9')) {
                num_str += c;
                ++num_end;
            } else {
                break;
            }
        }

        if (!num_str.empty()) {
            try {
                int64_t id_val = std::stoll(num_str);
                return RequestId(id_val);
            } catch (...) {
                return RequestId(); // parse failed, return null
            }
        }
    }

    return RequestId(); // unhandled format, return null
}

std::optional<JsonRpcRequest> JsonRpcRequest::from_json(const JsonValue& j) {
    try {
        // Check for jsonrpc field
        if (!j.contains("jsonrpc") || !j["jsonrpc"].is_string()) {
            return std::nullopt;
        }
        std::string jsonrpc_version = j["jsonrpc"].get<std::string>();
        if (jsonrpc_version != "2.0") {
            return std::nullopt;
        }

        // Check for id field (required for requests)
        if (!j.contains("id")) {
            return std::nullopt;
        }
        auto id_opt = detail::parse_request_id(j["id"]);
        if (!id_opt) {
            return std::nullopt;
        }

        // Check for method field
        if (!j.contains("method") || !j["method"].is_string()) {
            return std::nullopt;
        }

        JsonRpcRequest request;
        request.jsonrpc = jsonrpc_version;
        request.id = *id_opt;
        request.method = j["method"].get<std::string>();

        // Check for params (optional, but must be object or array if present)
        if (j.contains("params")) {
            const JsonValue& params = j["params"];
            if (!params.is_object() && !params.is_array()) {
                return std::nullopt;
            }
            request.params = params;
        }

        return request;

    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

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
