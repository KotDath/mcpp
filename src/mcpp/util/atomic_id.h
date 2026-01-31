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

#ifndef MCPP_UTIL_ATOMIC_ID_H
#define MCPP_UTIL_ATOMIC_ID_H

#include <atomic>
#include <cstdint>
#include <memory>

namespace mcpp::util {

/**
 * @brief Lock-free atomic ID provider for request ID generation
 *
 * AtomicRequestIdProvider generates sequential request IDs without requiring
 * locks, using std::atomic with fetch_add and relaxed memory ordering.
 *
 * This is designed for sharing between Peer and RequestContext via shared_ptr,
 * allowing multiple components to generate unique request IDs concurrently.
 *
 * Thread safety: All methods are thread-safe and can be called concurrently
 * from multiple threads.
 *
 * Design decisions:
 * - Uses memory_order_relaxed for maximum performance (ID ordering is not
 *   required for correctness, only uniqueness)
 * - Starts at 1 (0 is reserved for special cases in JSON-RPC)
 * - Non-copyable and non-movable to prevent accidental ID collisions
 *
 * Follows the rust-sdk AtomicU32Provider pattern from service.rs.
 *
 * Example usage:
 *   auto provider = std::make_shared<AtomicRequestIdProvider>();
 *   uint32_t id1 = provider->next_id();  // Returns 1
 *   uint32_t id2 = provider->next_id();  // Returns 2
 */
class AtomicRequestIdProvider {
public:
    /**
     * @brief Default constructor
     *
     * Initializes the next ID to 1 (0 is reserved).
     */
    AtomicRequestIdProvider() noexcept = default;

    /**
     * @brief Get the next unique ID
     *
     * Atomically increments and returns the next ID.
     * Uses memory_order_relaxed for maximum performance since
     * strict ordering is not required for request ID generation.
     *
     * Thread-safe: Can be called concurrently from multiple threads.
     *
     * @return The next unique ID (starts at 1, increments monotonically)
     */
    uint32_t next_id() noexcept {
        return next_id_.fetch_add(1, std::memory_order_relaxed);
    }

    // Non-copyable - copying would create ID collisions
    AtomicRequestIdProvider(const AtomicRequestIdProvider&) = delete;
    AtomicRequestIdProvider& operator=(const AtomicRequestIdProvider&) = delete;

    // Non-movable - moving would create ID collisions
    AtomicRequestIdProvider(AtomicRequestIdProvider&&) = delete;
    AtomicRequestIdProvider& operator=(AtomicRequestIdProvider&&) = delete;

    /**
     * @brief Virtual destructor for proper cleanup
     */
    ~AtomicRequestIdProvider() = default;

private:
    /// Next ID to allocate, starts at 1 (0 is reserved)
    std::atomic<uint32_t> next_id_{1};
};

} // namespace mcpp::util

#endif // MCPP_UTIL_ATOMIC_ID_H
