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

#ifndef MCPP_CLIENT_SAMPLING_H
#define MCPP_CLIENT_SAMPLING_H

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "mcpp/content/content.h"

namespace mcpp::client {

// ============================================================================
// Content Types
// ============================================================================

/**
 * @brief Text content block for sampling messages
 *
 * Corresponds to the "text" content type in MCP sampling spec.
 * Used for text-based message content in createMessage requests and results.
 */
struct TextContent {
    std::string type = "text";
    std::string text;
};

/**
 * @brief Tool use content block for sampling messages
 *
 * Corresponds to the "tool_use" content type in MCP sampling spec.
 * Used when the LLM requests to call a tool during agentic workflows.
 */
struct ToolUseContent {
    std::string type = "tool_use";
    std::string id;              // Unique ID for this tool use
    std::string name;            // Tool name
    nlohmann::json arguments;    // Tool arguments as JSON
};

/**
 * @brief Tool result content block for sampling messages
 *
 * Corresponds to the "tool_result" content type in MCP sampling spec.
 * Used to return the result of a tool execution back to the LLM.
 */
struct ToolResultContent {
    std::string type = "tool_result";
    std::string tool_use_id;                 // References ToolUseContent.id
    std::optional<std::string> content;      // Text result
    std::optional<bool> is_error;            // True if tool call failed
};

/**
 * @brief Content block variant for sampling message content
 *
 * Supports all MCP content types: TextContent, ImageContent, AudioContent,
 * ResourceLink, EmbeddedResource, ToolUseContent, and ToolResultContent.
 *
 * The variant order preserves backward compatibility (text first, then rich
 * content types, then tool-related types).
 */
using ContentBlock = std::variant<
    TextContent,
    content::ImageContent,
    content::AudioContent,
    content::ResourceLink,
    content::EmbeddedResource,
    ToolUseContent,
    ToolResultContent
>;

// ============================================================================
// Sampling Message Types
// ============================================================================

/**
 * @brief A single message in a sampling conversation
 *
 * Corresponds to the "SamplingMessage" type in MCP 2025-11-25 schema.
 * Messages have a role (user/assistant) and content.
 *
 * Content can be a single ContentBlock or a vector of ContentBlocks
 * (e.g., assistant message with multiple tool_use blocks).
 */
struct SamplingMessage {
    std::string role;      // "user" or "assistant"
    ContentBlock content;  // Message content (single block)

    // For messages with multiple content blocks (e.g., assistant with multiple tool calls)
    std::optional<std::vector<ContentBlock>> content_blocks;
};

/**
 * @brief Model preferences for sampling
 *
 * Corresponds to "ModelPreferences" in MCP 2025-11-25 schema.
 * Used to prioritize cost, speed, or intelligence when selecting a model.
 * Values should sum to 1.0 if all three are provided.
 */
struct ModelPreferences {
    std::optional<double> cost_priority;        // 0-1, lower is better
    std::optional<double> speed_priority;       // 0-1, higher is better
    std::optional<double> intelligence_priority; // 0-1, higher is better
};

// ============================================================================
// Tool Types
// ============================================================================

/**
 * @brief Tool choice option for auto mode
 *
 * LLM decides whether to use tools.
 */
struct ToolChoiceAuto {
    std::string type = "auto";
};

/**
 * @brief Tool choice option for required mode
 *
 * LLM must use at least one tool.
 */
struct ToolChoiceRequired {
    std::string type = "required";
};

/**
 * @brief Tool choice option for none mode
 *
 * LLM should not use any tools.
 */
struct ToolChoiceNone {
    std::string type = "none";
};

/**
 * @brief Tool choice option for specific tool
 *
 * LLM must use the specified tool.
 */
struct ToolChoiceTool {
    std::string type = "tool";
    std::string name;  // Specific tool to use
};

/**
 * @brief Tool choice variant
 *
 * Controls how the LLM should use available tools.
 */
using ToolChoice = std::variant<ToolChoiceAuto, ToolChoiceRequired, ToolChoiceNone, ToolChoiceTool>;

/**
 * @brief Tool definition for sampling
 *
 * Represents a tool that the LLM can call.
 * The server provides the full schema; this is a minimal representation.
 */
struct Tool {
    std::string name;
    nlohmann::json input_schema;  // JSON Schema for arguments
};

/**
 * @brief Configuration for tool loop execution
 *
 * Controls iteration limits and timeouts for agentic tool loops.
 */
struct ToolLoopConfig {
    size_t max_iterations = 10;  // Maximum tool iterations
    std::chrono::milliseconds timeout = std::chrono::milliseconds(300000);  // 5 minutes
};

// ============================================================================
// Sampling Request Types
// ============================================================================

/**
 * @brief Request parameters for sampling/createMessage
 *
 * Corresponds to the "CreateMessageRequest" type in MCP 2025-11-25 schema.
 * The server sends this request to the client asking for LLM text generation.
 *
 * Required fields:
 * - messages: Array of conversation messages
 * - max_tokens: Maximum tokens to generate
 *
 * Optional fields:
 * - model_preferences: Hints for model selection
 * - system_prompt: System prompt to use
 * - include_context: Whether to include server resources/prompts
 * - temperature: Sampling temperature (0-1)
 * - stop_sequences: Sequences that stop generation
 * - metadata: Arbitrary metadata
 * - tools: Tools available to the LLM
 * - tool_choice: How the LLM should use tools
 */
struct CreateMessageRequest {
    std::vector<SamplingMessage> messages;
    std::optional<ModelPreferences> model_preferences;
    std::optional<std::string> system_prompt;
    std::optional<std::string> include_context; // "none", "thisServer", "allServers"
    std::optional<double> temperature;
    int64_t max_tokens;
    std::optional<std::vector<std::string>> stop_sequences;
    std::optional<nlohmann::json> metadata;
    std::optional<std::vector<Tool>> tools;
    std::optional<ToolChoice> tool_choice;

    /**
     * @brief Parse a CreateMessageRequest from JSON
     *
     * @param j JSON value to parse
     * @return Parsed request, or nullopt if validation fails
     */
    static std::optional<CreateMessageRequest> from_json(const nlohmann::json& j);
};

/**
 * @brief Result from sampling/createMessage
 *
 * Corresponds to the "CreateMessageResult" type in MCP 2025-11-25 schema.
 * The client returns this to the server with the LLM's response.
 *
 * Stop reasons: "endTurn", "stopSequence", "maxTokens", "toolUse"
 */
struct CreateMessageResult {
    std::string role = "assistant";
    ContentBlock content;
    std::vector<ContentBlock> content_blocks;  // For multiple content blocks (e.g., tool uses)
    std::string model;                             // Model identifier used
    std::optional<std::string> stop_reason;        // "endTurn", "stopSequence", "maxTokens", "toolUse"

    /**
     * @brief Serialize this result to JSON
     *
     * @return JSON value representing this result
     */
    nlohmann::json to_json() const;
};

// ============================================================================
// Sampling Handler Types
// ============================================================================

/**
 * @brief Callback type for handling sampling requests
 *
 * User code provides a SamplingHandler to process createMessage requests.
 * The handler receives the request and returns the LLM's response.
 *
 * Example usage:
 *   client.set_sampling_handler([](const CreateMessageRequest& req) {
 *       // Call LLM API here
 *       CreateMessageResult result;
 *       result.content = TextContent{"text", "Hello from LLM"};
 *       result.model = "gpt-4";
 *       return result;
 *   });
 */
using SamplingHandler = std::function<CreateMessageResult(const CreateMessageRequest&)>;

// ============================================================================
// Sampling Client
// ============================================================================

/**
 * @brief Manages sampling (LLM text generation) for MCP clients
 *
 * SamplingClient handles the server-side createMessage requests,
 * parses them into strongly-typed structs, and invokes user-provided
 * handlers to perform actual LLM API calls.
 *
 * Tool loop support: When tools are provided in the request and a tool_caller
 * is registered, the SamplingClient can execute agentic tool loops where
 * the LLM calls tools, results are fed back, and the loop continues.
 *
 * Usage:
 *   SamplingClient sampling_client;
 *   sampling_client.set_sampling_handler(my_handler);
 *
 *   // Enable tool use with synchronous tool caller
 *   sampling_client.set_tool_caller([](std::string_view method, const nlohmann::json& params) {
 *       // Call MCP server's tools/call endpoint
 *       return tool_result;
 *   });
 *
 *   // When server sends sampling/createMessage request:
 *   auto result_json = sampling_client.handle_create_message(params_json);
 */
class SamplingClient {
public:
    /**
     * @brief Set the handler for createMessage requests
     *
     * The handler is invoked when the server sends a sampling/createMessage request.
     * It should call an LLM API and return the generated text.
     *
     * @param handler Function that processes sampling requests
     */
    void set_sampling_handler(SamplingHandler handler);

    /**
     * @brief Handle a sampling/createMessage request from the server
     *
     * Parses the JSON request, validates it, invokes the registered handler,
     * and returns the result as JSON.
     *
     * If the request includes tools and a tool_caller is registered,
     * automatically executes the tool loop.
     *
     * If parsing fails or no handler is registered, returns a JSON-RPC error.
     *
     * @param params Request parameters as JSON
     * @return Result or error as JSON
     */
    nlohmann::json handle_create_message(const nlohmann::json& params);

    /**
     * @brief Set the tool loop configuration
     *
     * @param config Configuration for tool loop execution
     */
    void set_tool_loop_config(ToolLoopConfig config);

    /**
     * @brief Get the tool loop configuration
     *
     * @return Current tool loop configuration
     */
    ToolLoopConfig& get_tool_loop_config() { return config_; }
    const ToolLoopConfig& get_tool_loop_config() const { return config_; }

    /**
     * @brief Set the tool caller for executing tools during tool loops
     *
     * The tool caller is a function that sends JSON-RPC requests to the MCP server
     * to execute tools (typically "tools/call"). It must be synchronous/blocking.
     *
     * @param caller Function that takes (method, params) and returns JSON result
     */
    void set_tool_caller(std::function<nlohmann::json(std::string_view, const nlohmann::json&)> caller);

    /**
     * @brief Clear the tool caller (disables tool loop execution)
     */
    void clear_tool_caller();

private:
    /// User-provided callback for actual LLM calls
    SamplingHandler sampling_handler_;

    /// Tool loop configuration
    ToolLoopConfig config_;

    /// Synchronous tool caller for executing tools during tool loops
    std::function<nlohmann::json(std::string_view, const nlohmann::json&)> tool_caller_;
};

// ============================================================================
// JSON Conversion Helpers (ContentBlock)
// ============================================================================

// Forward declarations - implementations are in content.cpp
// These delegate to mcpp::content::content_to_json/content_from_json
// to handle all content types including rich media (image, audio, resources)

/**
 * @brief Convert ContentBlock to JSON
 *
 * Handles the variant serialization for all content types:
 * TextContent, ImageContent, AudioContent, ResourceLink, EmbeddedResource,
 * ToolUseContent, ToolResultContent.
 *
 * @param content ContentBlock variant to serialize
 * @return JSON representation of the content
 */
nlohmann::json content_to_json(const ContentBlock& content);

/**
 * @brief Parse ContentBlock from JSON
 *
 * Handles the variant deserialization based on the "type" field.
 * Supported types: "text", "image", "audio", "resource", "embedded",
 * "tool_use", "tool_result"
 *
 * @param j JSON value to parse
 * @return ContentBlock if parsing succeeds, nullopt otherwise
 */
std::optional<ContentBlock> content_from_json(const nlohmann::json& j);

} // namespace mcpp::client

#endif // MCPP_CLIENT_SAMPLING_H
