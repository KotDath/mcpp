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

#include "mcpp/client/sampling.h"

#include <utility>

namespace mcpp::client {

// ============================================================================
// CreateMessageRequest
// ============================================================================

std::optional<CreateMessageRequest> CreateMessageRequest::from_json(const nlohmann::json& j) {
    CreateMessageRequest request;

    // Parse messages array (required)
    if (!j.contains("messages") || !j["messages"].is_array()) {
        return std::nullopt;
    }

    try {
        for (const auto& msg_json : j["messages"]) {
            SamplingMessage msg;

            // Parse role (required)
            if (!msg_json.contains("role") || !msg_json["role"].is_string()) {
                return std::nullopt;
            }
            msg.role = msg_json["role"].get<std::string>();

            // Parse content (required)
            if (!msg_json.contains("content")) {
                return std::nullopt;
            }

            // Content can be a string (for text content shorthand) or an object
            if (msg_json["content"].is_string()) {
                // Shorthand: content as string becomes TextContent
                TextContent text_content;
                text_content.type = "text";
                text_content.text = msg_json["content"].get<std::string>();
                msg.content = text_content;
            } else if (msg_json["content"].is_object()) {
                auto content = content_from_json(msg_json["content"]);
                if (!content) {
                    return std::nullopt;
                }
                msg.content = *content;
            } else {
                return std::nullopt;
            }

            request.messages.push_back(std::move(msg));
        }

        // Validate at least one message
        if (request.messages.empty()) {
            return std::nullopt;
        }

        // Parse max_tokens (required)
        if (!j.contains("maxTokens") || !j["maxTokens"].is_number_integer()) {
            return std::nullopt;
        }
        request.max_tokens = j["maxTokens"].get<int64_t>();
        if (request.max_tokens < 1) {
            return std::nullopt;
        }

        // Parse optional fields

        // model_preferences
        if (j.contains("modelPreferences") && j["modelPreferences"].is_object()) {
            const auto& prefs = j["modelPreferences"];
            ModelPreferences preferences;

            if (prefs.contains("costPriority") && prefs["costPriority"].is_number()) {
                preferences.cost_priority = prefs["costPriority"].get<double>();
            }
            if (prefs.contains("speedPriority") && prefs["speedPriority"].is_number()) {
                preferences.speed_priority = prefs["speedPriority"].get<double>();
            }
            if (prefs.contains("intelligencePriority") && prefs["intelligencePriority"].is_number()) {
                preferences.intelligence_priority = prefs["intelligencePriority"].get<double>();
            }

            request.model_preferences = preferences;
        }

        // system_prompt
        if (j.contains("systemPrompt") && j["systemPrompt"].is_string()) {
            request.system_prompt = j["systemPrompt"].get<std::string>();
        }

        // include_context
        if (j.contains("includeContext") && j["includeContext"].is_string()) {
            request.include_context = j["includeContext"].get<std::string>();
        }

        // temperature
        if (j.contains("temperature") && j["temperature"].is_number()) {
            request.temperature = j["temperature"].get<double>();
        }

        // stop_sequences
        if (j.contains("stopSequences") && j["stopSequences"].is_array()) {
            std::vector<std::string> sequences;
            for (const auto& seq : j["stopSequences"]) {
                if (seq.is_string()) {
                    sequences.push_back(seq.get<std::string>());
                }
            }
            request.stop_sequences = sequences;
        }

        // metadata (arbitrary JSON)
        if (j.contains("meta") && j["meta"].is_object()) {
            request.metadata = j["meta"];
        }

        return request;

    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// ============================================================================
// CreateMessageResult
// ============================================================================

nlohmann::json CreateMessageResult::to_json() const {
    nlohmann::json j;

    j["role"] = role;
    j["content"] = content_to_json(content);
    j["model"] = model;

    if (stop_reason.has_value()) {
        j["stopReason"] = *stop_reason;
    }

    return j;
}

// ============================================================================
// SamplingClient
// ============================================================================

void SamplingClient::set_sampling_handler(SamplingHandler handler) {
    sampling_handler_ = std::move(handler);
}

nlohmann::json SamplingClient::handle_create_message(const nlohmann::json& params) {
    // Parse the request
    auto request_opt = CreateMessageRequest::from_json(params);
    if (!request_opt) {
        // Parse error - return JSON-RPC error
        nlohmann::json error;
        error["code"] = -32700;  // Parse error
        error["message"] = "Failed to parse createMessage request";
        return error;
    }

    // Check if handler is registered
    if (!sampling_handler_) {
        // No handler - return method not found error
        nlohmann::json error;
        error["code"] = -32601;  // Method not found
        error["message"] = "No sampling handler registered";
        return error;
    }

    // Invoke the handler
    try {
        CreateMessageResult result = sampling_handler_(*request_opt);
        return result.to_json();
    } catch (const std::exception&) {
        // Handler threw exception - return internal error
        nlohmann::json error;
        error["code"] = -32603;  // Internal error
        error["message"] = "Sampling handler failed";
        return error;
    }
}

} // namespace mcpp::client
