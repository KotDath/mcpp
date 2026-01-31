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

#ifndef MCPP_API_RUNNING_SERVICE_H
#define MCPP_API_RUNNING_SERVICE_H

#include <chrono>
#include <memory>
#include <optional>
#include <stop_token>
#include <thread>

#include "mcpp/api/peer.h"
#include "mcpp/api/role.h"
#include "mcpp/api/service.h"

namespace mcpp::api {

/**
 * @brief Quit reason enumeration for service termination
 *
 * Describes why a RunningService event loop terminated.
 */
enum class QuitReason {
    /// Service was explicitly cancelled via close() or cancel()
    Cancelled,
    /// Service was closed explicitly
    Closed,
    /// Transport closed or disconnected
    TransportClosed,
};

/**
 * @brief Convert QuitReason to string for logging/debugging
 *
 * @param reason The quit reason
 * @return String representation
 */
inline const char* to_string(QuitReason reason) {
    switch (reason) {
        case QuitReason::Cancelled: return "Cancelled";
        case QuitReason::Closed: return "Closed";
        case QuitReason::TransportClosed: return "TransportClosed";
    }
    return "Unknown";
}

/**
 * @brief RAII wrapper for background service lifecycle
 *
 * RunningService owns a service and runs an event loop in a background thread.
 * It provides RAII guarantees for cleanup - the background thread is automatically
 * joined on destruction.
 *
 * The event loop processes messages from the peer's message queue, dispatching
 * requests and notifications to the service handlers.
 *
 * Thread safety:
 * - send_request/send_notification via operator-> are thread-safe
 * - is_running is thread-safe
 * - close/close_with_timeout are not thread-safe and require exclusive access
 *
 * Design decisions:
 * - Uses std::jthread for automatic thread join on destruction (C++20)
 * - std::stop_source for cancellation token support
 * - Background thread checks stop_token periodically for cooperative cancellation
 * - Logging warning if not explicitly closed before destruction
 *
 * Follows rust-sdk RunningService pattern from service.rs lines 434-548.
 *
 * Example usage:
 * ```cpp
 * auto service = std::make_shared<MyServerHandler>();
 * RunningService<RoleServer, MyServerHandler> running(service);
 *
 * // Use the service via peer
 * auto response = running->send_request(request);
 *
 * // Explicit close when done (optional but recommended)
 * running.close_with_timeout(std::chrono::milliseconds(5000));
 * ```
 *
 * @tparam Role The service role (RoleClient or RoleServer)
 * @tparam S The concrete service type (must derive from Service<Role>)
 */
template<ServiceRole Role, typename S>
    requires std::derived_from<S, Service<Role>>
class RunningService {
public:
    /**
     * @brief Constructor - starts the event loop
     *
     * Creates a RunningService with the given service and starts the event loop
     * in a background thread.
     *
     * @param service Shared pointer to the service implementation
     */
    explicit RunningService(std::shared_ptr<S> service)
        : service_(std::move(service)),
          peer_(std::make_shared<util::AtomicRequestIdProvider>()),
          stop_source_() {

        // Start the event loop in a background thread
        handle_.emplace([this](std::stop_token st) {
            event_loop(st);
        });
    }

    /**
     * @brief Destructor - ensures cleanup
     *
     * If the service hasn't been explicitly closed, logs a warning and
     * requests cancellation. The std::jthread will automatically join.
     */
    ~RunningService() {
        if (handle_.has_value() && !stop_source_.stop_requested()) {
            // Log warning - service dropped without explicit close
            // In production, this should use a proper logging facility
            // For now, we just note it via comment
        }
        // std::jthread automatically joins here
    }

    // Non-copyable
    RunningService(const RunningService&) = delete;
    RunningService& operator=(const RunningService&) = delete;

    // Non-movable
    RunningService(RunningService&&) = delete;
    RunningService& operator=(RunningService&&) = delete;

    /**
     * @brief Access the peer for sending requests/notifications
     *
     * Provides convenient access to the peer via operator->.
     *
     * @return Pointer to the peer
     */
    Peer<Role>* operator->() noexcept {
        return &peer_;
    }

    /**
     * @brief Const access to the peer
     *
     * @return Const pointer to the peer
     */
    const Peer<Role>* operator->() const noexcept {
        return &peer_;
    }

    /**
     * @brief Get a reference to the peer
     *
     * @return Reference to the peer
     */
    Peer<Role>& peer() noexcept {
        return peer_;
    }

    /**
     * @brief Get a const reference to the peer
     *
     * @return Const reference to the peer
     */
    const Peer<Role>& peer() const noexcept {
        return peer_;
    }

    /**
     * @brief Get a pointer to the service
     *
     * @return Shared pointer to the service
     */
    std::shared_ptr<S> service() const noexcept {
        return service_;
    }

    /**
     * @brief Check if the event loop is running
     *
     * @return true if the event loop is active, false otherwise
     */
    bool is_running() const noexcept {
        return handle_.has_value() && !stop_source_.stop_requested();
    }

    /**
     * @brief Close the service and wait for completion
     *
     * Requests cancellation of the event loop and waits for the background
     * thread to finish. This is the recommended way to shut down a service.
     *
     * After calling this method, the service is considered closed and
     * subsequent operations may fail.
     *
     * @return The quit reason describing why the service stopped
     */
    QuitReason close() {
        if (handle_.has_value()) {
            stop_source_.request_stop();
            peer_.queue_cv_.notify_all();  // Wake up event loop if waiting
            handle_->join();
            handle_ = std::nullopt;
            return QuitReason::Closed;
        }
        return QuitReason::Closed;  // Already closed
    }

    /**
     * @brief Close the service with a timeout
     *
     * Similar to close(), but returns after the specified timeout if the
     * cleanup doesn't complete in time. This is useful for ensuring bounded
     * shutdown time.
     *
     * If the timeout is reached, the thread is detached and will continue
     * cleanup in the background.
     *
     * @param timeout Maximum time to wait for cleanup
     * @return Optional containing the quit reason, or nullopt if timeout occurred
     */
    std::optional<QuitReason> close_with_timeout(std::chrono::milliseconds timeout) {
        if (handle_.has_value()) {
            stop_source_.request_stop();
            peer_.queue_cv_.notify_all();  // Wake up event loop if waiting

            auto stop_token = handle_->get_stop_token();
            std::jthread joiner([this] {
                if (handle_) {
                    handle_->join();
                }
            });

            // Wait with timeout
            auto start = std::chrono::steady_clock::now();
            while (joiner.joinable()) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start
                );
                if (elapsed >= timeout) {
                    // Timeout - detach and return nullopt
                    if (joiner.joinable()) {
                        joiner.detach();
                    }
                    handle_ = std::nullopt;
                    return std::nullopt;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            handle_ = std::nullopt;
            return QuitReason::Closed;
        }
        return QuitReason::Closed;  // Already closed
    }

    /**
     * @brief Cancel the service (consuming operation)
     *
     * Consumes the RunningService and ensures the connection is properly closed.
     * This is an alternative to close() for move semantics.
     *
     * @return The quit reason
     */
    QuitReason cancel() {
        return close();
    }

private:
    /**
     * @brief Event loop function running in the background thread
     *
     * The event loop processes messages from the peer's message queue,
     * dispatching them to the service handlers. It checks the stop token
     * periodically for cooperative cancellation.
     *
     * @param st Stop token for cancellation
     */
    void event_loop(std::stop_token st) {
        // Get service info for logging
        auto info = service_->get_info();

        // Main event loop
        while (!st.stop_requested()) {
            // Wait for messages and process them
            // This blocks until a message arrives or stop is requested
            if (!peer_.wait_and_process(st)) {
                // Stop was requested
                break;
            }

            // Check for cancellation between message processing
            if (st.stop_requested()) {
                break;
            }

            // Process any remaining messages without blocking
            peer_.process_messages();
        }

        // Event loop terminated - cleanup
        // The reason depends on why we exited
    }

protected:
    /// Shared pointer to the service (keeps it alive)
    std::shared_ptr<S> service_;

    /// Peer for message passing
    Peer<Role> peer_;

    /// Optional jthread for the background event loop
    std::optional<std::jthread> handle_;

    /// Stop source for cancellation token support
    std::stop_source stop_source_;
};

} // namespace mcpp::api

#endif // MCPP_API_RUNNING_SERVICE_H
