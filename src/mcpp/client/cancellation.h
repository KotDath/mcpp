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

#ifndef MCPP_CLIENT_CANCELLATION_H
#define MCPP_CLIENT_CANCELLATION_H

#include <stop_token>
#include <optional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <variant>

#include "../core/json_rpc.h"

namespace mcpp::client {

/**
 * @brief CancellationToken for cooperative cancellation
 *
 * A token allows checking if cancellation has been requested.
 * Tokens are cheap to copy and can be passed to long-running operations
 * that periodically poll is_cancelled().
 *
 * Thread-safe: Multiple threads can check the same token concurrently.
 *
 * Usage:
 *   CancellationSource source;
 *   CancellationToken token = source.get_token();
 *
 *   // In long-running operation:
 *   if (token.is_cancelled()) {
 *       return; // Clean up and exit
 *   }
 */
class CancellationToken {
public:
    /**
     * @brief Default constructor - creates a token that can never be cancelled
     *
     * Useful for optional cancellation - if no token is provided,
     * operations will never be cancelled.
     */
    CancellationToken() = default;

    /**
     * @brief Construct a token from a stop_token
     *
     * @param token The stop_token to wrap
     */
    explicit CancellationToken(std::stop_token token) noexcept
        : token_(std::move(token)) {}

    /**
     * @brief Check if cancellation has been requested
     *
     * @return true if stop was requested, false otherwise
     */
    bool is_cancelled() const noexcept {
        return token_.stop_requested();
    }

    /**
     * @brief Get the underlying stop_token
     *
     * @return const reference to the stop_token
     */
    const std::stop_token& get() const noexcept {
        return token_;
    }

private:
    std::stop_token token_;
};

/**
 * @brief CancellationSource for requesting cancellation
 *
 * A source can request cancellation on all associated tokens.
 * Each source generates tokens that can be passed to operations.
 *
 * Non-copyable and non-movable - each source is unique.
 * This prevents accidental cancellation of unintended operations.
 *
 * Thread-safe: cancel() can be called from any thread.
 *
 * Usage:
 *   CancellationSource source;
 *   std::thread worker([token = source.get_token()]() {
 *       while (!token.is_cancelled()) {
 *           // Do work
 *       }
 *   });
 *   source.cancel(); // Request cancellation from another thread
 */
class CancellationSource {
public:
    CancellationSource() = default;

    // Non-copyable
    CancellationSource(const CancellationSource&) = delete;
    CancellationSource& operator=(const CancellationSource&) = delete;

    // Movable (for transferring ownership to CancellationManager)
    CancellationSource(CancellationSource&&) noexcept = default;
    CancellationSource& operator=(CancellationSource&&) noexcept = default;

    /**
     * @brief Get a token associated with this source
     *
     * The token can be copied and passed to multiple operations.
     * All tokens from the same source will be cancelled together.
     *
     * @return CancellationToken that observes this source
     */
    CancellationToken get_token() const noexcept {
        return CancellationToken(source_.get_token());
    }

    /**
     * @brief Request cancellation
     *
     * All tokens obtained from this source will have is_cancelled()
     * return true after this call.
     *
     * Thread-safe: Can be called from any thread.
     * Idempotent: Multiple calls have the same effect as one.
     */
    void cancel() noexcept {
        source_.request_stop();
    }

    /**
     * @brief Check if cancellation has been requested
     *
     * @return true if stop was requested, false otherwise
     */
    bool is_cancelled() const noexcept {
        return source_.stop_requested();
    }

private:
    std::stop_source source_;
};

/**
 * @brief Manager for cancelable MCP client requests
 *
 * CancellationManager tracks pending requests by their RequestId
 * and handles cancellation notifications from the server.
 *
 * When the server sends a notifications/cancelled message, the manager
 * finds the corresponding request and calls cancel() on its CancellationSource.
 *
 * Race condition handling:
 * - If a cancellation arrives after completion, it's silently ignored
 * - unregister_request is idempotent (no error if already unregistered)
 *
 * Thread-safe: All methods use mutex locking for concurrent access.
 *
 * Usage:
 *   // When sending a request:
 *   CancellationSource source;
 *   cancellation_manager.register_request(id, source);
 *
 *   // In request handler:
 *   auto token = source.get_token();
 *   if (token.is_cancelled()) { return; }
 *
 *   // When response arrives:
 *   cancellation_manager.unregister_request(id);
 */
class CancellationManager {
public:
    CancellationManager() = default;

    // Non-copyable, non-movable
    CancellationManager(const CancellationManager&) = delete;
    CancellationManager& operator=(const CancellationManager&) = delete;
    CancellationManager(CancellationManager&&) = delete;
    CancellationManager& operator=(CancellationManager&&) = delete;

    ~CancellationManager() = default;

    /**
     * @brief Register a request for potential cancellation
     *
     * Stores the cancellation source mapped to the request ID.
     * The source will be used if the server sends a cancellation notification.
     *
     * @param id Request ID to register
     * @param source CancellationSource for this request (moved)
     */
    void register_request(core::RequestId id, CancellationSource source);

    /**
     * @brief Handle a cancellation notification from the server
     *
     * When the server sends notifications/cancelled with a requestId,
     * this method finds the matching request and cancels it.
     *
     * If the request is not found (already completed), this is a no-op.
     * This handles the race condition where cancellation arrives after response.
     *
     * @param id Request ID to cancel
     * @param reason Optional reason string (logged for debugging)
     */
    void handle_cancelled(core::RequestId id, const std::optional<std::string>& reason);

    /**
     * @brief Unregister a request (response received or timeout)
     *
     * Removes the request from pending tracking.
     * Called when a request completes (response, error, or timeout).
     *
     * Idempotent: No error if the request was already unregistered
     * or cancelled via handle_cancelled().
     *
     * @param id Request ID to unregister
     */
    void unregister_request(core::RequestId id);

    /**
     * @brief Get the count of pending cancelable requests
     *
     * @return Number of requests currently registered for cancellation
     */
    size_t pending_count() const;

private:
    // Map of request ID to cancellation source
    std::unordered_map<core::RequestId, CancellationSource> pending_;

    // Mutex protecting pending_ map
    mutable std::mutex mutex_;
};

} // namespace mcpp::client

#endif // MCPP_CLIENT_CANCELLATION_H
