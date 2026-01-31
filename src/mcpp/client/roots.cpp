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

#include "mcpp/client/roots.h"

namespace mcpp::client {

// ============================================================================
// ListRootsResult
// ============================================================================

JsonValue ListRootsResult::to_json() const {
    // Build JSON array of roots
    JsonValue roots_array = JsonValue::array();

    for (const auto& root : roots) {
        JsonValue root_json;
        root_json["uri"] = root.uri;

        // Include name only if present
        if (root.name.has_value()) {
            root_json["name"] = root.name.value();
        } else {
            root_json["name"] = "";
        }

        roots_array.push_back(std::move(root_json));
    }

    // Return in MCP roots/list response format
    JsonValue result;
    result["roots"] = std::move(roots_array);
    return result;
}

// ============================================================================
// RootsManager
// ============================================================================

void RootsManager::set_roots(std::vector<Root> roots) {
    roots_ = std::move(roots);
}

void RootsManager::set_notify_callback(NotifyCallback cb) {
    notify_cb_ = std::move(cb);
}

void RootsManager::notify_changed() {
    // Send notification if callback is registered
    if (notify_cb_) {
        notify_cb_();
    }
}

} // namespace mcpp::client
