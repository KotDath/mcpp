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

#include "mcpp/async/timeout.h"

#include <utility>

namespace mcpp::async {

TimeoutManager::TimeoutManager(std::chrono::milliseconds default_timeout)
    : default_timeout_(default_timeout) {
    // deadlines_ map starts empty
}

void TimeoutManager::set_timeout(RequestId id,
                                 std::chrono::milliseconds timeout,
                                 TimeoutCallback on_timeout) {
    TimePoint deadline = Clock::now() + timeout;

    std::lock_guard<std::mutex> lock(mutex_);
    deadlines_[id] = TimeoutEntry{deadline, std::move(on_timeout)};
}

void TimeoutManager::cancel(RequestId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    deadlines_.erase(id);
}

std::vector<RequestId> TimeoutManager::check_timeouts() {
    TimePoint now = Clock::now();
    std::vector<RequestId> expired_ids;
    std::vector<TimeoutCallback> expired_callbacks;

    // Collect expired entries while holding the lock
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Find all entries that have expired
        for (auto& [id, entry] : deadlines_) {
            if (now >= entry.deadline) {
                expired_ids.push_back(id);
            }
        }

        // Extract callbacks for expired entries and remove them from map
        expired_callbacks.reserve(expired_ids.size());
        for (const auto& id : expired_ids) {
            auto it = deadlines_.find(id);
            if (it != deadlines_.end()) {
                expired_callbacks.push_back(std::move(it->second.callback));
                deadlines_.erase(it);
            }
        }
    }
    // Lock released here

    // Invoke callbacks outside the lock to prevent deadlock
    // if a callback calls back into TimeoutManager (e.g., cancel())
    for (size_t i = 0; i < expired_ids.size(); ++i) {
        if (expired_callbacks[i]) {
            expired_callbacks[i](expired_ids[i]);
        }
    }

    return expired_ids;
}

bool TimeoutManager::has_timeout(const RequestId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return deadlines_.find(id) != deadlines_.end();
}

size_t TimeoutManager::pending_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return deadlines_.size();
}

} // namespace mcpp::async
