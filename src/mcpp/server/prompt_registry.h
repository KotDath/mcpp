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

#ifndef MCPP_SERVER_PROMPT_REGISTRY_H
#define MCPP_SERVER_PROMPT_REGISTRY_H

#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>

namespace mcpp::server {

/**
 * @brief Completion suggestion for argument autocompletion
 *
 * Represents a single completion value with an optional description.
 * Matches the MCP CompleteResult format which returns an array of
 * completion values with optional descriptions.
 */
struct Completion {
    /// The completion value to insert
    std::string value;

    /// Optional human-readable description of this completion
    std::optional<std::string> description;
};

/**
 * @brief Completion handler function type
 *
 * User-provided function that generates completion suggestions for
 * prompt arguments. Called when a client requests completion via
 * prompts/complete.
 *
 * The handler receives:
 * - argument_name: The name of the argument being completed
 * - current_value: The current partial value in the argument field
 * - reference: Optional reference context (e.g., cursor position)
 *
 * Returns a vector of Completion suggestions. Empty vector means no completions.
 *
 * Thread safety: Handlers may be called from multiple threads.
 * Implementations must be thread-safe or use external synchronization.
 */
using CompletionHandler = std::function<std::vector<Completion>(
    const std::string& argument_name,
    const nlohmann::json& current_value,
    const std::optional<nlohmann::json>& reference
)>;

/**
 * @brief Callback type for list_changed notifications
 *
 * Invoked when the registry contents change (prompts added/removed).
 */
using NotifyCallback = std::function<void()>;

/**
 * PromptMessage represents a single message in a prompt template.
 *
 * Messages have a role (user or assistant) and content that can be
 * text, images, or other content types as defined in the MCP spec.
 *
 * Example:
 *   PromptMessage msg{
 *     .role = "user",
 *     .content = {{"type", "text"}, {"text", "Hello, {{name}}!"}}
 *   };
 */
struct PromptMessage {
    std::string role;  // "user" or "assistant"
    nlohmann::json content;  // TextContent, ImageContent, etc.
};

/**
 * PromptArgument defines a single argument in a prompt template.
 *
 * Arguments are parameters that can be substituted into prompt messages.
 * The handler is responsible for performing the substitution.
 */
struct PromptArgument {
    std::string name;
    std::optional<std::string> description;
    bool required = false;
};

/**
 * PromptHandler is the function type for prompt handlers.
 *
 * The handler receives the prompt name and the arguments provided by the client.
 * It should return a vector of PromptMessage with argument substitution applied.
 *
 * Example handler:
 *   [](const std::string& name, const nlohmann::json& args) {
 *     std::string topic = args.value("topic", "world");
 *     return std::vector<PromptMessage>{
 *       {"user", {{"type", "text"}, {"text", "Tell me about " + topic}}}
 *     };
 *   }
 */
using PromptHandler = std::function<std::vector<PromptMessage>(
    const std::string& name,
    const nlohmann::json& arguments
)>;

/**
 * PromptRegistration stores all metadata for a registered prompt.
 *
 * This includes the prompt name, description, argument definitions,
 * and the handler function that generates the prompt messages.
 */
struct PromptRegistration {
    std::string name;
    std::optional<std::string> description;
    std::vector<PromptArgument> arguments;
    PromptHandler handler;
};

/**
 * PromptRegistry manages prompt registration, discovery, and retrieval.
 *
 * Prompts are reusable templates that clients can use as starting points
 * for LLM interactions. This registry provides:
 *
 * - Registration of prompts with name, description, and argument definitions
 * - Listing all registered prompts for discovery (prompts/list)
 * - Retrieving a specific prompt with argument substitution (prompts/get)
 */
class PromptRegistry {
public:
    /**
     * Register a prompt with the registry.
     *
     * @param name Unique name for the prompt
     * @param description Optional human-readable description
     * @param arguments List of argument definitions
     * @param handler Function that generates prompt messages
     * @return true if registration succeeded, false if name already exists
     */
    bool register_prompt(
        const std::string& name,
        const std::optional<std::string>& description,
        const std::vector<PromptArgument>& arguments,
        PromptHandler handler
    );

    /**
     * List all registered prompts.
     *
     * Returns an array of prompt objects with name, optional description,
     * and argument definitions. Used for prompts/list responses.
     *
     * @return JSON array of prompt definitions
     */
    std::vector<nlohmann::json> list_prompts() const;

    /**
     * Get a prompt by name with argument substitution.
     *
     * Calls the prompt's handler with the provided arguments and returns
     * the result in MCP GetPromptResult format:
     *   {"messages": [{"role": "user", "content": {...}}, ...]}
     *
     * @param name Prompt name
     * @param arguments Arguments for substitution (JSON object)
     * @return Prompt result JSON, or nullopt if prompt not found
     */
    std::optional<nlohmann::json> get_prompt(
        const std::string& name,
        const nlohmann::json& arguments
    ) const;

    /**
     * Check if a prompt with the given name exists.
     *
     * @param name Prompt name to check
     * @return true if prompt exists, false otherwise
     */
    bool has_prompt(const std::string& name) const;

    /**
     * @brief Set a completion handler for a prompt argument
     *
     * Registers a completion handler for the specified prompt.
     * The handler will be called when clients request completion
     * via prompts/complete for this prompt's arguments.
     *
     * @param prompt_name Name of the prompt
     * @param handler Function to generate completion suggestions
     */
    void set_completion_handler(
        const std::string& prompt_name,
        CompletionHandler handler
    );

    /**
     * @brief Get completion suggestions for a prompt argument
     *
     * Calls the completion handler registered for the prompt
     * and returns suggestions for the given argument and value.
     *
     * @param prompt_name Name of the prompt
     * @param argument_name Name of the argument being completed
     * @param current_value Current partial value in the argument field
     * @param reference Optional reference context (e.g., cursor position)
     * @return Vector of completion suggestions, or nullopt if no handler registered
     */
    std::optional<std::vector<Completion>> get_completion(
        const std::string& prompt_name,
        const std::string& argument_name,
        const nlohmann::json& current_value,
        const std::optional<nlohmann::json>& reference
    ) const;

    /**
     * @brief Set the callback for sending list_changed notifications
     *
     * The callback will be invoked when notify_changed() is called.
     * Typically used to send notifications/prompts/list_changed to clients.
     *
     * @param cb Callback function (empty function to clear)
     */
    void set_notify_callback(NotifyCallback cb);

    /**
     * @brief Send prompts/list_changed notification
     *
     * If a notify callback is registered, invokes it to send the notification.
     * Call this after registering/unregistering prompts to notify clients.
     */
    void notify_changed();

private:
    std::unordered_map<std::string, PromptRegistration> prompts_;

    /// Completion handlers keyed by prompt name
    std::unordered_map<std::string, CompletionHandler> completion_handlers_;

    /// Callback for sending list_changed notifications
    NotifyCallback notify_cb_;
};

} // namespace mcpp::server

#endif // MCPP_SERVER_PROMPT_REGISTRY_H
