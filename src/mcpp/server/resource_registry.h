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

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace mcpp::server {

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
 * @brief Registry for MCP resources with discovery and reading support
 *
 * ResourceRegistry enables servers to expose data (files, artifacts,
 * computed values) to MCP clients. Resources are registered with a URI,
 * name, optional description, MIME type, and a handler function.
 *
 * Key features:
 * - Register resources with any URI scheme (file://, custom://, etc.)
 * - List all registered resources via list_resources()
 * - Read resource content via read_resource() with proper MCP format
 * - Support for both text and binary (base64 blob) content types
 *
 * Thread safety: Not thread-safe. Use external synchronization if calling
 * from multiple threads.
 *
 * Usage:
 *   ResourceRegistry registry;
 *
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
 *   auto list = registry.list_resources();
 *   auto content = registry.read_resource("file:///config.json");
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

private:
    /// Registered resources keyed by URI
    std::unordered_map<std::string, ResourceRegistration> resources_;
};

} // namespace mcpp::server

#endif // MCPP_SERVER_RESOURCE_REGISTRY_H
