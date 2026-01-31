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
