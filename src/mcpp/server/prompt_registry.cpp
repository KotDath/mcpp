// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/prompt_registry.h"

namespace mcpp::server {

bool PromptRegistry::register_prompt(
    const std::string& name,
    const std::optional<std::string>& description,
    const std::vector<PromptArgument>& arguments,
    PromptHandler handler
) {
    // Check if prompt with this name already exists
    if (prompts_.find(name) != prompts_.end()) {
        return false;
    }

    // Store the prompt registration
    PromptRegistration registration{
        name,
        description,
        arguments,
        std::move(handler)
    };

    prompts_[name] = std::move(registration);
    notify_changed();
    return true;
}

std::vector<nlohmann::json> PromptRegistry::list_prompts() const {
    std::vector<nlohmann::json> result;
    result.reserve(prompts_.size());

    for (const auto& [name, registration] : prompts_) {
        nlohmann::json prompt_entry;
        prompt_entry["name"] = registration.name;

        if (registration.description.has_value()) {
            prompt_entry["description"] = *registration.description;
        }

        // Build arguments array
        if (!registration.arguments.empty()) {
            nlohmann::json::array_t args_array;
            for (const auto& arg : registration.arguments) {
                nlohmann::json arg_entry;
                arg_entry["name"] = arg.name;
                if (arg.description.has_value()) {
                    arg_entry["description"] = *arg.description;
                }
                if (arg.required) {
                    arg_entry["required"] = true;
                }
                args_array.push_back(std::move(arg_entry));
            }
            prompt_entry["arguments"] = std::move(args_array);
        }

        result.push_back(std::move(prompt_entry));
    }

    return result;
}

std::optional<nlohmann::json> PromptRegistry::get_prompt(
    const std::string& name,
    const nlohmann::json& arguments
) const {
    auto it = prompts_.find(name);
    if (it == prompts_.end()) {
        return std::nullopt;
    }

    // Call the handler to get prompt messages with argument substitution
    const PromptRegistration& registration = it->second;
    std::vector<PromptMessage> messages = registration.handler(name, arguments);

    // Convert to MCP GetPromptResult format
    nlohmann::json result;
    nlohmann::json::array_t messages_array;

    for (const auto& msg : messages) {
        nlohmann::json msg_entry;
        msg_entry["role"] = msg.role;
        msg_entry["content"] = msg.content;
        messages_array.push_back(std::move(msg_entry));
    }

    result["messages"] = std::move(messages_array);

    // Include optional _meta if the prompt provided any metadata
    // (This would be an extension - not required for basic prompts)

    return result;
}

bool PromptRegistry::has_prompt(const std::string& name) const {
    return prompts_.find(name) != prompts_.end();
}

void PromptRegistry::set_completion_handler(
    const std::string& prompt_name,
    CompletionHandler handler
) {
    completion_handlers_[prompt_name] = std::move(handler);
}

std::optional<std::vector<Completion>> PromptRegistry::get_completion(
    const std::string& prompt_name,
    const std::string& argument_name,
    const nlohmann::json& current_value,
    const std::optional<nlohmann::json>& reference
) const {
    auto it = completion_handlers_.find(prompt_name);
    if (it == completion_handlers_.end()) {
        return std::nullopt;
    }

    // Call the handler to get completion suggestions
    const CompletionHandler& handler = it->second;
    return handler(argument_name, current_value, reference);
}

void PromptRegistry::set_notify_callback(NotifyCallback cb) {
    notify_cb_ = std::move(cb);
}

void PromptRegistry::notify_changed() {
    if (notify_cb_) {
        notify_cb_();
    }
}

} // namespace mcpp::server
