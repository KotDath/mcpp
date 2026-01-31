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

#ifndef MCPP_API_PEER_H
#define MCPP_API_PEER_H

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <variant>

#include "mcpp/api/role.h"
#include "mcpp/api/service.h"
#include "mcpp/core/error.h"
#include "mcpp/util/atomic_id.h"

namespace mcpp::api {

/**
 * @brief Message types for Peer message channel
 *
 * PeerMessage represents either a request (expects response) or
 * notification (no response) that can be sent through the message
 * passing channel.
 *
 * This follows the rust-sdk pattern from service.rs lines 294-305.
 */
template<ServiceRole Role>
class Peer {
public:
    // Import role-specific types from Service
    using PeerReq = typename Service<Role>::PeerReq;
    using PeerResp = typename Service<Role>::PeerResp;
    using PeerNot = typename Service<Role>::PeerNot;
    using PeerInfo = typename Service<Role>::PeerInfo;
    using Info = typename Service<Role>::Info;

    /**
     * @brief Request message with response callback
     *
     * Represents an outgoing request that expects a response.
     * The on_success callback is invoked when a successful response arrives.
     * The on_error callback is invoked when an error response arrives.
     *
     * Uses shared_ptr<promise> for future lifetime management across threads.
     */
    struct RequestMessage {
        /// Request ID for correlation
        core::RequestId id;

        /// The request payload (method name or structured request)
        PeerReq request;

        /// Success callback - invoked with response result
        std::function<void(const PeerResp&)> on_success;

        /// Error callback - invoked with JSON-RPC error
        std::function<void(const core::JsonRpcError&)> on_error;
    };

    /**
     * @brief Notification message (no response expected)
     *
     * Represents an outgoing notification that does not expect a response.
     * Notifications are one-way messages used for signaling and events.
     */
    struct NotificationMessage {
        /// The notification payload (method name or structured notification)
        PeerNot notification;
    };

    /**
     * @brief Message variant for the channel
     *
     * Messages can be either requests (with response handling) or
     * notifications (fire-and-forget).
     */
    using Message = std::variant<NotificationMessage, RequestMessage>;

    /**
     * @brief Construct a Peer with an ID provider
     *
     * @param id_provider Shared atomic ID provider for request ID generation
     */
    explicit Peer(std::shared_ptr<util::AtomicRequestIdProvider> id_provider)
        : id_provider_(std::move(id_provider)) {}

    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~Peer() = default;

    // Non-copyable
    Peer(const Peer&) = delete;
    Peer& operator=(const Peer&) = delete;

    // Non-movable (ID provider is shared, moving would create complications)
    Peer(Peer&&) = delete;
    Peer& operator=(Peer&&) = delete;

    /**
     * @brief Send a request to the remote peer
     *
     * Sends a request and returns a std::future that will contain the response.
     * The request is pushed to the message queue for processing by the event loop.
     *
     * Thread-safe: Can be called concurrently from multiple threads.
     *
     * @param request The request to send
     * @return Future that will contain the response or error
     *
     * @note This generates a request ID using id_provider_ and creates
     *       callbacks that set the promise value when the response arrives.
     */
    std::future<PeerResp> send_request(const PeerReq& request) {
        // Generate request ID
        core::RequestId id = id_provider_->next_id();

        // Create promise and future for response handling
        auto promise = std::make_shared<std::promise<PeerResp>>();
        auto future = promise->get_future();

        // Create callbacks that will set the promise when response arrives
        RequestMessage msg{
            id,
            request,
            [promise](const PeerResp& resp) {
                promise->set_value(resp);
            },
            [promise](const core::JsonRpcError& err) {
                // Set error as an exception in the future
                promise->set_exception(
                    std::make_exception_ptr(
                        std::runtime_error(err.message)
                    )
                );
            }
        };

        // Push message to queue
        {
            std::lock_guard lock(queue_mutex_);
            message_queue_.push(std::move(msg));
        }
        queue_cv_.notify_one();

        return future;
    }

    /**
     * @brief Send a notification to the remote peer
     *
     * Sends a notification (fire-and-forget message) to the remote peer.
     * Notifications do not expect responses.
     *
     * Thread-safe: Can be called concurrently from multiple threads.
     *
     * @param notification The notification to send
     */
    void send_notification(const PeerNot& notification) {
        NotificationMessage msg{notification};

        // Push message to queue
        {
            std::lock_guard lock(queue_mutex_);
            message_queue_.push(std::move(msg));
        }
        queue_cv_.notify_one();
    }

    /**
     * @brief Process pending messages (called by event loop)
     *
     * This method should be called by the event loop to process messages
     * in the queue. Each message is dispatched to the appropriate handler.
     *
     * Returns immediately if no messages are available.
     *
     * @return Number of messages processed
     */
    size_t process_messages() {
        size_t processed = 0;

        std::unique_lock lock(queue_mutex_);

        while (!message_queue_.empty()) {
            Message msg = std::move(message_queue_.front());
            message_queue_.pop();
            lock.unlock();  // Release lock while processing

            // Process message
            std::visit([this](auto&& message) {
                using T = std::decay_t<decltype(message)>;
                if constexpr (std::is_same_v<T, NotificationMessage>) {
                    handle_notification_message(message);
                } else if constexpr (std::is_same_v<T, RequestMessage>) {
                    handle_request_message(message);
                }
            }, msg);

            ++processed;
            lock.lock();  // Re-acquire for next iteration
        }

        return processed;
    }

    /**
     * @brief Wait for messages to be available and process them
     *
     * Blocks until at least one message is available or the stop token
     * is requested. Used by the event loop to wait for work.
     *
     * @param token Stop token for cancellation
     * @return true if messages were processed, false if stopped
     */
    bool wait_and_process(std::stop_token token) {
        std::unique_lock lock(queue_mutex_);

        // Wait for messages or stop request
        queue_cv_.wait(lock, [&] {
            return !message_queue_.empty() || token.stop_requested();
        });

        if (token.stop_requested()) {
            return false;
        }

        lock.unlock();
        process_messages();
        return true;
    }

    /**
     * @brief Check if there are pending messages
     *
     * @return true if the message queue is not empty
     */
    bool has_pending_messages() const {
        std::lock_guard lock(queue_mutex_);
        return !message_queue_.empty();
    }

    /**
     * @brief Get information about the remote peer
     *
     * Returns the peer information if available (set after initialization).
     * Uses shared_lock for thread-safe concurrent reads.
     *
     * Thread-safe: Multiple threads can call this concurrently.
     *
     * @return Optional containing peer info, or nullopt if not yet initialized
     */
    std::optional<PeerInfo> peer_info() const {
        std::shared_lock lock(mutex_);
        return peer_info_;
    }

    /**
     * @brief Set information about the remote peer
     *
     * Called during initialization to store peer information.
     * Uses unique_lock for exclusive write access.
     *
     * Thread-safe: Safe to call, but typically called once during initialization.
     *
     * @param info The peer information to set
     */
    void set_peer_info(const PeerInfo& info) {
        std::unique_lock lock(mutex_);
        peer_info_ = info;
    }

    /**
     * @brief Set information about the remote peer (move overload)
     *
     * @param info The peer information to set (moved)
     */
    void set_peer_info(PeerInfo&& info) {
        std::unique_lock lock(mutex_);
        peer_info_ = std::move(info);
    }

    /**
     * @brief Get the shared ID provider
     *
     * Returns the shared atomic ID provider used for request ID generation.
     * This can be shared with RequestContext for correlated ID generation.
     *
     * @return Shared pointer to the ID provider
     */
    std::shared_ptr<util::AtomicRequestIdProvider> id_provider() const noexcept {
        return id_provider_;
    }

protected:
    /**
     * @brief Handle a notification message
     *
     * Default implementation is a no-op. Subclasses can override to
     * actually send notifications via the transport.
     *
     * @param msg The notification message to handle
     */
    virtual void handle_notification_message(const NotificationMessage& msg) {
        (void)msg;
        // Default: no-op - subclasses implement transport sending
    }

    /**
     * @brief Handle a request message
     *
     * Default implementation is a no-op. Subclasses can override to
     * actually send requests via the transport.
     *
     * @param msg The request message to handle
     */
    virtual void handle_request_message(const RequestMessage& msg) {
        (void)msg;
        // Default: no-op - subclasses implement transport sending
    }

protected:
    /// Shared atomic ID provider for generating unique request IDs
    std::shared_ptr<util::AtomicRequestIdProvider> id_provider_;

    /// Mutex protecting peer_info_ (shared_mutex for reader-writer semantics)
    mutable std::shared_mutex mutex_;

    /// Information about the remote peer (set after initialization)
    std::optional<PeerInfo> peer_info_;

    /// Mutex protecting message_queue_
    mutable std::mutex queue_mutex_;

    /// Condition variable for blocking wait on empty queue
    std::condition_variable_any queue_cv_;

    /// Message queue for pending requests/notifications (mpsc pattern)
    std::queue<Message> message_queue_;
};

} // namespace mcpp::api

#endif // MCPP_API_PEER_H
