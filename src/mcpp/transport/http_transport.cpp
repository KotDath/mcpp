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

#include "mcpp/transport/http_transport.h"

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <thread>

namespace mcpp {
namespace transport {

namespace {
    // Cryptographically secure random number generator
    std::random_device& get_random_device() {
        static std::random_device rd;
        return rd;
    }

    // Hex character set for UUID generation
    constexpr char hex_chars[] = "0123456789abcdef";
} // anonymous namespace

HttpTransport::~HttpTransport() {
    disconnect();
}

bool HttpTransport::connect() {
    // Create a new session for this transport
    current_session_id_ = create_session();
    return !current_session_id_.empty();
}

void HttpTransport::disconnect() {
    // Clear current session
    if (!current_session_id_.empty()) {
        terminate_session(current_session_id_);
        current_session_id_.clear();
    }

    // Clear message buffer
    message_buffer_.clear();
}

bool HttpTransport::is_connected() const {
    return !current_session_id_.empty() && sessions_.find(current_session_id_) != sessions_.end();
}

bool HttpTransport::send(std::string_view message) {
    if (current_session_id_.empty()) {
        if (error_callback_) {
            error_callback_("Cannot send: no active session");
        }
        return false;
    }

    // Find session and buffer the message (non-blocking)
    auto it = sessions_.find(current_session_id_);
    if (it == sessions_.end()) {
        if (error_callback_) {
            error_callback_("Cannot send: session not found");
        }
        return false;
    }

    // Buffer message for SSE delivery (non-blocking)
    it->second.pending_messages.push_back(std::string(message));
    ++last_event_id_;

    return true;
}

void HttpTransport::set_message_callback(MessageCallback cb) {
    message_callback_ = std::move(cb);
}

void HttpTransport::set_error_callback(ErrorCallback cb) {
    error_callback_ = std::move(cb);
}

void HttpTransport::send_notification(const nlohmann::json& notification) {
    if (current_session_id_.empty()) {
        if (error_callback_) {
            error_callback_("Cannot send notification: no active session");
        }
        return;
    }

    auto it = sessions_.find(current_session_id_);
    if (it == sessions_.end()) {
        if (error_callback_) {
            error_callback_("Cannot send notification: session not found");
        }
        return;
    }

    // Format as SSE event and buffer
    std::string event_id = std::to_string(last_event_id_);
    std::string sse_data = util::SseFormatter::format_event(notification, event_id);
    it->second.pending_messages.push_back(sse_data);
    ++last_event_id_;
}

std::string HttpTransport::create_session() {
    // Generate UUID v4 using cryptographically secure random
    // Format: 8-4-4-4-12 hex digits (32 total)
    std::ostringstream oss;
    std::mt19937_64 generator(get_random_device()());
    std::uniform_int_distribution<uint64_t> distribution;

    // Generate 128 random bits
    uint64_t r1 = distribution(generator);
    uint64_t r2 = distribution(generator);

    // Format as UUID v4 (version 4 = random, variant 1 = RFC 4122)
    // xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx where y in {8,9,A,B}
    oss << std::hex << std::setfill('0');
    oss << std::setw(8) << (r1 >> 32) << "-";
    oss << std::setw(4) << ((r1 >> 16) & 0xFFFF) << "-";
    oss << "4" << std::setw(3) << ((r1 >> 8) & 0xFFF) << "-";  // Version 4
    uint64_t y = (r1 & 0xF);
    y = (y & 0x3) | 0x8;  // Variant 1: 10xx
    oss << y << std::setw(3) << ((r2 >> 48) & 0xFFF) << "-";
    oss << std::setw(12) << (r2 & 0xFFFFFFFFFFFFULL);

    std::string session_id = oss.str();

    // Store session data
    SessionData data;
    data.session_id = session_id;
    data.last_activity = std::chrono::steady_clock::now();
    data.last_event_id = 0;
    sessions_[session_id] = std::move(data);

    return session_id;
}

bool HttpTransport::validate_session(const std::string& session_id) {
    // Clean up expired sessions first
    cleanup_expired_sessions();

    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return false;
    }

    // Check session timeout
    auto now = std::chrono::steady_clock::now();
    auto inactive_duration = std::chrono::duration_cast<std::chrono::minutes>(
        now - it->second.last_activity
    );

    if (inactive_duration >= SESSION_TIMEOUT) {
        sessions_.erase(it);
        return false;
    }

    // Update activity timestamp
    it->second.last_activity = now;
    return true;
}

bool HttpTransport::terminate_session(const std::string& session_id) {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return false;
    }

    sessions_.erase(it);
    return true;
}

void HttpTransport::cleanup_expired_sessions() {
    auto now = std::chrono::steady_clock::now();

    for (auto it = sessions_.begin(); it != sessions_.end();) {
        auto inactive_duration = std::chrono::duration_cast<std::chrono::minutes>(
            now - it->second.last_activity
        );

        if (inactive_duration >= SESSION_TIMEOUT) {
            if (error_callback_) {
                std::string error = "Session timeout: " + it->first;
                error_callback_(error);
            }
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace transport
} // namespace mcpp
