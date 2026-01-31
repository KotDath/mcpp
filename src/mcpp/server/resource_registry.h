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

#include "mcpp/content/pagination.h"
#include "mcpp/util/uri_template.h"

namespace mcpp {

namespace server {

/**
 * @brief Completion suggestion for argument autocompletion
 *
 * Represents a single completion value with an optional description.
 * Matches the MCP CompleteResult format which returns an array of
 * completion values with optional descriptions.
 *
 * Note: This struct is shared between PromptRegistry and ResourceRegistry
 * for consistency in completion handling across both registries.
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
 * resource URIs or values. Called when a client requests completion
 * via resources/complete.
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

} // namespace server

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
 * @brief Callback type for list_changed notifications
 *
 * Invoked when the registry contents change (resources added/removed).
 */
using NotifyCallback = std::function<void()>;

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
     * @brief List all registered resources with pagination
     *
     * Returns paginated results for both regular and template resources.
     * The cursor is an opaque token; clients should not interpret its contents.
     *
     * @param cursor Optional cursor from previous page for continuation
     * @return PaginatedResult containing resource metadata and optional next cursor
     */
    content::PaginatedResult<nlohmann::json> list_resources_paginated(
        const std::optional<std::string>& cursor = std::nullopt
    ) const;

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

    // === Completion Support ===

    /**
     * @brief Set a completion handler for a resource
     *
     * Registers a completion handler for the specified resource.
     * The handler will be called when clients request completion
     * via resources/complete for this resource.
     *
     * @param resource_name Name of the resource (URI or template name)
     * @param handler Function to generate completion suggestions
     */
    void set_completion_handler(
        const std::string& resource_name,
        CompletionHandler handler
    );

    /**
     * @brief Get completion suggestions for a resource
     *
     * Calls the completion handler registered for the resource
     * and returns suggestions for the given argument and value.
     *
     * @param resource_name Name of the resource
     * @param argument_name Name of the argument being completed
     * @param current_value Current partial value in the argument field
     * @param reference Optional reference context (e.g., cursor position)
     * @return Vector of completion suggestions, or nullopt if no handler registered
     */
    std::optional<std::vector<Completion>> get_completion(
        const std::string& resource_name,
        const std::string& argument_name,
        const nlohmann::json& current_value,
        const std::optional<nlohmann::json>& reference
    ) const;

    /**
     * @brief Set the callback for sending list_changed notifications
     *
     * The callback will be invoked when notify_changed() is called.
     * Typically used to send notifications/resources/list_changed to clients.
     *
     * @param cb Callback function (empty function to clear)
     */
    void set_notify_callback(NotifyCallback cb);

    /**
     * @brief Send resources/list_changed notification
     *
     * If a notify callback is registered, invokes it to send the notification.
     * Call this after registering/unregistering resources to notify clients.
     */
    void notify_changed();

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

    /// Completion handlers keyed by resource name (URI or template)
    std::unordered_map<std::string, CompletionHandler> completion_handlers_;

    /// Callback for sending list_changed notifications
    NotifyCallback notify_cb_;

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

    /**
     * @brief Build resource result in MCP format from content
     *
     * @param content The resource content
     * @param default_mime_type Fallback MIME type
     * @return JSON object in MCP ReadResourceResult format
     */
    nlohmann::json build_resource_result(
        const ResourceContent& content,
        const std::string& default_mime_type
    ) const;
};

} // namespace server

} // namespace mcpp

#endif // MCPP_SERVER_RESOURCE_REGISTRY_H
