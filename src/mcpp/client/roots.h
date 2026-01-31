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

#ifndef MCPP_CLIENT_ROOTS_H
#define MCPP_CLIENT_ROOTS_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace mcpp::client {

// Type alias for JSON values - using nlohmann::json
using JsonValue = nlohmann::json;

/**
 * @brief Root directory or file that the server can operate on
 *
 * Represents a root URI that the client advertises to the server.
 * Per MCP 2025-11-25 spec, root URIs must start with "file://".
 *
 * Example JSON:
 * {"uri": "file:///path/to/project", "name": "My Project"}
 */
struct Root {
    /**
     * @brief URI identifying the root
     *
     * MUST start with "file://" per MCP spec.
     */
    std::string uri;

    /**
     * @brief Optional display name for the root
     *
     * Provides a human-readable name for the root.
     */
    std::optional<std::string> name;

    /**
     * @brief Validate that this root has a valid file:// URI
     *
     * @return true if uri starts with "file://", false otherwise
     */
    bool is_valid() const {
        return uri.rfind("file://", 0) == 0;
    }
};

/**
 * @brief Result for roots/list request from server
 *
 * Contains the list of roots that the client advertises.
 *
 * Example JSON:
 * {"roots": [{"uri": "file:///path/to/project", "name": "My Project"}]}
 */
struct ListRootsResult {
    /**
     * @brief List of roots
     */
    std::vector<Root> roots;

    /**
     * @brief Convert this result to JSON
     *
     * @return JSON value in MCP roots/list response format
     */
    JsonValue to_json() const;
};

/**
 * @brief Manages client roots and sends list_changed notifications
 *
 * RootsManager stores the list of root URIs that the client advertises
 * to the server. It can send notifications when the roots change.
 *
 * Thread safety: Not thread-safe. Use external synchronization if needed.
 *
 * Usage:
 *   RootsManager manager;
 *
 *   // Set up notification callback
 *   manager.set_notify_callback([&client]() {
 *       client.send_notification("notifications/roots/list_changed", nullptr);
 *   });
 *
 *   // Set initial roots
 *   manager.set_roots({
 *       {"file:///path/to/project", "My Project"}
 *   });
 *
 *   // When roots change, notify
 *   manager.set_roots(new_roots);
 *   manager.notify_changed();
 */
class RootsManager {
public:
    /**
     * @brief Callback type for roots changed notifications
     *
     * Invoked when notify_changed() is called.
     */
    using NotifyCallback = std::function<void()>;

    RootsManager() = default;
    ~RootsManager() = default;

    // Non-copyable
    RootsManager(const RootsManager&) = delete;
    RootsManager& operator=(const RootsManager&) = delete;

    // Movable
    RootsManager(RootsManager&&) = default;
    RootsManager& operator=(RootsManager&&) = default;

    /**
     * @brief Set the roots list
     *
     * Replaces the current roots with the provided list.
     * Does not automatically send notification - call notify_changed()
     * after this to trigger the list_changed notification.
     *
     * @param roots New roots list
     */
    void set_roots(std::vector<Root> roots);

    /**
     * @brief Get the current roots list
     *
     * @return const reference to current roots
     */
    const std::vector<Root>& get_roots() const { return roots_; }

    /**
     * @brief Set the callback for roots changed notifications
     *
     * The callback will be invoked when notify_changed() is called.
     *
     * @param cb Callback function (nullptr to clear)
     */
    void set_notify_callback(NotifyCallback cb);

    /**
     * @brief Send roots/list_changed notification
     *
     * If a notify callback is registered, invokes it to send
     * the notification. Otherwise, does nothing.
     */
    void notify_changed();

    /**
     * @brief Validate that a URI is a valid file:// root URI
     *
     * Static utility function to check if a URI starts with "file://".
     *
     * @param uri URI to validate
     * @return true if uri starts with "file://", false otherwise
     */
    static bool validate_uri(const std::string& uri) {
        return uri.rfind("file://", 0) == 0;
    }

private:
    /// Current roots list
    std::vector<Root> roots_;

    /// Callback for sending list_changed notifications
    NotifyCallback notify_cb_;
};

} // namespace mcpp::client

#endif // MCPP_CLIENT_ROOTS_H
