#ifndef MCPP_CORE_REQUEST_TRACKER_H
#define MCPP_CORE_REQUEST_TRACKER_H

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "json_rpc.h"
#include "error.h"

namespace mcpp::core {

// Forward declaration - ResponseCallback will be defined in async/callbacks.h
// Using std::function directly here for now
using ResponseCallback = std::function<void(const JsonValue&)>;

/**
 * Pending request storage
 *
 * Holds the callbacks and timestamp for a request awaiting response.
 * When a response arrives, the appropriate callback is invoked.
 */
struct PendingRequest {
    ResponseCallback on_success;
    std::function<void(const JsonRpcError&)> on_error;
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * Library-managed request ID generator and pending request tracker
 *
 * This class provides:
 * - Atomic request ID generation (lock-free)
 * - Thread-safe pending request storage (mutex-protected)
 * - Request completion and cancellation
 *
 * Pattern from RESEARCH.md: Library-Managed Request Tracking
 * - User code never chooses request IDs
 * - Library generates unique IDs via atomic counter
 * - Pending requests tracked in mutex-protected unordered_map
 *
 * Thread safety:
 * - next_id() is lock-free (uses std::atomic)
 * - All other methods use mutex for pending map access
 */
class RequestTracker {
public:
    RequestTracker() = default;

    // Non-copyable, non-movable
    RequestTracker(const RequestTracker&) = delete;
    RequestTracker& operator=(const RequestTracker&) = delete;
    RequestTracker(RequestTracker&&) = delete;
    RequestTracker& operator=(RequestTracker&&) = delete;

    ~RequestTracker() = default;

    /**
     * Generate the next unique request ID
     *
     * Thread-safe: uses atomic fetch_add for lock-free increment.
     * Returns the new ID (monotonically increasing within a session).
     *
     * @return New unique request ID
     */
    RequestId next_id();

    /**
     * Register a pending request with its callbacks
     *
     * Stores the request with current timestamp.
     * Thread-safe: uses mutex lock.
     *
     * @param id Request ID (from next_id())
     * @param on_success Callback for successful response
     * @param on_error Callback for error response
     */
    void register_pending(
        RequestId id,
        ResponseCallback on_success,
        std::function<void(const JsonRpcError&)> on_error
    );

    /**
     * Complete a pending request
     *
     * Removes and returns the pending request for the given ID.
     * Returns nullopt if no such pending request exists.
     * Thread-safe: uses mutex lock.
     *
     * @param id Request ID to complete
     * @return Pending request if found, nullopt otherwise
     */
    std::optional<PendingRequest> complete(RequestId id);

    /**
     * Cancel a pending request
     *
     * Removes the pending request without invoking any callback.
     * Use for timeout handling or manual cancellation.
     * Thread-safe: uses mutex lock.
     *
     * @param id Request ID to cancel
     */
    void cancel(RequestId id);

    /**
     * Get the current count of pending requests
     *
     * Thread-safe: uses mutex lock.
     *
     * @return Number of pending requests
     */
    size_t pending_count() const;

private:
    // Atomic counter for lock-free ID generation
    std::atomic<uint64_t> counter_{0};

    // Pending request storage, protected by mutex
    std::unordered_map<RequestId, PendingRequest> pending_;

    // Mutex protecting pending_ map
    mutable std::mutex mutex_;
};

} // namespace mcpp::core

#endif // MCPP_CORE_REQUEST_TRACKER_H
