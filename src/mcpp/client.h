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

#ifndef MCPP_CLIENT_H
#define MCPP_CLIENT_H

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "mcpp/async/callbacks.h"
#include "mcpp/async/timeout.h"
#include "mcpp/client/cancellation.h"
#include "mcpp/client/elicitation.h"
#include "mcpp/client/roots.h"
#include "mcpp/client/sampling.h"
#include "mcpp/core/json_rpc.h"
#include "mcpp/core/request_tracker.h"
#include "mcpp/protocol/initialize.h"
#include "mcpp/transport/transport.h"

namespace mcpp {

// Bring JsonValue into scope (defined in core::JsonValue and client::JsonValue)
using core::JsonValue;

/**
 * @brief Low-level callback-based MCP client
 *
 * McpClient is the main entry point for library users to interact with an MCP server.
 * It integrates all protocol foundation components:
 * - Transport abstraction for sending/receiving messages
 * - RequestTracker for library-managed request ID generation
 * - TimeoutManager for request timeout handling
 * - Protocol types for MCP initialization handshake
 *
 * Key design decisions:
 * - Client owns the transport via unique_ptr (manages lifecycle)
 * - Request IDs are generated automatically (user never provides IDs)
 * - All async operations use std::function callbacks
 * - Explicit lifecycle (connect/disconnect), not RAII-based
 *
 * Thread safety: Not thread-safe. Use external synchronization if calling
 * from multiple threads.
 *
 * Usage:
 *   auto transport = std::make_unique<StdioTransport>();
 *   McpClient client(std::move(transport));
 *
 *   client.connect();
 *
 *   client.initialize(
 *       InitializeRequestParams{...},
 *       [](const InitializeResult& result) {
 *           std::cout << "Initialized with " << result.serverInfo.name << std::endl;
 *       },
 *       [](const JsonRpcError& error) {
 *           std::cerr << "Init failed: " << error.message << std::endl;
 *       }
 *   );
 *
 *   // ... send requests, register handlers ...
 *
 *   client.disconnect();
 */
class McpClient {
public:
    /**
     * @brief Callback type for handling incoming server requests
     *
     * The handler returns a JsonValue result that will be sent back
     * as a response to the server's request.
     */
    using RequestHandler = std::function<JsonValue(std::string_view method, const JsonValue& params)>;

    /**
     * @brief Callback type for handling incoming server notifications
     *
     * Notifications are one-way messages that do not expect a response.
     */
    using NotificationHandler = std::function<void(std::string_view method, const JsonValue& params)>;

    /**
     * @brief Construct a McpClient with a transport implementation
     *
     * The client takes ownership of the transport. After construction,
     * call connect() to establish the connection.
     *
     * @param transport The transport implementation (stdio, SSE, WebSocket, etc.)
     * @param default_timeout Default timeout for requests (30 seconds by default)
     */
    explicit McpClient(
        std::unique_ptr<transport::Transport> transport,
        std::chrono::milliseconds default_timeout = std::chrono::milliseconds(30000)
    );

    /**
     * @brief Destructor - disconnects and cleans up transport
     *
     * If still connected, disconnects the transport before destruction.
     * All pending requests are cancelled (callbacks are not invoked).
     */
    ~McpClient();

    // Non-copyable, non-movable (owns unique_ptr to transport)
    McpClient(const McpClient&) = delete;
    McpClient& operator=(const McpClient&) = delete;
    McpClient(McpClient&&) = delete;
    McpClient& operator=(McpClient&&) = delete;

    /**
     * @brief Establish the transport connection
     *
     * Delegates to the underlying transport's connect() method.
     * Must be called before sending any requests or notifications.
     *
     * @return true if connection succeeded, false otherwise
     */
    bool connect();

    /**
     * @brief Close the transport connection
     *
     * Gracefully shuts down the transport connection.
     * After this call, is_connected() will return false.
     */
    void disconnect();

    /**
     * @brief Check if the transport is currently connected
     *
     * @return true if connected, false otherwise
     */
    bool is_connected() const;

    /**
     * @brief Cancel a pending request
     *
     * Requests cancellation for the specified request ID.
     * This is useful for user-initiated cancellation (e.g., UI cancel button).
     *
     * If the request is not found (already completed), this is a no-op.
     *
     * @param id Request ID to cancel
     */
    void cancel_request(core::RequestId id);

    /**
     * @brief Send a JSON-RPC request to the server
     *
     * Generates a request ID automatically and registers the pending request.
     * The response will be delivered via the provided callbacks.
     *
     * @param method The RPC method name (e.g., "tools/list")
     * @param params The parameters object (may be null for methods with no params)
     * @param on_success Callback invoked when a successful response is received
     * @param on_error Callback invoked when an error response is received or timeout occurs
     * @param timeout Optional timeout duration (uses default if not specified)
     */
    void send_request(
        std::string_view method,
        const JsonValue& params,
        async::ResponseCallback on_success,
        async::ErrorCallback on_error,
        std::optional<std::chrono::milliseconds> timeout = {}
    );

    /**
     * @brief Send a JSON-RPC notification to the server
     *
     * Notifications are fire-and-forget messages that do not expect a response.
     * No ID is generated and no timeout is tracked.
     *
     * @param method The notification method name (e.g., "notifications/initialized")
     * @param params The parameters object (may be null for notifications with no params)
     */
    void send_notification(std::string_view method, const JsonValue& params);

    /**
     * @brief Register a handler for incoming server requests
     *
     * When the server sends a request (e.g., "ping"), the registered handler
     * will be invoked to generate a response.
     *
     * Only one handler per method name; calling this again replaces the previous handler.
     *
     * @param method The method name to handle
     * @param handler The handler function that returns a result value
     */
    void set_request_handler(std::string_view method, RequestHandler handler);

    /**
     * @brief Register a handler for incoming server notifications
     *
     * When the server sends a notification, the registered handler will be invoked.
     *
     * Only one handler per method name; calling this again replaces the previous handler.
     *
     * @param method The notification method name to handle
     * @param handler The handler function (no return value)
     */
    void set_notification_handler(std::string_view method, NotificationHandler handler);

    /**
     * @brief Perform the MCP initialize handshake
     *
     * Sends an "initialize" request to the server with the provided parameters.
     * On success, the InitializeResult is delivered via on_complete.
     * After receiving a successful initialize result, automatically sends
     * the "initialized" notification to complete the handshake.
     *
     * @param params The initialization request parameters
     * @param on_complete Callback invoked with the server's initialize result
     * @param on_error Callback invoked on error or timeout
     */
    void initialize(
        const protocol::InitializeRequestParams& params,
        std::function<void(const protocol::InitializeResult&)> on_complete,
        async::ErrorCallback on_error
    );

    /**
     * @brief Send the "initialized" notification
     *
     * This should be called after receiving a successful initialize result.
     * The initialize() method calls this automatically, but this method
     * is provided for manual control if needed.
     */
    void send_initialized_notification();

    /**
     * @brief Get the roots manager
     *
     * Provides access to the RootsManager for setting and getting roots.
     *
     * @return Reference to the roots manager
     */
    client::RootsManager& get_roots_manager() { return roots_manager_; }

    /**
     * @brief Get the roots manager (const overload)
     *
     * @return Const reference to the roots manager
     */
    const client::RootsManager& get_roots_manager() const { return roots_manager_; }

    /**
     * @brief Set the sampling handler for LLM text generation requests
     *
     * The handler will be invoked when the server sends a sampling/createMessage request.
     * This enables the server to request LLM completions through the client's LLM provider.
     *
     * @param handler Function that processes sampling requests and returns LLM responses
     */
    void set_sampling_handler(client::SamplingHandler handler);

    /**
     * @brief Enable tool use support for sampling
     *
     * When enabled, the SamplingClient can execute agentic tool loops by calling
     * tools on the MCP server via the tools/call method.
     *
     * This sets up a synchronous tool_caller that blocks on responses using
     * promise/future for tool calls during the tool loop.
     *
     * @param enable True to enable tool use, false to disable
     */
    void enable_tool_use_for_sampling(bool enable = true);

    /**
     * @brief Get the tool loop configuration
     *
     * Provides access to configure max_iterations and timeout for tool loops.
     *
     * @return Reference to the tool loop configuration
     */
    client::ToolLoopConfig& get_tool_loop_config() { return sampling_client_.get_tool_loop_config(); }

    /**
     * @brief Get the tool loop configuration (const overload)
     *
     * @return Const reference to the tool loop configuration
     */
    const client::ToolLoopConfig& get_tool_loop_config() const { return sampling_client_.get_tool_loop_config(); }

    /**
     * @brief Set the elicitation handler for user input requests
     *
     * The handler will be invoked when the server sends an elicitation/create request.
     * This enables the server to request structured user input via forms (CLNT-04)
     * or out-of-band interactions via URLs (CLNT-05).
     *
     * @param handler Function that presents UI and returns user's response
     */
    void set_elicitation_handler(client::ElicitationHandler handler);

private:
    /// Transport layer for sending/receiving messages
    std::unique_ptr<transport::Transport> transport_;

    /// Request ID generator and pending request storage
    core::RequestTracker request_tracker_;

    /// Timeout manager for pending requests
    async::TimeoutManager timeout_manager_;

    /// Default timeout for requests
    std::chrono::milliseconds default_timeout_;

    /// Handlers for incoming server requests (method -> handler)
    std::unordered_map<std::string, RequestHandler> request_handlers_;

    /// Handlers for incoming server notifications (method -> handler)
    std::unordered_map<std::string, NotificationHandler> notification_handlers_;

    /// Roots manager for handling roots/list requests and list_changed notifications
    client::RootsManager roots_manager_;

    /// Sampling client for handling sampling/createMessage requests from server
    client::SamplingClient sampling_client_;

    /// Elicitation client for handling elicitation/create requests from server
    client::ElicitationClient elicitation_client_;

    /// Cancellation manager for handling request cancellation
    client::CancellationManager cancellation_manager_;

    /// Callback invoked by transport when a message is received
    void on_message(std::string_view message);

    /// Callback invoked by transport when an error occurs
    void on_transport_error(std::string_view error);

    /// Handle a JSON-RPC response message
    void handle_response(const core::JsonRpcResponse& response);

    /// Handle a JSON-RPC request message from the server
    void handle_server_request(const core::JsonRpcRequest& request);

    /// Handle a JSON-RPC notification message from the server
    void handle_notification(const core::JsonRpcNotification& notification);

    /// Send a response back to the server
    void send_response(core::RequestId id, const JsonValue& result);

    /// Send an error response back to the server
    void send_error_response(core::RequestId id, const core::JsonRpcError& error);
};

} // namespace mcpp

#endif // MCPP_CLIENT_H
