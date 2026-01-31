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
#include <sstream>
#include <string>

namespace mcpp {
namespace util {

/**
 * @brief Server-Sent Events (SSE) formatter for HTTP transport
 *
 * SseFormatter provides static methods for formatting messages according to
 * the SSE (text/event-stream) specification. This is used by HttpTransport
 * to send messages to clients via HTTP GET with SSE.
 *
 * SSE format per W3C spec:
 * - Each event consists of one or more fields
 * - Each field: "field_name: value\n"
 * - Events terminated by double newline ("\n\n")
 * - Required fields: data (contains the JSON payload)
 * - Optional fields: event (event type), id (event ID for reconnection), retry
 *
 * @note This is a header-only utility with no external SSE library dependency.
 *       The SSE format is simple enough to inline correctly.
 */
class SseFormatter {
public:
    /**
     * @brief Format a JSON-RPC message as an SSE event
     *
     * Creates a properly formatted SSE event with the JSON message as data.
     * The message is serialized using nlohmann::json::dump().
     *
     * Format:
     *   data: {"json":"rpc"}\n
     *   id: 123\n
     *   \n
     *
     * @param message The JSON-RPC message to format
     * @param event_id Optional event ID for reconnection support (Last-Event-ID header)
     * @return Formatted SSE event as a string
     *
     * @note The double newline at the end terminates the event per SSE spec.
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
     * HTTP responses with SSE must set Content-Type to "text/event-stream".
     *
     * @return "text/event-stream"
     */
    static const char* content_type() {
        return "text/event-stream";
    }

    /**
     * @brief Get the Cache-Control header value for SSE responses
     *
     * SSE responses should disable caching to ensure real-time delivery.
     *
     * @return "no-cache"
     */
    static const char* cache_control() {
        return "no-cache";
    }

    /**
     * @brief Get the Connection header value for SSE responses
     *
     * SSE responses should keep the connection open for streaming.
     *
     * @return "keep-alive"
     */
    static const char* connection() {
        return "keep-alive";
    }
};

} // namespace util
} // namespace mcpp

#endif // MCPP_UTIL_SSE_FORMATTER_H
