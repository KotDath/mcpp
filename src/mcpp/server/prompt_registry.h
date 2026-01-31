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

private:
    std::unordered_map<std::string, PromptRegistration> prompts_;
};

} // namespace mcpp::server

#endif // MCPP_SERVER_PROMPT_REGISTRY_H
