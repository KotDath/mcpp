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

#include "mcpp/client/elicitation.h"

namespace mcpp::client {

// ============================================================================
// ElicitResult::to_json
// ============================================================================

JsonValue ElicitResult::to_json() const {
    JsonValue j;
    j["action"] = action;

    if (content) {
        JsonValue content_json;
        for (const auto& [key, value] : *content) {
            content_json[key] = std::visit([](const auto& v) -> JsonValue {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    return v;
                } else if constexpr (std::is_same_v<T, double>) {
                    return v;
                } else if constexpr (std::is_same_v<T, bool>) {
                    return v;
                } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                    return v;
                }
                return JsonValue();
            }, value);
        }
        j["content"] = content_json;
    }

    return j;
}

// ============================================================================
// ElicitationCompleteNotification::from_json
// ============================================================================

std::optional<ElicitationCompleteNotification> ElicitationCompleteNotification::from_json(
    const nlohmann::json& j
) {
    try {
        ElicitationCompleteNotification notification;

        // Parse required fields
        if (!j.contains("elicitation_id") || !j["elicitation_id"].is_string()) {
            return std::nullopt;
        }
        notification.elicitation_id = j["elicitation_id"].get<std::string>();

        if (!j.contains("action") || !j["action"].is_string()) {
            return std::nullopt;
        }
        notification.action = j["action"].get<std::string>();

        // Parse optional content
        if (j.contains("content")) {
            notification.content = j["content"];
        }

        return notification;

    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// ============================================================================
// ElicitationClient
// ============================================================================

void ElicitationClient::set_elicitation_handler(ElicitationHandler handler) {
    handler_ = std::move(handler);
}

namespace {

// Helper to parse PrimitiveSchema from JSON
std::optional<PrimitiveSchema> parse_primitive_schema(const nlohmann::json& j) {
    try {
        PrimitiveSchema schema;

        // Required: type
        if (!j.contains("type") || !j["type"].is_string()) {
            return std::nullopt;
        }
        schema.type = j["type"].get<std::string>();

        // Optional fields
        if (j.contains("title") && j["title"].is_string()) {
            schema.title = j["title"].get<std::string>();
        }
        if (j.contains("description") && j["description"].is_string()) {
            schema.description = j["description"].get<std::string>();
        }
        if (j.contains("default") && j["default"].is_string()) {
            schema.default_value = j["default"].get<std::string>();
        }

        // String type constraints
        if (j.contains("pattern") && j["pattern"].is_string()) {
            schema.pattern = j["pattern"].get<std::string>();
        }
        if (j.contains("minLength") && j["minLength"].is_number_unsigned()) {
            schema.min_length = j["minLength"].get<size_t>();
        }
        if (j.contains("maxLength") && j["maxLength"].is_number_unsigned()) {
            schema.max_length = j["maxLength"].get<size_t>();
        }

        // Number/integer type constraints
        if (j.contains("minimum") && j["minimum"].is_number()) {
            schema.minimum = j["minimum"].get<double>();
        }
        if (j.contains("maximum") && j["maximum"].is_number()) {
            schema.maximum = j["maximum"].get<double>();
        }

        // Array type constraints (enum-based select)
        if (j.contains("enum") && j["enum"].is_array()) {
            std::vector<std::string> enum_values;
            for (const auto& e : j["enum"]) {
                if (e.is_string()) {
                    enum_values.push_back(e.get<std::string>());
                }
            }
            schema.enum_values = std::move(enum_values);
        }
        if (j.contains("multiselect") && j["multiselect"].is_boolean()) {
            schema.multiselect = j["multiselect"].get<bool>();
        }

        return schema;

    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// Helper to parse ElicitRequestForm from JSON
std::optional<ElicitRequestForm> parse_elicitation_request_form(const nlohmann::json& j) {
    try {
        ElicitRequestForm form;

        // Required: message
        if (!j.contains("message") || !j["message"].is_string()) {
            return std::nullopt;
        }
        form.message = j["message"].get<std::string>();

        // Mode is "form" by default
        if (j.contains("mode") && j["mode"].is_string()) {
            form.mode = j["mode"].get<std::string>();
        }

        // Required: requested_schema (object of field names -> PrimitiveSchema)
        if (!j.contains("requested_schema") || !j["requested_schema"].is_object()) {
            return std::nullopt;
        }

        const auto& schema_obj = j["requested_schema"];
        for (auto it = schema_obj.begin(); it != schema_obj.end(); ++it) {
            const std::string& field_name = it.key();
            auto schema = parse_primitive_schema(it.value());
            if (!schema) {
                return std::nullopt;  // Invalid schema for this field
            }
            form.requested_schema[field_name] = *schema;
        }

        // Optional: required (array of field names)
        if (j.contains("required") && j["required"].is_array()) {
            std::vector<std::string> required;
            for (const auto& r : j["required"]) {
                if (r.is_string()) {
                    required.push_back(r.get<std::string>());
                }
            }
            if (!required.empty()) {
                form.required = std::move(required);
            }
        }

        return form;

    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// Helper to parse ElicitRequestURL from JSON
std::optional<ElicitRequestURL> parse_elicitation_request_url(const nlohmann::json& j) {
    try {
        ElicitRequestURL url_req;

        // Required: message
        if (!j.contains("message") || !j["message"].is_string()) {
            return std::nullopt;
        }
        url_req.message = j["message"].get<std::string>();

        // Mode is "url" by default
        if (j.contains("mode") && j["mode"].is_string()) {
            url_req.mode = j["mode"].get<std::string>();
        }

        // Required: elicitation_id
        if (!j.contains("elicitation_id") || !j["elicitation_id"].is_string()) {
            return std::nullopt;
        }
        url_req.elicitation_id = j["elicitation_id"].get<std::string>();

        // Required: url
        if (!j.contains("url") || !j["url"].is_string()) {
            return std::nullopt;
        }
        url_req.url = j["url"].get<std::string>();

        // Optional: confirm_url
        if (j.contains("confirm_url") && j["confirm_url"].is_string()) {
            url_req.confirm_url = j["confirm_url"].get<std::string>();
        }

        return url_req;

    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// Helper to create a JSON-RPC error response
nlohmann::json make_error(int code, const std::string& message) {
    nlohmann::json j;
    j["code"] = code;
    j["message"] = message;
    return j;
}

} // namespace

nlohmann::json ElicitationClient::handle_elicitation_create(const nlohmann::json& params) {
    // Check if handler is registered
    if (!handler_) {
        return make_error(-32603, "No elicitation handler registered");
    }

    // Parse mode to determine request type
    std::string mode;
    if (params.contains("mode") && params["mode"].is_string()) {
        mode = params["mode"].get<std::string>();
    }

    try {
        if (mode == "form" || mode.empty()) {
            // Form mode - synchronous
            auto form_opt = parse_elicitation_request_form(params);
            if (!form_opt) {
                return make_error(-32602, "Invalid elicitation form request");
            }

            // Invoke handler and return result immediately
            ElicitResult result = handler_(*form_opt);
            return result.to_json();

        } else if (mode == "url") {
            // URL mode - asynchronous
            auto url_req_opt = parse_elicitation_request_url(params);
            if (!url_req_opt) {
                return make_error(-32602, "Invalid elicitation URL request");
            }

            const std::string elicitation_id = url_req_opt->elicitation_id;

            // Call handler to initiate URL mode (open browser, show message)
            // Handler returns immediately; actual completion comes later via notification
            ElicitResult initial_result = handler_(*url_req_opt);

            // For URL mode, we acknowledge the request was received
            // The actual result comes via elicitation/complete notification
            nlohmann::json response;
            response["status"] = "pending";
            response["elicitation_id"] = elicitation_id;
            response["action"] = initial_result.action;

            return response;

        } else {
            return make_error(-32602, "Unknown elicitation mode: " + mode);
        }

    } catch (const std::exception& e) {
        return make_error(-32603, std::string("Elicitation handler error: ") + e.what());
    }
}

void ElicitationClient::handle_elicitation_complete(const nlohmann::json& params) {
    // Parse the notification
    auto complete_opt = ElicitationCompleteNotification::from_json(params);
    if (!complete_opt) {
        return;  // Invalid notification, ignore
    }

    const std::string& elicitation_id = complete_opt->elicitation_id;

    // Look up pending request by elicitation_id
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pending_url_requests_.find(elicitation_id);
    if (it != pending_url_requests_.end()) {
        // Invoke the stored completion callback
        ElicitResult result;
        result.action = complete_opt->action;
        if (complete_opt->content) {
            // Convert content to variant map if present
            std::map<std::string, std::variant<std::string, double, bool, std::vector<std::string>>> content_map;
            // For now, we store as raw JSON; user can process it as needed
            // Full parsing would depend on the expected content structure
        }
        it->second(result);

        // Remove from pending map
        pending_url_requests_.erase(it);
    }
    // If no pending request found, silently ignore (may have timed out)
}

} // namespace mcpp::client
