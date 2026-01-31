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

#include "mcpp/client/cancellation.h"

#include <cstdio>
#include <variant>

namespace mcpp::client {

void CancellationManager::register_request(core::RequestId id, CancellationSource source) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.emplace(std::move(id), std::move(source));
}

void CancellationManager::handle_cancelled(core::RequestId id, const std::optional<std::string>& reason) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pending_.find(id);
    if (it != pending_.end()) {
        // Request is still pending - cancel it
        it->second.cancel();
        pending_.erase(it);

        // Log reason if provided (for debugging)
        if (reason) {
            std::fprintf(stderr, "[mcpp] Request cancelled: %s\n", reason->c_str());
        }
    }
    // If not found, request already completed - race condition handled (no-op)
}

void CancellationManager::unregister_request(core::RequestId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.erase(id);
    // No error if not present (already unregistered or cancelled)
}

size_t CancellationManager::pending_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_.size();
}

} // namespace mcpp::client
