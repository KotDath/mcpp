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
