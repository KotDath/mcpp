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

#ifndef MCPP_SERVER_RESOURCE_REGISTRY_H
#define MCPP_SERVER_RESOURCE_REGISTRY_H

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "mcpp/util/uri_template.h"

namespace mcpp {

namespace transport {
class Transport;
}

namespace server {

/**
 * @brief Resource content result from a handler
 *
 * Returned by resource handlers to provide content for a resource URI.
 * Supports both text and binary (blob) content types.
 *
 * For text resources, set is_text=true and populate the text field.
 * For binary resources, set is_text=false and populate the blob field
 * with base64-encoded data.
 */
struct ResourceContent {
    /// URI of this resource
    std::string uri;

    /// Optional MIME type (overrides registration-time mime_type if provided)
    std::optional<std::string> mime_type;

    /// Content type flag: true for text, false for binary blob
    bool is_text = true;

    /// Text content (used when is_text == true)
    std::string text;

    /// Binary content as base64-encoded string (used when is_text == false)
    std::string blob;
};

/**
 * @brief Resource handler function type
 *
 * User-provided function that generates resource content for a given URI.
 * Called when a client requests to read a resource via resources/read.
 *
 * The handler receives the requested URI and returns ResourceContent.
 * Handlers may perform file I/O, computation, or any other operation
 * to generate the resource content.
 *
 * Thread safety: Handlers may be called from multiple threads.
 * Implementations must be thread-safe or use external synchronization.
 */
using ResourceHandler = std::function<ResourceContent(const std::string& uri)>;

/**
 * @brief Template resource handler function type
 *
 * User-provided function that generates resource content for a URI template.
 * Called when a client requests to read a resource matching a registered template.
 *
 * The handler receives both the expanded URI and the extracted parameters.
 * This allows handlers to access path variables, query parameters, etc.
 *
 * Thread safety: Handlers may be called from multiple threads.
 * Implementations must be thread-safe or use external synchronization.
 */
using TemplateResourceHandler = std::function<ResourceContent(
    const std::string& expanded_uri,
    const nlohmann::json& parameters
)>;

/**
 * @brief Resource registration record
 *
 * Stores metadata and handler for a registered resource.
 * Resources are registered by URI and can be discovered via resources/list.
 */
struct ResourceRegistration {
    /// Unique URI identifying this resource (any scheme: file://, custom://, etc.)
    std::string uri;

    /// Human-readable name for this resource
    std::string name;

    /// Optional description of what this resource provides
    std::optional<std::string> description;

    /// MIME type of this resource (e.g., "text/plain", "application/json")
    std::string mime_type;

    /// Handler function called when reading this resource
    ResourceHandler handler;
};

/**
 * @brief Template resource registration record
 *
 * Stores metadata and handler for a registered resource template.
 * Templates use URI template syntax (RFC 6570 Level 1-2) to define
 * parameterized resources like "file://{path}" or "config://{section}".
 */
struct TemplateResourceRegistration {
    /// URI template pattern (e.g., "file://{path}", "users/{id}")
    std::string uri_template;

    /// Human-readable name for this resource template
    std::string name;

    /// Optional description of what this resource template provides
    std::optional<std::string> description;

    /// MIME type of resources matching this template
    std::string mime_type;

    /// Parameter names extracted from the template (e.g., ["path"] from "file://{path}")
    std::vector<std::string> parameter_names;

    /// Handler function called when reading a resource matching this template
    TemplateResourceHandler handler;
};

/**
 * @brief Registry for MCP resources with discovery and reading support
 *
 * ResourceRegistry enables servers to expose data (files, artifacts,
 * computed values) to MCP clients. Resources are registered with a URI,
 * name, optional description, MIME type, and a handler function.
 *
 * Key features:
 * - Register resources with any URI scheme (file://, custom://, etc.)
 * - Register resource templates with URI parameter expansion (RFC 6570)
 * - List all registered resources via list_resources()
 * - Read resource content via read_resource() with proper MCP format
 * - Support for both text and binary (base64 blob) content types
 * - Resource subscriptions with change notifications
 *
 * Thread safety: Not thread-safe. Use external synchronization if calling
 * from multiple threads.
 *
 * Usage:
 *   ResourceRegistry registry;
 *
 *   // Register a static resource
 *   registry.register_resource(
 *       "file:///config.json",
 *       "Config",
 *       "Application configuration",
 *       "application/json",
 *       [](const std::string& uri) -> ResourceContent {
 *           return ResourceContent{uri, "application/json", true, read_config(), ""};
 *       }
 *   );
 *
 *   // Register a template resource
 *   registry.register_template(
 *       "file://{path}",
 *       "File",
 *       "Read any file",
 *       "text/plain",
 *       [](const std::string& expanded_uri, const nlohmann::json& params) -> ResourceContent {
 *           std::string path = params["path"];
 *           return ResourceContent{expanded_uri, "text/plain", true, read_file(path), ""};
 *       }
 *   );
 *
 *   auto list = registry.list_resources();
 *   auto content = registry.read_resource("file:///etc/config");
 */
class ResourceRegistry {
public:
    ResourceRegistry() = default;

    // Non-copyable, non-movable
    ResourceRegistry(const ResourceRegistry&) = delete;
    ResourceRegistry& operator=(const ResourceRegistry&) = delete;
    ResourceRegistry(ResourceRegistry&&) = delete;
    ResourceRegistry& operator=(ResourceRegistry&&) = delete;

    ~ResourceRegistry() = default;

    /**
     * @brief Register a resource
     *
     * Registers a resource with the given URI, name, description,
     * MIME type, and handler function. If a resource with the same
     * URI already exists, it is replaced.
     *
     * @param uri Unique URI for this resource
     * @param name Human-readable name
     * @param description Optional description
     * @param mime_type MIME type of the resource content
     * @param handler Function to generate resource content
     * @return true if registration succeeded
     */
    bool register_resource(
        const std::string& uri,
        const std::string& name,
        const std::optional<std::string>& description,
        const std::string& mime_type,
        ResourceHandler handler
    );

    /**
     * @brief List all registered resources
     *
     * Returns an array of resource objects suitable for the
     * MCP resources/list response. Each object contains:
     * - uri: The resource URI
     * - name: Human-readable name
     * - description: Optional description (if provided during registration)
     * - mimeType: The MIME type
     *
     * @return Vector of JSON objects describing each resource
     */
    std::vector<nlohmann::json> list_resources() const;

    /**
     * @brief Read a resource by URI
     *
     * Finds the resource with the given URI, calls its handler,
     * and returns the content in MCP ReadResourceResult format.
     *
     * Returns std::nullopt if the URI is not registered.
     *
     * @param uri URI of the resource to read
     * @return JSON object with contents array, or nullopt if not found
     */
    std::optional<nlohmann::json> read_resource(const std::string& uri) const;

    /**
     * @brief Check if a resource URI is registered
     *
     * @param uri URI to check
     * @return true if the resource exists
     */
    bool has_resource(const std::string& uri) const;

    // === Template Resources ===

    /**
     * @brief Register a resource template
     *
     * Registers a resource template with URI parameter expansion support.
     * Templates use RFC 6570 Level 1-2 syntax for variable expansion.
     *
     * Example templates:
     * - "file://{path}" matches any file:// URI
     * - "users/{id}" matches "users/123", "users/abc", etc.
     * - "config://{section}/{key}" matches "config/database/host"
     *
     * @param uri_template URI template pattern (e.g., "file://{path}")
     * @param name Human-readable name for this template
     * @param description Optional description
     * @param mime_type MIME type of resources matching this template
     * @param handler Function to generate resource content
     * @return true if registration succeeded
     */
    bool register_template(
        const std::string& uri_template,
        const std::string& name,
        const std::optional<std::string>& description,
        const std::string& mime_type,
        TemplateResourceHandler handler
    );

    // === Resource Subscriptions ===

    /**
     * @brief Subscribe to resource updates
     *
     * Registers a subscriber to receive notifications when a resource changes.
     * The subscriber_id typically identifies a client session.
     *
     * @param uri URI of the resource to monitor
     * @param subscriber_id Unique identifier for the subscriber (e.g., session ID)
     * @return true if subscription succeeded
     */
    bool subscribe(const std::string& uri, const std::string& subscriber_id);

    /**
     * @brief Unsubscribe from resource updates
     *
     * Removes a subscription for a specific resource and subscriber.
     *
     * @param uri URI of the resource
     * @param subscriber_id Subscriber identifier
     * @return true if was subscribed and now removed
     */
    bool unsubscribe(const std::string& uri, const std::string& subscriber_id);

    /**
     * @brief Notify subscribers that a resource has been updated
     *
     * Sends notifications/resources/updated to all subscribers of the resource.
     * Requires transport to be set via set_transport().
     *
     * @param uri URI of the updated resource
     */
    void notify_updated(const std::string& uri);

    /**
     * @brief Set the transport for sending subscription notifications
     *
     * The transport is used to send notifications/resources/updated messages.
     * Non-owning pointer - the caller ensures the transport outlives this registry.
     *
     * @param transport Reference to the transport implementation
     */
    void set_transport(transport::Transport& transport);

private:
    /**
     * @brief Subscription record
     *
     * Tracks a single subscriber's interest in a resource.
     */
    struct Subscription {
        std::string uri;
        std::string subscriber_id;
        std::chrono::steady_clock::time_point subscribed_at;
    };

    /// Registered resources keyed by URI
    std::unordered_map<std::string, ResourceRegistration> resources_;

    /// Registered template resources keyed by URI template
    std::unordered_map<std::string, TemplateResourceRegistration> template_resources_;

    /// Subscriptions keyed by URI (each URI maps to list of subscribers)
    std::unordered_map<std::string, std::vector<Subscription>> subscriptions_;

    /// Transport for sending subscription notifications (non-owning)
    transport::Transport* transport_ = nullptr;

    /**
     * @brief Match a URI against a template and extract parameters
     *
     * @param uri The URI to match
     * @param uri_template The template pattern
     * @param parameter_names Parameter names from the template
     * @return JSON object with extracted parameters, or nullptr if no match
     */
    nlohmann::json match_template(
        const std::string& uri,
        const std::string& uri_template,
        const std::vector<std::string>& parameter_names
    ) const;
};

} // namespace server

} // namespace mcpp

#endif // MCPP_SERVER_RESOURCE_REGISTRY_H
