// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/resource_registry.h"

namespace mcpp::server {

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
    return true;
}

std::vector<nlohmann::json> ResourceRegistry::list_resources() const {
    std::vector<nlohmann::json> result;
    result.reserve(resources_.size());

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

    return result;
}

std::optional<nlohmann::json> ResourceRegistry::read_resource(const std::string& uri) const {
    auto it = resources_.find(uri);
    if (it == resources_.end()) {
        return std::nullopt;
    }

    const ResourceRegistration& registration = it->second;

    // Call the handler to get the content
    ResourceContent content = registration.handler(uri);

    // Build the MCP ReadResourceResult format
    // According to MCP spec, the result has a "contents" array
    // with items containing uri, mimeType, and either "text" or "blob"
    nlohmann::json contents_item;
    contents_item["uri"] = content.uri;
    contents_item["type"] = "resource";

    // Use content's mime_type if provided, otherwise fall back to registration
    if (content.mime_type.has_value()) {
        contents_item["mimeType"] = *content.mime_type;
    } else {
        contents_item["mimeType"] = registration.mime_type;
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

bool ResourceRegistry::has_resource(const std::string& uri) const {
    return resources_.find(uri) != resources_.end();
}

} // namespace mcpp::server
