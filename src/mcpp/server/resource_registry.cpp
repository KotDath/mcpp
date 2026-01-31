// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/resource_registry.h"

#include <regex>
#include <sstream>

#include "mcpp/core/json_rpc.h"
#include "mcpp/transport/transport.h"
#include "mcpp/util/uri_template.h"

namespace mcpp {

namespace server {

namespace {

/**
 * @brief Extract parameter names from a URI template
 *
 * Parses templates like "file://{path}" or "users/{id}" and extracts
 * the parameter names (e.g., ["path"], ["id"]).
 *
 * @param uri_template The URI template string
 * @return Vector of parameter names in order of appearance
 */
std::vector<std::string> extract_parameter_names(const std::string& uri_template) {
    std::vector<std::string> names;
    size_t pos = 0;

    while ((pos = uri_template.find('{', pos)) != std::string::npos) {
        size_t end = uri_template.find('}', pos);
        if (end == std::string::npos) {
            break;
        }

        std::string name = uri_template.substr(pos + 1, end - pos - 1);

        // Handle query parameter templates like {?var*}
        if (!name.empty() && name[0] == '?') {
            name = name.substr(1);
        }
        // Handle asterisk suffix for Level 2 expansion
        if (!name.empty() && name.back() == '*') {
            name = name.substr(0, name.length() - 1);
        }

        if (!name.empty()) {
            names.push_back(name);
        }

        pos = end + 1;
    }

    return names;
}

} // anonymous namespace

bool ResourceRegistry::register_resource(
    const std::string& uri,
    const std::string& name,
    const std::optional<std::string>& description,
    const std::string& mime_type,
    ResourceHandler handler
) {
    ResourceRegistration reg{
        uri,
        name,
        description,
        mime_type,
        std::move(handler)
    };

    resources_[uri] = std::move(reg);
    notify_changed();
    return true;
}

std::vector<nlohmann::json> ResourceRegistry::list_resources() const {
    std::vector<nlohmann::json> result;
    result.reserve(resources_.size() + template_resources_.size());

    // Add static resources
    for (const auto& [uri, registration] : resources_) {
        nlohmann::json resource;
        resource["uri"] = registration.uri;
        resource["name"] = registration.name;

        if (registration.description.has_value()) {
            resource["description"] = *registration.description;
        }

        resource["mimeType"] = registration.mime_type;
        result.push_back(std::move(resource));
    }

    // Add template resources
    for (const auto& [uri_template, registration] : template_resources_) {
        nlohmann::json resource;
        resource["uri"] = registration.uri_template;
        resource["name"] = registration.name;

        if (registration.description.has_value()) {
            resource["description"] = *registration.description;
        }

        resource["mimeType"] = registration.mime_type;

        // Mark this as a template resource per MCP spec
        nlohmann::json template_obj;
        template_obj["uri"] = registration.uri_template;
        resource["template"] = std::move(template_obj);

        result.push_back(std::move(resource));
    }

    return result;
}

std::optional<nlohmann::json> ResourceRegistry::read_resource(const std::string& uri) const {
    // First, try to find a static resource
    auto it = resources_.find(uri);
    if (it != resources_.end()) {
        const ResourceRegistration& registration = it->second;
        ResourceContent content = registration.handler(uri);

        return build_resource_result(content, registration.mime_type);
    }

    // Try to match against templates
    for (const auto& [template_str, registration] : template_resources_) {
        nlohmann::json params = match_template(uri, registration.uri_template, registration.parameter_names);
        if (params != nullptr && !params.empty()) {
            // Template matched - call the template handler
            ResourceContent content = registration.handler(uri, params);

            return build_resource_result(content, registration.mime_type);
        }
    }

    return std::nullopt;
}

bool ResourceRegistry::has_resource(const std::string& uri) const {
    // Check static resources
    if (resources_.find(uri) != resources_.end()) {
        return true;
    }

    // Check if any template matches
    for (const auto& [template_str, registration] : template_resources_) {
        nlohmann::json params = match_template(uri, registration.uri_template, registration.parameter_names);
        if (params != nullptr && !params.empty()) {
            return true;
        }
    }

    return false;
}

// === Template Resources ===

bool ResourceRegistry::register_template(
    const std::string& uri_template,
    const std::string& name,
    const std::optional<std::string>& description,
    const std::string& mime_type,
    TemplateResourceHandler handler
) {
    TemplateResourceRegistration reg;
    reg.uri_template = uri_template;
    reg.name = name;
    reg.description = description;
    reg.mime_type = mime_type;
    reg.parameter_names = extract_parameter_names(uri_template);
    reg.handler = std::move(handler);

    template_resources_[uri_template] = std::move(reg);

    // Note: util::UriTemplate::expand is available for building URIs from templates
    // e.g., std::string uri = util::UriTemplate::expand("file://{path}", {{"path", "/etc/config"}});

    notify_changed();
    return true;
}

nlohmann::json ResourceRegistry::match_template(
    const std::string& uri,
    const std::string& uri_template,
    const std::vector<std::string>& parameter_names
) const {
    nlohmann::json params = nlohmann::json::object();

    // Build a regex pattern from the template
    // Replace {var} with (.+) to capture values
    std::string pattern_str = uri_template;

    // Escape special regex characters except for our placeholders
    const std::string special_chars = R"([]{}()^$.|*+?\)";
    size_t pos = 0;

    // First, escape special regex characters
    for (char c : special_chars) {
        std::string escape;
        escape += '\\';
        escape += c;
        size_t escape_pos = 0;
        while ((escape_pos = pattern_str.find(c, escape_pos)) != std::string::npos) {
            // Don't escape if this is part of a placeholder
            if (escape_pos > 0 && pattern_str[escape_pos - 1] == '{') {
                escape_pos++;
                continue;
            }
            if (escape_pos < pattern_str.length() - 1 && pattern_str[escape_pos + 1] == '}') {
                escape_pos++;
                continue;
            }
            pattern_str.replace(escape_pos, 1, escape);
            escape_pos += escape.length();
        }
    }

    // Replace placeholders with capture groups
    size_t placeholder_pos = 0;
    size_t param_index = 0;
    while ((placeholder_pos = pattern_str.find("{", placeholder_pos)) != std::string::npos) {
        size_t end = pattern_str.find("}", placeholder_pos);
        if (end == std::string::npos) {
            break;
        }

        // Check if this is a query parameter template {?var*}
        bool is_query = false;
        if (placeholder_pos + 1 < pattern_str.length() && pattern_str[placeholder_pos + 1] == '?') {
            is_query = true;
        }

        std::string capture;
        if (is_query) {
            // For query parameters, capture the entire query string
            capture = "(\\?.*)?";
        } else {
            // For path parameters, capture everything except / (non-greedy)
            capture = "([^/?]+)";
        }

        pattern_str.replace(placeholder_pos, end - placeholder_pos + 1, capture);
        placeholder_pos += capture.length();
        param_index++;
    }

    // Anchor the pattern
    pattern_str = "^" + pattern_str + "$";

    try {
        std::regex pattern(pattern_str);
        std::smatch matches;

        if (std::regex_match(uri, matches, pattern)) {
            // Extract parameter values from matches
            // matches[0] is the full string, matches[1+] are capture groups
            for (size_t i = 0; i < parameter_names.size() && i + 1 < matches.size(); ++i) {
                const std::string& param_name = parameter_names[i];
                std::string value = matches[i + 1].str();

                // Remove leading '?' from query parameter values
                if (!value.empty() && value[0] == '?') {
                    value = value.substr(1);
                }

                params[param_name] = value;
            }
        }
    } catch (const std::regex_error&) {
        // If regex fails to compile, return empty params
        return nullptr;
    }

    return params;
}

// === Resource Subscriptions ===

bool ResourceRegistry::subscribe(const std::string& uri, const std::string& subscriber_id) {
    Subscription sub;
    sub.uri = uri;
    sub.subscriber_id = subscriber_id;
    sub.subscribed_at = std::chrono::steady_clock::now();

    subscriptions_[uri].push_back(std::move(sub));
    return true;
}

bool ResourceRegistry::unsubscribe(const std::string& uri, const std::string& subscriber_id) {
    auto it = subscriptions_.find(uri);
    if (it == subscriptions_.end()) {
        return false;
    }

    auto& subs = it->second;
    auto sub_it = std::remove_if(subs.begin(), subs.end(),
        [&subscriber_id](const Subscription& sub) {
            return sub.subscriber_id == subscriber_id;
        });

    bool was_subscribed = sub_it != subs.end();
    subs.erase(sub_it, subs.end());

    // Clean up empty subscription lists
    if (subs.empty()) {
        subscriptions_.erase(it);
    }

    return was_subscribed;
}

void ResourceRegistry::notify_updated(const std::string& uri) {
    if (transport_ == nullptr) {
        return;
    }

    auto it = subscriptions_.find(uri);
    if (it == subscriptions_.end()) {
        return;
    }

    // Build the notifications/resources/updated message
    nlohmann::json notification_params;
    notification_params["uri"] = uri;

    core::JsonRpcNotification notification;
    notification.method = "notifications/resources/updated";
    notification.params = notification_params;

    std::string message = notification.to_string();

    // Send to each subscriber
    // Note: In a real implementation, you'd want to handle per-subscriber transport
    // This is a simplified version where we send to all via the same transport
    for (const auto& sub : it->second) {
        (void)sub; // Suppress unused warning in this simplified implementation
        transport_->send(message);
    }
}

void ResourceRegistry::set_transport(transport::Transport& transport) {
    transport_ = &transport;
}

// === Completion Support ===

void ResourceRegistry::set_completion_handler(
    const std::string& resource_name,
    CompletionHandler handler
) {
    completion_handlers_[resource_name] = std::move(handler);
}

std::optional<std::vector<Completion>> ResourceRegistry::get_completion(
    const std::string& resource_name,
    const std::string& argument_name,
    const nlohmann::json& current_value,
    const std::optional<nlohmann::json>& reference
) const {
    auto it = completion_handlers_.find(resource_name);
    if (it == completion_handlers_.end()) {
        return std::nullopt;
    }

    // Call the handler to get completion suggestions
    const CompletionHandler& handler = it->second;
    return handler(argument_name, current_value, reference);
}

// === Private Helpers ===

nlohmann::json ResourceRegistry::build_resource_result(
    const ResourceContent& content,
    const std::string& default_mime_type
) const {
    // Build the MCP ReadResourceResult format
    // According to MCP spec, the result has a "contents" array
    // with items containing uri, mimeType, and either "text" or "blob"
    nlohmann::json contents_item;
    contents_item["uri"] = content.uri;
    contents_item["type"] = "resource";

    // Use content's mime_type if provided, otherwise fall back to default
    if (content.mime_type.has_value()) {
        contents_item["mimeType"] = *content.mime_type;
    } else {
        contents_item["mimeType"] = default_mime_type;
    }

    // Set either text or blob based on is_text flag
    if (content.is_text) {
        contents_item["text"] = content.text;
    } else {
        contents_item["blob"] = content.blob;
    }

    nlohmann::json result;
    nlohmann::json::array_t contents_array;
    contents_array.push_back(std::move(contents_item));
    result["contents"] = std::move(contents_array);

    return result;
}

void ResourceRegistry::set_notify_callback(NotifyCallback cb) {
    notify_cb_ = std::move(cb);
}

void ResourceRegistry::notify_changed() {
    if (notify_cb_) {
        notify_cb_();
    }
}

} // namespace server

} // namespace mcpp
