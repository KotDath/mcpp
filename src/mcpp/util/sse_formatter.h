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

#ifndef MCPP_UTIL_SSE_FORMATTER_H
#define MCPP_UTIL_SSE_FORMATTER_H

#include <nlohmann/json.hpp>
#include <string>
#include <sstream>

namespace mcpp {
namespace util {

/**
 * @brief Server-Sent Events (SSE) formatter for HTTP transport
 *
 * Provides static methods for formatting SSE events according to the
 * text/event-stream specification. SSE is used for server-to-client
 * streaming in the MCP Streamable HTTP transport.
 *
 * Format specification:
 * - Each event has "data:" line(s) containing the JSON payload
 * - Optional "id:" line for event identification
 * - Optional "event:" line for event type naming
 * - Double newline ("\n\n") terminates each event
 *
 * @see https://modelcontextprotocol.io/specification/2025-11-25/basic/transports
 */
class SseFormatter {
public:
    /**
     * @brief Format a JSON-RPC message as an SSE event
     *
     * Creates a properly formatted SSE event with the JSON message as data.
     * The event ID is optional but recommended for reconnection support.
     *
     * @param message The JSON-RPC message to format (typically a notification)
     * @param event_id Optional event identifier for reconnection tracking
     * @return Formatted SSE event string ready to send
     *
     * Example output:
     *   data: {"jsonrpc":"2.0","method":"notifications/message"}
     *   id: 123
     *
     *   (note: trailing blank line is event terminator)
     */
    static std::string format_event(const nlohmann::json& message,
                                    const std::string& event_id = "") {
        std::ostringstream oss;
        oss << "data: " << message.dump() << "\n";
        if (!event_id.empty()) {
            oss << "id: " << event_id << "\n";
        }
        oss << "\n";  // Double newline ends event
        return oss.str();
    }

    /**
     * @brief Get the Content-Type header value for SSE responses
     *
     * @return "text/event-stream"
     */
    static const char* content_type() {
        return "text/event-stream";
    }

    /**
     * @brief Get the Cache-Control header value for SSE responses
     *
     * SSE requires disabling caching to ensure real-time delivery.
     *
     * @return "no-cache"
     */
    static const char* cache_control() {
        return "no-cache";
    }

    /**
     * @brief Get the Connection header value for SSE responses
     *
     * SSE requires persistent connections for streaming.
     *
     * @return "keep-alive"
     */
    static const char* connection() {
        return "keep-alive";
    }

private:
    // Static utility class - no instantiation
    SseFormatter() = delete;
    SseFormatter(const SseFormatter&) = delete;
    SseFormatter& operator=(const SseFormatter&) = delete;
};

} // namespace util
} // namespace mcpp

#endif // MCPP_UTIL_SSE_FORMATTER_H
