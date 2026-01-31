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

#include "mcpp/content/content.h"

#include <stdexcept>
#include <utility>
#include <vector>

namespace mcpp::client {

namespace {

// Helper to check if a ContentBlock is ToolUseContent
bool is_tool_use(const ContentBlock& block) {
    return std::holds_alternative<ToolUseContent>(block);
}

// Helper to check if content contains tool use
bool contains_tool_use(const ContentBlock& content) {
    return is_tool_use(content);
}

// Helper to check if result content blocks contain tool use
bool contains_tool_use(const std::vector<ContentBlock>& blocks) {
    for (const auto& block : blocks) {
        if (is_tool_use(block)) {
            return true;
        }
    }
    return false;
}

// Helper to extract tool uses from content blocks
std::vector<ToolUseContent> extract_tool_uses(const std::vector<ContentBlock>& blocks) {
    std::vector<ToolUseContent> tool_uses;
    for (const auto& block : blocks) {
        if (std::holds_alternative<ToolUseContent>(block)) {
            tool_uses.push_back(std::get<ToolUseContent>(block));
        }
    }
    return tool_uses;
}

// Helper to extract tool uses from single content block
std::vector<ToolUseContent> extract_tool_uses(const ContentBlock& content) {
    if (is_tool_use(content)) {
        return {std::get<ToolUseContent>(content)};
    }
    return {};
}

// Helper to convert ToolChoice to JSON
nlohmann::json tool_choice_to_json(const ToolChoice& choice) {
    return std::visit([](const auto& c) -> nlohmann::json {
        nlohmann::json j;
        j["type"] = c.type;
        if constexpr (std::is_same_v<std::decay_t<decltype(c)>, ToolChoiceTool>) {
            j["name"] = c.name;
        }
        return j;
    }, choice);
}

} // namespace

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

            // Content can be:
            // 1. A string (for text content shorthand)
            // 2. A single object
            // 3. An array of content blocks
            if (msg_json["content"].is_string()) {
                // Shorthand: content as string becomes TextContent
                TextContent text_content;
                text_content.type = "text";
                text_content.text = msg_json["content"].get<std::string>();
                msg.content = text_content;
            } else if (msg_json["content"].is_array()) {
                // Array of content blocks
                std::vector<ContentBlock> blocks;
                for (const auto& block_json : msg_json["content"]) {
                    auto block = content_from_json(block_json);
                    if (!block) {
                        return std::nullopt;
                    }
                    blocks.push_back(*block);
                }
                msg.content_blocks = blocks;
                // Set single content to first block for compatibility
                if (!blocks.empty()) {
                    msg.content = blocks[0];
                }
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

        // Parse tools (optional)
        if (j.contains("tools") && j["tools"].is_array()) {
            std::vector<Tool> tools;
            for (const auto& tool_json : j["tools"]) {
                Tool tool;
                if (tool_json.contains("name") && tool_json["name"].is_string()) {
                    tool.name = tool_json["name"].get<std::string>();
                }
                if (tool_json.contains("inputSchema")) {
                    tool.input_schema = tool_json["inputSchema"];
                }
                tools.push_back(tool);
            }
            request.tools = tools;
        }

        // Parse tool_choice (optional)
        if (j.contains("toolChoice")) {
            const auto& tc = j["toolChoice"];
            if (tc.is_object() && tc.contains("type") && tc["type"].is_string()) {
                std::string type = tc["type"].get<std::string>();
                if (type == "auto") {
                    request.tool_choice = ToolChoiceAuto{};
                } else if (type == "required") {
                    request.tool_choice = ToolChoiceRequired{};
                } else if (type == "none") {
                    request.tool_choice = ToolChoiceNone{};
                } else if (type == "tool") {
                    ToolChoiceTool tool_choice;
                    tool_choice.type = "tool";
                    if (tc.contains("name") && tc["name"].is_string()) {
                        tool_choice.name = tc["name"].get<std::string>();
                    }
                    request.tool_choice = tool_choice;
                }
            }
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

    // If we have multiple content blocks, serialize as array
    if (!content_blocks.empty()) {
        nlohmann::json::array_t content_array;
        for (const auto& block : content_blocks) {
            content_array.push_back(content_to_json(block));
        }
        j["content"] = content_array;
    } else {
        j["content"] = content_to_json(content);
    }

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

void SamplingClient::set_tool_loop_config(ToolLoopConfig config) {
    config_ = config;
}

void SamplingClient::set_tool_caller(std::function<nlohmann::json(std::string_view, const nlohmann::json&)> caller) {
    tool_caller_ = std::move(caller);
}

void SamplingClient::clear_tool_caller() {
    tool_caller_ = nullptr;
}

namespace {

// Helper to execute a single tool call
ToolResultContent call_tool(
    const ToolUseContent& tool_use,
    std::function<nlohmann::json(std::string_view, const nlohmann::json&)> tool_caller
) {
    ToolResultContent result;
    result.type = "tool_result";
    result.tool_use_id = tool_use.id;

    if (!tool_caller) {
        result.is_error = true;
        result.content = "No tool caller registered";
        return result;
    }

    try {
        // Build the tools/call request
        nlohmann::json call_params;
        call_params["name"] = tool_use.name;
        call_params["arguments"] = tool_use.arguments;

        // Call the tool
        nlohmann::json response = tool_caller("tools/call", call_params);

        // Extract content from response
        // MCP tools/call returns: { content: [{ type: "text", text: "..." }] }
        if (response.contains("content") && response["content"].is_array()) {
            const auto& content_array = response["content"];
            std::string combined_content;
            for (const auto& item : content_array) {
                if (item.contains("type") && item["type"] == "text" &&
                    item.contains("text") && item["text"].is_string()) {
                    if (!combined_content.empty()) {
                        combined_content += "\n";
                    }
                    combined_content += item["text"].get<std::string>();
                }
            }
            result.content = combined_content;

            // Check for error flag
            if (response.contains("isError") && response["isError"].is_boolean()) {
                result.is_error = response["isError"].get<bool>();
            }
        } else if (response.contains("error")) {
            result.is_error = true;
            if (response["error"].is_string()) {
                result.content = response["error"].get<std::string>();
            } else {
                result.content = response["error"].dump();
            }
        } else {
            result.content = response.dump();
        }
    } catch (const std::exception& e) {
        result.is_error = true;
        result.content = std::string("Tool call failed: ") + e.what();
    }

    return result;
}

// Execute the tool loop
CreateMessageResult execute_tool_loop(
    const CreateMessageRequest& original_request,
    SamplingHandler handler,
    const ToolLoopConfig& config,
    std::function<nlohmann::json(std::string_view, const nlohmann::json&)> tool_caller
) {
    std::vector<SamplingMessage> messages = original_request.messages;
    size_t iterations = 0;
    auto start_time = std::chrono::steady_clock::now();

    while (iterations < config.max_iterations) {
        // Check timeout
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed > config.timeout) {
            throw std::runtime_error("Tool loop timeout exceeded");
        }

        // Build request with current messages
        CreateMessageRequest current_request = original_request;
        current_request.messages = messages;

        // Call LLM
        CreateMessageResult result = handler(current_request);

        // Check if result contains tool use
        bool has_tool_use = false;

        // Check single content block
        if (is_tool_use(result.content)) {
            has_tool_use = true;
        }

        // Check content blocks array
        if (!result.content_blocks.empty()) {
            for (const auto& block : result.content_blocks) {
                if (is_tool_use(block)) {
                    has_tool_use = true;
                    break;
                }
            }
        }

        // Also check stop_reason for "toolUse"
        if (result.stop_reason == "toolUse") {
            has_tool_use = true;
        }

        if (!has_tool_use || !tool_caller) {
            // No tool use or no tool caller - return final result
            return result;
        }

        // Extract tool uses from result
        std::vector<ToolUseContent> tool_uses;

        if (is_tool_use(result.content)) {
            tool_uses.push_back(std::get<ToolUseContent>(result.content));
        }

        for (const auto& block : result.content_blocks) {
            if (std::holds_alternative<ToolUseContent>(block)) {
                tool_uses.push_back(std::get<ToolUseContent>(block));
            }
        }

        if (tool_uses.empty()) {
            // No actual tool uses found despite has_tool_use being true
            return result;
        }

        // Call each tool and add results to messages
        for (const auto& tool_use : tool_uses) {
            // Create assistant message with tool use
            SamplingMessage assistant_msg;
            assistant_msg.role = "assistant";
            assistant_msg.content = tool_use;
            messages.push_back(assistant_msg);

            // Call the tool
            ToolResultContent tool_result = call_tool(tool_use, tool_caller);

            // Create user message with tool result
            SamplingMessage user_msg;
            user_msg.role = "user";
            user_msg.content = tool_result;
            messages.push_back(user_msg);
        }

        iterations++;
    }

    // Max iterations reached - throw error
    throw std::runtime_error("Tool loop max iterations exceeded");
}

} // namespace

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
        // Check if we should use tool loop
        const bool has_tools = request_opt->tools.has_value() && !request_opt->tools->empty();
        const bool has_tool_caller = tool_caller_ != nullptr;

        if (has_tools && has_tool_caller) {
            // Execute tool loop
            CreateMessageResult result = execute_tool_loop(
                *request_opt,
                sampling_handler_,
                config_,
                tool_caller_
            );
            return result.to_json();
        } else {
            // Simple single call
            CreateMessageResult result = sampling_handler_(*request_opt);
            return result.to_json();
        }
    } catch (const std::exception& e) {
        // Handler threw exception - return internal error
        nlohmann::json error;
        error["code"] = -32603;  // Internal error
        error["message"] = std::string("Sampling handler failed: ") + e.what();
        return error;
    }
}

// ============================================================================
// JSON Conversion Helpers (ContentBlock)
// ============================================================================

namespace {

// Helper to convert Annotations to JSON
nlohmann::json annotations_to_json_impl(const content::Annotations& a) {
    nlohmann::json j;
    if (a.audience.has_value()) {
        j["audience"] = *a.audience;
    }
    if (a.priority.has_value()) {
        j["priority"] = *a.priority;
    }
    if (a.last_modified.has_value()) {
        j["lastModified"] = *a.last_modified;
    }
    return j;
}

} // anonymous namespace

nlohmann::json content_to_json(const ContentBlock& content) {
    return std::visit([](const auto& c) -> nlohmann::json {
        nlohmann::json j;
        j["type"] = c.type;

        using ContentType = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<ContentType, TextContent>) {
            j["text"] = c.text;

        } else if constexpr (std::is_same_v<ContentType, content::ImageContent>) {
            j["data"] = c.data;
            j["mimeType"] = c.mime_type;
            if (c.annotations.has_value()) {
                j["annotations"] = annotations_to_json_impl(*c.annotations);
            }

        } else if constexpr (std::is_same_v<ContentType, content::AudioContent>) {
            j["data"] = c.data;
            j["mimeType"] = c.mime_type;
            if (c.annotations.has_value()) {
                j["annotations"] = annotations_to_json_impl(*c.annotations);
            }

        } else if constexpr (std::is_same_v<ContentType, content::ResourceLink>) {
            j["uri"] = c.uri;
            if (c.annotations.has_value()) {
                j["annotations"] = annotations_to_json_impl(*c.annotations);
            }

        } else if constexpr (std::is_same_v<ContentType, content::EmbeddedResource>) {
            j["uri"] = c.resource.uri;
            if (c.resource.mime_type.has_value()) {
                j["mimeType"] = *c.resource.mime_type;
            }
            if (c.resource.is_text) {
                j["text"] = c.resource.text;
            } else {
                j["blob"] = c.resource.blob;
            }
            if (c.annotations.has_value()) {
                j["annotations"] = annotations_to_json_impl(*c.annotations);
            }

        } else if constexpr (std::is_same_v<ContentType, ToolUseContent>) {
            j["id"] = c.id;
            j["name"] = c.name;
            j["arguments"] = c.arguments;

        } else if constexpr (std::is_same_v<ContentType, ToolResultContent>) {
            j["tool_use_id"] = c.tool_use_id;
            if (c.content.has_value()) {
                j["content"] = *c.content;
            }
            if (c.is_error.has_value()) {
                j["isError"] = *c.is_error;
            }
        }

        return j;
    }, content);
}

std::optional<ContentBlock> content_from_json(const nlohmann::json& j) {
    if (!j.contains("type") || !j["type"].is_string()) {
        return std::nullopt;
    }

    std::string type = j["type"].get<std::string>();

    if (type == "text") {
        if (!j.contains("text") || !j["text"].is_string()) {
            return std::nullopt;
        }
        TextContent content;
        content.type = "text";
        content.text = j["text"].get<std::string>();
        return content;

    } else if (type == "image") {
        if (!j.contains("data") || !j["data"].is_string()) {
            return std::nullopt;
        }
        if (!j.contains("mimeType") || !j["mimeType"].is_string()) {
            return std::nullopt;
        }
        content::ImageContent content;
        content.type = "image";
        content.data = j["data"].get<std::string>();
        content.mime_type = j["mimeType"].get<std::string>();
        if (j.contains("annotations") && j["annotations"].is_object()) {
            const auto& a = j["annotations"];
            content::Annotations annotations;
            if (a.contains("audience") && a["audience"].is_array()) {
                std::vector<std::string> audience;
                for (const auto& item : a["audience"]) {
                    if (item.is_string()) {
                        audience.push_back(item.get<std::string>());
                    }
                }
                if (!audience.empty()) {
                    content.annotations = std::move(annotations);
                    content.annotations->audience = std::move(audience);
                }
            }
            if (a.contains("priority") && a["priority"].is_number()) {
                if (!content.annotations.has_value()) {
                    content.annotations = content::Annotations{};
                }
                content.annotations->priority = a["priority"].get<double>();
            }
            if (a.contains("lastModified") && a["lastModified"].is_string()) {
                if (!content.annotations.has_value()) {
                    content.annotations = content::Annotations{};
                }
                content.annotations->last_modified = a["lastModified"].get<std::string>();
            }
        }
        return content;

    } else if (type == "audio") {
        if (!j.contains("data") || !j["data"].is_string()) {
            return std::nullopt;
        }
        if (!j.contains("mimeType") || !j["mimeType"].is_string()) {
            return std::nullopt;
        }
        content::AudioContent content;
        content.type = "audio";
        content.data = j["data"].get<std::string>();
        content.mime_type = j["mimeType"].get<std::string>();
        // Parse annotations similarly to image...
        return content;

    } else if (type == "resource") {
        if (!j.contains("uri") || !j["uri"].is_string()) {
            return std::nullopt;
        }
        content::ResourceLink content;
        content.type = "resource";
        content.uri = j["uri"].get<std::string>();
        return content;

    } else if (type == "embedded") {
        if (!j.contains("uri") || !j["uri"].is_string()) {
            return std::nullopt;
        }
        content::EmbeddedResource content;
        content.type = "embedded";
        content.resource.uri = j["uri"].get<std::string>();
        if (j.contains("mimeType") && j["mimeType"].is_string()) {
            content.resource.mime_type = j["mimeType"].get<std::string>();
        }
        if (j.contains("text") && j["text"].is_string()) {
            content.resource.is_text = true;
            content.resource.text = j["text"].get<std::string>();
        } else if (j.contains("blob") && j["blob"].is_string()) {
            content.resource.is_text = false;
            content.resource.blob = j["blob"].get<std::string>();
        } else {
            return std::nullopt;
        }
        return content;

    } else if (type == "tool_use") {
        ToolUseContent content;
        content.type = "tool_use";
        if (j.contains("id") && j["id"].is_string()) {
            content.id = j["id"].get<std::string>();
        }
        if (j.contains("name") && j["name"].is_string()) {
            content.name = j["name"].get<std::string>();
        }
        if (j.contains("arguments")) {
            content.arguments = j["arguments"];
        }
        return content;

    } else if (type == "tool_result") {
        ToolResultContent content;
        content.type = "tool_result";
        if (j.contains("tool_use_id") && j["tool_use_id"].is_string()) {
            content.tool_use_id = j["tool_use_id"].get<std::string>();
        }
        if (j.contains("content") && j["content"].is_string()) {
            content.content = j["content"].get<std::string>();
        }
        if (j.contains("isError") && j["isError"].is_boolean()) {
            content.is_error = j["isError"].get<bool>();
        }
        return content;
    }

    return std::nullopt;
}

} // namespace mcpp::client
