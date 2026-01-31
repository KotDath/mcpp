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

#ifndef MCPP_SERVER_REQUEST_CONTEXT_H
#define MCPP_SERVER_REQUEST_CONTEXT_H

#include <chrono>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "mcpp/transport/transport.h"
#include "mcpp/util/sse_formatter.h"

namespace mcpp {
namespace server {

/**
 * @brief Default timeout for requests (5 minutes per MCP spec)
 *
 * Long-running operations should send progress notifications to reset
 * the timeout clock and prevent premature timeout (UTIL-02).
 */
inline constexpr std::chrono::milliseconds DEFAULT_REQUEST_TIMEOUT{300000}; // 5 minutes

/**
 * @brief Context object passed to handlers for progress reporting and streaming
 *
 * RequestContext provides handlers with access to the transport layer
 * for sending notifications and encapsulates the progress token from
 * the request metadata.
 *
 * This enables handlers to report progress for long-running operations
 * via the MCP notifications/progress mechanism, and to send incremental
 * results via the streaming support.
 *
 * UTIL-02: Progress notifications reset the timeout clock to prevent
 * long-running operations from timing out. When a progress notification
 * with a matching progress_token is received, the deadline is reset to
 * now + default_timeout.
 *
 * Streaming mode:
 * - When enabled via set_streaming(true), handlers can send incremental results
 * - send_stream_result() sends partial results before completion
 * - For HTTP/SSE transport, results are formatted as SSE events
 * - For stdio transport, results are sent as regular JSON messages
 * - The final tool return value marks completion (no explicit "done" needed)
 *
 * Streaming initialization:
 * - McpServer or tool handler calls set_streaming(true) before tool execution
 * - Typically enabled when client indicates streaming capability in initialization
 * - HTTP transport always supports streaming via SSE
 * - stdio transport uses progress notification mechanism for streaming
 *
 * Typical usage:
 * ```cpp
 * void my_tool_handler(const std::string& name,
 *                      const json& args,
 *                      RequestContext& ctx) {
 *     // Report progress at 25% (also resets timeout)
 *     ctx.report_progress(25.0, "Processing data...");
 *
 *     // Send incremental result
 *     ctx.send_stream_result({{"status", "partial"}, {"data", "..."}}):
 *
 *     // Do work...
 *
 *     // Report progress at 50% (also resets timeout)
 *     ctx.report_progress(50.0, "Still processing...");
 *
 *     // Send another incremental result
 *     ctx.send_stream_result({{"status", "partial"}, {"data", "more..."}}):
 *
 *     // Report completion
 *     ctx.report_progress(100.0, "Done");
 *
 *     // Final return value completes the operation
 *     return {{"content", "Final result"}};
 * }
 * ```
 */
class RequestContext {
public:
    /**
     * @brief Clock type for timeout tracking
     *
     * Uses steady_clock for timeout tracking to be immune to system time changes.
     */
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::milliseconds;

    /**
     * @brief Construct a RequestContext with request ID and transport
     *
     * Initializes the deadline to now + DEFAULT_REQUEST_TIMEOUT.
     *
     * @param request_id The JSON-RPC request ID for this request
     * @param transport Reference to the transport for sending notifications
     * @param default_timeout Optional custom timeout (defaults to 5 minutes)
     */
    RequestContext(
        const std::string& request_id,
        transport::Transport& transport,
        Duration default_timeout = DEFAULT_REQUEST_TIMEOUT
    );

    /**
     * @brief Default destructor
     */
    ~RequestContext() = default;

    // Non-copyable
    RequestContext(const RequestContext&) = delete;
    RequestContext& operator=(const RequestContext&) = delete;

    // Movable
    RequestContext(RequestContext&&) noexcept = default;
    RequestContext& operator=(RequestContext&&) noexcept = default;

    /**
     * @brief Set the progress token from request metadata
     *
     * The progress token is extracted from request._meta.progressToken
     * by the server before calling the handler.
     *
     * @param token The progress token to use for notifications
     */
    void set_progress_token(const std::string& token);

    /**
     * @brief Check if a progress token is set
     *
     * @return true if a progress token is available, false otherwise
     */
    bool has_progress_token() const;

    /**
     * @brief Get the progress token
     *
     * @return Optional containing the progress token if set, empty otherwise
     */
    const std::optional<std::string>& progress_token() const;

    /**
     * @brief Get the request ID
     *
     * @return The JSON-RPC request ID for this request
     */
    const std::string& request_id() const;

    /**
     * @brief Report progress for long-running operations
     *
     * Sends a notifications/progress message to the client with the
     * current progress value and optional message.
     *
     * If no progress token is set, this method is a no-op.
     *
     * UTIL-02: This also resets the timeout deadline to now + default_timeout,
     * preventing long-running operations from timing out while they're actively
     * sending progress updates.
     *
     * @param progress Progress value from 0 to 100 (percentage)
     * @param message Optional status message to include with the progress update
     *
     * @note Progress values should be in the range 0-100. Values outside
     *       this range will be clamped.
     */
    void report_progress(
        double progress,
        const std::string& message = ""
    );

    /**
     * @brief Reset the timeout deadline (UTIL-02)
     *
     * Resets the deadline to now + default_timeout. This is called when
     * a progress notification with a matching progress_token is received.
     *
     * Thread-safe: Uses mutex lock for deadline access.
     *
     * This allows long-running operations to keep the request alive by
     * sending periodic progress notifications.
     */
    void reset_timeout_on_progress();

    /**
     * @brief Check if the request timeout has expired
     *
     * @return true if the current time is past the deadline, false otherwise
     *
     * Thread-safe: Uses mutex lock for deadline access.
     */
    bool is_timeout_expired() const;

    /**
     * @brief Get the current deadline
     *
     * @return The time point when this request will timeout
     *
     * Thread-safe: Uses mutex lock for deadline access.
     */
    TimePoint deadline() const;

    /**
     * @brief Get the default timeout duration
     *
     * @return The timeout duration in milliseconds
     */
    Duration default_timeout() const noexcept {
        return default_timeout_;
    }

    /**
     * @brief Get the transport reference
     *
     * Provides access to the transport for sending notifications.
     *
     * @return Reference to the transport
     */
    transport::Transport& transport();

    /**
     * @brief Check if streaming mode is enabled
     *
     * Streaming mode allows handlers to send incremental results
     * before the operation completes.
     *
     * @return true if streaming is enabled, false otherwise
     */
    bool is_streaming() const;

    /**
     * @brief Enable or disable streaming mode
     *
     * When enabled, handlers can use send_stream_result() to send
     * incremental results. Streaming is typically enabled by the server
     * when the client indicates streaming capability.
     *
     * @param enable true to enable streaming, false to disable
     */
    void set_streaming(bool enable);

    /**
     * @brief Send a streaming (incremental) result
     *
     * Sends a partial result to the client before the operation completes.
     * This is useful for long-running tools that generate results over time.
     *
     * The result format:
     * - For HTTP/SSE transport: wrapped in SSE format via SseFormatter
     * - For stdio transport: sent as regular JSON-RPC message
     *
     * If no progress token is available, this method is a no-op.
     *
     * @param partial_result The partial result to send (any JSON-serializable value)
     *
     * @note The final tool return value marks completion. There is no explicit
     *       "done" message - the client receives the final result when the
     *       tool handler returns.
     */
    void send_stream_result(const nlohmann::json& partial_result);

private:
    std::string request_id_;
    transport::Transport& transport_;
    std::optional<std::string> progress_token_;
    bool streaming_ = false;

    /// Default timeout duration (5 minutes by default)
    Duration default_timeout_;

    /// Deadline for request timeout (UTIL-02)
    TimePoint deadline_;

    /// Mutex protecting deadline_ for thread-safe access
    mutable std::mutex deadline_mutex_;
};

} // namespace server
} // namespace mcpp

#endif // MCPP_SERVER_REQUEST_CONTEXT_H
