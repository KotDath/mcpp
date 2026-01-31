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

#include <future>
#include <memory>
#include <shared_mutex>
#include <optional>

#include "mcpp/api/role.h"
#include "mcpp/api/service.h"
#include "mcpp/util/atomic_id.h"

namespace mcpp::api {

/**
 * @brief Peer represents a connection to a remote MCP endpoint
 *
 * Peer<Role> encapsulates connection state with thread-safe access using
 * std::shared_mutex for reader-writer lock semantics. It provides methods
 * for sending requests and notifications to the remote peer.
 *
 * Thread safety:
 * - Multiple threads can safely call const methods concurrently (peer_info)
 * - send_request and send_notification are thread-safe but have stub implementations
 *   for this foundation plan (full implementation in 06-02)
 *
 * Design decisions:
 * - Uses shared AtomicRequestIdProvider for lock-free ID generation
 * - std::shared_mutex allows concurrent reads with exclusive writes for peer_info
 * - Stub implementations return std::future for async API compatibility
 *
 * Follows rust-sdk pattern from service.rs lines 312-347.
 *
 * @tparam Role The service role (RoleClient or RoleServer)
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
     * @brief Send a request to the remote peer (stub implementation)
     *
     * TODO: In plan 06-02, this will be implemented to:
     * - Generate a request ID using id_provider_
     * - Serialize the request to JSON-RPC format
     * - Send via the transport layer
     * - Return a std::future for async response handling
     *
     * Thread-safe: Can be called concurrently from multiple threads.
     *
     * @param request The request to send
     * @return Future that will contain the response
     */
    std::future<PeerResp> send_request(const PeerReq& request) {
        // Stub implementation for foundation
        // In 06-02, this will:
        // 1. Generate request ID from id_provider_
        // 2. Serialize request to JSON-RPC
        // 3. Send via transport
        // 4. Return future for response
        (void)request;
        std::promise<PeerResp> p;
        auto f = p.get_future();
        // Set dummy value for now - will be properly implemented in 06-02
        // p.set_value(...);
        return f;
    }

    /**
     * @brief Send a notification to the remote peer (stub implementation)
     *
     * TODO: In plan 06-02, this will be implemented to:
     * - Serialize the notification to JSON-RPC format
     * - Send via the transport layer
     * - Return void (notifications don't expect responses)
     *
     * Thread-safe: Can be called concurrently from multiple threads.
     *
     * @param notification The notification to send
     */
    void send_notification(const PeerNot& notification) {
        // Stub implementation for foundation
        // In 06-02, this will:
        // 1. Serialize notification to JSON-RPC
        // 2. Send via transport
        (void)notification;
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
    /// Shared atomic ID provider for generating unique request IDs
    std::shared_ptr<util::AtomicRequestIdProvider> id_provider_;

    /// Mutex protecting peer_info_ (shared_mutex for reader-writer semantics)
    mutable std::shared_mutex mutex_;

    /// Information about the remote peer (set after initialization)
    std::optional<PeerInfo> peer_info_;
};

} // namespace mcpp::api

#endif // MCPP_API_PEER_H
