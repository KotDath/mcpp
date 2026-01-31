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

#include <optional>
#include <string>

#include "mcpp/transport/transport.h"

namespace mcpp {
namespace server {

/**
 * @brief Context object passed to handlers for progress reporting
 *
 * RequestContext provides handlers with access to the transport layer
 * for sending notifications and encapsulates the progress token from
 * the request metadata.
 *
 * This enables handlers to report progress for long-running operations
 * via the MCP notifications/progress mechanism.
 *
 * Typical usage:
 * ```cpp
 * void my_tool_handler(const std::string& name,
 *                      const json& args,
 *                      RequestContext& ctx) {
 *     // Report progress at 25%
 *     ctx.report_progress(25.0, "Processing data...");
 *
 *     // Do work...
 *
 *     // Report progress at 50%
 *     ctx.report_progress(50.0, "Still processing...");
 *
 *     // Report completion
 *     ctx.report_progress(100.0, "Done");
 * }
 * ```
 */
class RequestContext {
public:
    /**
     * @brief Construct a RequestContext with request ID and transport
     *
     * @param request_id The JSON-RPC request ID for this request
     * @param transport Reference to the transport for sending notifications
     */
    RequestContext(
        const std::string& request_id,
        transport::Transport& transport
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
     * @brief Get the transport reference
     *
     * Provides access to the transport for sending notifications.
     *
     * @return Reference to the transport
     */
    transport::Transport& transport();

private:
    std::string request_id_;
    transport::Transport& transport_;
    std::optional<std::string> progress_token_;
};

} // namespace server
} // namespace mcpp

#endif // MCPP_SERVER_REQUEST_CONTEXT_H
