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

#include "request_tracker.h"

namespace mcpp::core {

RequestId RequestTracker::next_id() {
    // Atomic increment with sequential consistency
    // Returns the value before increment (0, 1, 2, ...)
    return static_cast<int64_t>(counter_.fetch_add(1, std::memory_order_seq_cst));
}

void RequestTracker::register_pending(
    RequestId id,
    ResponseCallback on_success,
    std::function<void(const JsonRpcError&)> on_error
) {
    std::lock_guard<std::mutex> lock(mutex_);

    pending_[id] = PendingRequest{
        .on_success = std::move(on_success),
        .on_error = std::move(on_error),
        .timestamp = std::chrono::steady_clock::now()
    };
}

std::optional<PendingRequest> RequestTracker::complete(RequestId id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pending_.find(id);
    if (it == pending_.end()) {
        return std::nullopt;
    }

    // Move the PendingRequest out before erasing to avoid callback copies
    PendingRequest request = std::move(it->second);
    pending_.erase(it);

    return request;
}

void RequestTracker::cancel(RequestId id) {
    std::lock_guard<std::mutex> lock(mutex_);

    pending_.erase(id);
}

size_t RequestTracker::pending_count() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return pending_.size();
}

} // namespace mcpp::core
