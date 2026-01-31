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

#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

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
 * @brief Content block variant for sampling message content
 *
 * Currently supports TextContent. Extensible for future content types
 * like ImageContent, AudioContent, etc.
 */
using ContentBlock = std::variant<TextContent>;

// ============================================================================
// Sampling Message Types
// ============================================================================

/**
 * @brief A single message in a sampling conversation
 *
 * Corresponds to the "SamplingMessage" type in MCP 2025-11-25 schema.
 * Messages have a role (user/assistant) and content.
 */
struct SamplingMessage {
    std::string role;      // "user" or "assistant"
    ContentBlock content;  // Message content
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
 *
 * Note: tools and tool_choice are added in plan 03-04 for tool use support.
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
 */
struct CreateMessageResult {
    std::string role = "assistant";
    ContentBlock content;
    std::string model;                             // Model identifier used
    std::optional<std::string> stop_reason;        // "endTurn", "stopSequence", "maxTokens"

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
 * Usage:
 *   SamplingClient sampling_client;
 *   sampling_client.set_sampling_handler(my_handler);
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
     * If parsing fails or no handler is registered, returns a JSON-RPC error.
     *
     * @param params Request parameters as JSON
     * @return Result or error as JSON
     */
    nlohmann::json handle_create_message(const nlohmann::json& params);

private:
    /// User-provided callback for actual LLM calls
    SamplingHandler sampling_handler_;
};

// ============================================================================
// JSON Conversion Helpers (ContentBlock)
// ============================================================================

/**
 * @brief Convert ContentBlock to JSON
 *
 * Handles the variant serialization for different content types.
 */
inline nlohmann::json content_to_json(const ContentBlock& content) {
    return std::visit([](const auto& c) -> nlohmann::json {
        nlohmann::json j;
        j["type"] = c.type;
        if constexpr (std::is_same_v<std::decay_t<decltype(c)>, TextContent>) {
            j["text"] = c.text;
        }
        return j;
    }, content);
}

/**
 * @brief Parse ContentBlock from JSON
 *
 * Handles the variant deserialization based on the "type" field.
 */
inline std::optional<ContentBlock> content_from_json(const nlohmann::json& j) {
    if (!j.contains("type") || !j["type"].is_string()) {
        return std::nullopt;
    }

    std::string type = j["type"].get<std::string>();

    if (type == "text") {
        if (j.contains("text") && j["text"].is_string()) {
            TextContent content;
            content.type = "text";
            content.text = j["text"].get<std::string>();
            return content;
        }
    }

    // Unknown content type
    return std::nullopt;
}

} // namespace mcpp::client

#endif // MCPP_CLIENT_SAMPLING_H
