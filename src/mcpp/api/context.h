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

#ifndef MCPP_API_CONTEXT_H
#define MCPP_API_CONTEXT_H

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <variant>

#include "mcpp/api/role.h"
#include "mcpp/core/json_rpc.h"
#include "mcpp/util/logger.h"

namespace mcpp::api {

// Forward declaration for Service (used in Peer reference)
template<ServiceRole Role>
class Service;

/**
 * @brief Request-scoped context for handling MCP requests
 *
 * RequestContext provides request metadata, progress tracking, and
 * logging capabilities for each incoming request. It follows the
 * rust-sdk RequestContext pattern with thread-safe property access.
 *
 * Thread safety:
 * - Multiple threads may read properties concurrently (shared_lock)
 * - Only one thread may write properties at a time (unique_lock)
 * - The Span provides automatic request duration tracking
 *
 * @tparam Role The service role (RoleClient or RoleServer)
 */
template<ServiceRole Role>
class RequestContext {
public:
    /**
     * @brief Construct a request context
     *
     * @param request_id Request ID for tracking
     * @param method RPC method name
     */
    RequestContext(std::string_view request_id, std::string_view method)
        : request_id_(request_id)
        , method_(method)
        , span_("request", {{"request_id", std::string(request_id)},
                            {"method", std::string(method)}}) {}

    /**
     * @brief Destructor
     */
    ~RequestContext() = default;

    /**
     * @brief Non-copyable
     */
    RequestContext(const RequestContext&) = delete;
    RequestContext& operator=(const RequestContext&) = delete;

    /**
     * @brief Movable
     */
    RequestContext(RequestContext&&) = default;
    RequestContext& operator=(RequestContext&&) = default;

    /**
     * @brief Get the request ID
     *
     * Thread-safe: Multiple threads may call concurrently.
     *
     * @return Request ID string
     */
    std::string request_id() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return request_id_;
    }

    /**
     * @brief Get the method name
     *
     * Thread-safe: Multiple threads may call concurrently.
     *
     * @return Method name string
     */
    std::string method() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return method_;
    }

    /**
     * @brief Get the progress token if available
     *
     * Thread-safe: Multiple threads may call concurrently.
     *
     * @return Progress token or nullopt if not set
     */
    std::optional<std::string> progress_token() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return progress_token_;
    }

    /**
     * @brief Set the progress token
     *
     * Thread-safe: Only one thread may set the token at a time.
     *
     * @param token Progress token value
     */
    void set_progress_token(std::string_view token) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        progress_token_ = std::string(token);
    }

    /**
     * @brief Add contextual key-value pair
     *
     * Thread-safe: Multiple threads may call concurrently.
     *
     * @param key Context key
     * @param value Context value
     */
    void add_context(std::string_view key, std::string_view value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        context_[std::string(key)] = std::string(value);
        span_.add_context(key, value);
    }

    /**
     * @brief Get a context value
     *
     * Thread-safe: Multiple threads may call concurrently.
     *
     * @param key Context key
     * @return Context value or nullopt if not found
     */
    std::optional<std::string> get_context(std::string_view key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = context_.find(std::string(key));
        if (it != context_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get all context key-value pairs
     *
     * Thread-safe: Multiple threads may call concurrently.
     *
     * @return Copy of the context map
     */
    std::map<std::string, std::string> all_context() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return context_;
    }

    /**
     * @brief Access the logging span
     *
     * Thread-safe: Each context has its own span.
     *
     * @return Reference to the span
     */
    util::Logger::Span& span() noexcept {
        return span_;
    }

    /**
     * @brief Log a message in this request's context
     *
     * @param level Log level
     * @param message Message to log
     */
    void log(util::Logger::Level level, std::string_view message) {
        auto ctx = all_context();
        ctx["request_id"] = request_id_;
        ctx["method"] = method_;
        util::logger().log(level, message, ctx);
    }

private:
    std::string request_id_;
    std::string method_;
    std::optional<std::string> progress_token_;
    std::map<std::string, std::string> context_;
    mutable std::shared_mutex mutex_;
    util::Logger::Span span_;
};

/**
 * @brief Notification-scoped context for handling MCP notifications
 *
 * NotificationContext provides notification metadata and logging for
 * incoming notifications. Unlike RequestContext, it has no request ID
 * since notifications are one-way messages without responses.
 *
 * Thread safety:
 * - Multiple threads may read properties concurrently (shared_lock)
 * - Only one thread may write properties at a time (unique_lock)
 * - The Span provides automatic notification duration tracking
 *
 * @tparam Role The service role (RoleClient or RoleServer)
 */
template<ServiceRole Role>
class NotificationContext {
public:
    /**
     * @brief Construct a notification context
     *
     * @param method Notification method name
     */
    explicit NotificationContext(std::string_view method)
        : method_(method)
        , span_("notification", {{"method", std::string(method)}}) {}

    /**
     * @brief Destructor
     */
    ~NotificationContext() = default;

    /**
     * @brief Non-copyable
     */
    NotificationContext(const NotificationContext&) = delete;
    NotificationContext& operator=(const NotificationContext&) = delete;

    /**
     * @brief Movable
     */
    NotificationContext(NotificationContext&&) = default;
    NotificationContext& operator=(NotificationContext&&) = default;

    /**
     * @brief Get the method name
     *
     * Thread-safe: Multiple threads may call concurrently.
     *
     * @return Method name string
     */
    std::string method() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return method_;
    }

    /**
     * @brief Add contextual key-value pair
     *
     * Thread-safe: Only one thread may write at a time.
     *
     * @param key Context key
     * @param value Context value
     */
    void add_context(std::string_view key, std::string_view value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        context_[std::string(key)] = std::string(value);
        span_.add_context(key, value);
    }

    /**
     * @brief Get a context value
     *
     * Thread-safe: Multiple threads may call concurrently.
     *
     * @param key Context key
     * @return Context value or nullopt if not found
     */
    std::optional<std::string> get_context(std::string_view key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = context_.find(std::string(key));
        if (it != context_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get all context key-value pairs
     *
     * Thread-safe: Multiple threads may call concurrently.
     *
     * @return Copy of the context map
     */
    std::map<std::string, std::string> all_context() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return context_;
    }

    /**
     * @brief Access the logging span
     *
     * Thread-safe: Each context has its own span.
     *
     * @return Reference to the span
     */
    util::Logger::Span& span() noexcept {
        return span_;
    }

    /**
     * @brief Log a message in this notification's context
     *
     * @param level Log level
     * @param message Message to log
     */
    void log(util::Logger::Level level, std::string_view message) {
        auto ctx = all_context();
        ctx["method"] = method_;
        util::logger().log(level, message, ctx);
    }

private:
    std::string method_;
    std::map<std::string, std::string> context_;
    mutable std::shared_mutex mutex_;
    util::Logger::Span span_;
};

// Type aliases for commonly used context types
using ClientRequestContext = RequestContext<RoleClient>;
using ServerRequestContext = RequestContext<RoleServer>;
using ClientNotificationContext = NotificationContext<RoleClient>;
using ServerNotificationContext = NotificationContext<RoleServer>;

} // namespace mcpp::api

#endif // MCPP_API_CONTEXT_H
