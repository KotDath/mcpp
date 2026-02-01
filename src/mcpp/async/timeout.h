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

#ifndef MCPP_ASYNC_TIMEOUT_H
#define MCPP_ASYNC_TIMEOUT_H

#include <chrono>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "mcpp/async/callbacks.h"

namespace mcpp::async {

/**
 * @brief Manages timeout tracking for pending JSON-RPC requests
 *
 * The TimeoutManager tracks deadlines for pending requests and provides
 * a mechanism to check for expired requests. This enables automatic
 * cleanup of requests that take too long to complete.
 *
 * Key design decisions:
 * - Uses steady_clock for timeout calculations (unaffected by system time changes)
 * - Thread-safe: all operations are mutex-protected
 * - Callbacks invoked AFTER mutex release to prevent deadlock
 * - Returns list of expired IDs for cleanup coordination
 *
 * Usage pattern:
 *   1. Set timeout when sending request: set_timeout(id, timeout_ms, callback)
 *   2. Periodically check: auto expired = timeout_manager.check_timeouts()
 *   3. Clean up both timeout and request tracking for expired IDs
 *   4. Cancel timeout when response arrives: cancel(id)
 *
 * Thread safety: Safe for concurrent use from any thread.
 */
class TimeoutManager {
public:
    /**
     * Clock type for timeout calculations
     *
     * steady_clock is used because it is monotonic and unaffected by
     * system time changes. This ensures timeout accuracy even if the
     * system clock is adjusted.
     */
    using Clock = std::chrono::steady_clock;

    /**
     * Duration type for timeouts
     */
    using Duration = Clock::duration;

    /**
     * Time point type for deadline tracking
     */
    using TimePoint = Clock::time_point;

    /**
     * Construct a TimeoutManager with a default timeout duration
     *
     * @param default_timeout The default timeout to use when not explicitly specified
     *
     * The default timeout is used as a fallback when registering timeouts.
     * Common values: 30s for MCP tool calls, 60s for long operations.
     */
    explicit TimeoutManager(std::chrono::milliseconds default_timeout);

    /**
     * @brief Default destructor
     *
     * Note: The TimeoutManager does NOT automatically invoke pending callbacks
     * on destruction. Callers should ensure all pending requests are cleaned up
     * before destroying the manager (e.g., by calling cancel() for each).
     */
    ~TimeoutManager() = default;

    // Non-copyable, non-movable (mutex cannot be copied/moved)
    TimeoutManager(const TimeoutManager&) = delete;
    TimeoutManager& operator=(const TimeoutManager&) = delete;
    TimeoutManager(TimeoutManager&&) = delete;
    TimeoutManager& operator=(TimeoutManager&&) = delete;

    /**
     * Register a timeout for a request
     *
     * @param id The request ID to track
     * @param timeout Duration after which the request should time out
     * @param on_timeout Callback to invoke when timeout expires
     *
     * If a timeout already exists for this ID, it will be replaced
     * with the new timeout and callback.
     *
     * Thread safety: Safe to call concurrently with any other method.
     */
    void set_timeout(RequestId id,
                     std::chrono::milliseconds timeout,
                     TimeoutCallback on_timeout);

    /**
     * Cancel the timeout for a request
     *
     * @param id The request ID whose timeout should be cancelled
     *
     * If no timeout exists for this ID, this is a no-op.
     *
     * This should be called when a response is received for the request,
     * to prevent the timeout from firing later.
     *
     * Thread safety: Safe to call concurrently with any other method.
     */
    void cancel(RequestId id);

    /**
     * Check for expired timeouts
     *
     * @return List of request IDs that have timed out
     *
     * For each expired request:
     * 1. The callback is extracted and the entry removed from the map
     * 2. The mutex is released
     * 3. The callback is invoked (outside the lock to prevent deadlock)
     *
     * The returned list of IDs allows the caller to coordinate cleanup
     * with other request tracking structures (e.g., RequestTracker).
     *
     * Thread safety: Safe to call concurrently with any other method.
     *
     * Example usage:
     *   auto expired = timeout_manager.check_timeouts();
     *   for (const auto& id : expired) {
     *       request_tracker.remove(id);
     *   }
     */
    std::vector<RequestId> check_timeouts();

    /**
     * Get the default timeout duration
     *
     * @return The default timeout in milliseconds
     *
     * Thread safety: Safe to call concurrently.
     */
    std::chrono::milliseconds default_timeout() const {
        return default_timeout_;
    }

    /**
     * Check if a timeout exists for a request ID
     *
     * @param id The request ID to check
     * @return true if a timeout is registered for this ID
     *
     * Thread safety: Safe to call concurrently.
     */
    bool has_timeout(const RequestId& id) const;

    /**
     * Get the number of pending timeouts
     *
     * @return Number of requests currently being tracked
     *
     * Useful for debugging and monitoring.
     *
     * Thread safety: Safe to call concurrently.
     */
    size_t pending_count() const;

private:
    /**
     * Internal representation of a timeout entry
     */
    struct TimeoutEntry {
        TimePoint deadline;       ///< When this timeout expires
        TimeoutCallback callback; ///< What to do when it expires
    };

    /// Default timeout duration for new timeouts
    std::chrono::milliseconds default_timeout_;

    /// Map of request IDs to their timeout entries
    std::unordered_map<RequestId, TimeoutEntry> deadlines_;

    /// Mutex protecting deadlines_ map
    mutable std::mutex mutex_;
};

} // namespace mcpp::async

#endif // MCPP_ASYNC_TIMEOUT_H
