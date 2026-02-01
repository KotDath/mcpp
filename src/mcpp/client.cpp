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

#include "mcpp/client.h"

#include <cstdio>
#include <future>
#include <utility>

namespace mcpp {

namespace {

// Helper to parse RequestId from JSON value
std::optional<core::RequestId> parse_request_id(const nlohmann::json& j) {
    if (j.is_number_integer()) {
        return j.get<int64_t>();
    } else if (j.is_string()) {
        return j.get<std::string>();
    }
    return std::nullopt;
}

} // namespace

namespace {

// Helper to detect if a JSON value looks like a JSON-RPC request
bool is_request(const nlohmann::json& j) {
    return j.contains("method") && j.contains("id");
}

// Helper to detect if a JSON value looks like a JSON-RPC notification
bool is_notification(const nlohmann::json& j) {
    return j.contains("method") && !j.contains("id");
}

// Helper to detect if a JSON value looks like a JSON-RPC response
bool is_response(const nlohmann::json& j) {
    return j.contains("id") && !j.contains("method");
}

} // namespace

// ============================================================================
// Constructor/Destructor
// ============================================================================

McpClient::McpClient(
    std::unique_ptr<transport::Transport> transport,
    std::chrono::milliseconds default_timeout
)
    : transport_(std::move(transport))
    , timeout_manager_(default_timeout)
    , default_timeout_(default_timeout) {
    // Set up transport callbacks
    transport_->set_message_callback([this](std::string_view message) {
        on_message(message);
    });
    transport_->set_error_callback([this](std::string_view error) {
        on_transport_error(error);
    });

    // Set up roots notification callback
    roots_manager_.set_notify_callback([this]() {
        send_notification("notifications/roots/list_changed", nullptr);
    });

    // Register roots/list request handler
    set_request_handler("roots/list",
        [this](std::string_view /* method */, const JsonValue& /* params */) -> JsonValue {
            client::ListRootsResult result;
            result.roots = roots_manager_.get_roots();
            return result.to_json();
        }
    );

    // Register sampling/createMessage request handler
    set_request_handler("sampling/createMessage",
        [this](std::string_view /* method */, const JsonValue& params) -> JsonValue {
            return sampling_client_.handle_create_message(params);
        }
    );

    // Register notifications/cancelled handler for server-side cancellation
    set_notification_handler("notifications/cancelled",
        [this](std::string_view /* method */, const JsonValue& params) {
            std::optional<core::RequestId> request_id;
            if (params.contains("requestId")) {
                request_id = parse_request_id(params["requestId"]);
            }
            std::optional<std::string> reason;
            if (params.contains("reason")) {
                const auto& reason_val = params["reason"];
                if (reason_val.is_string()) {
                    reason = reason_val.get<std::string>();
                }
            }
            if (request_id) {
                cancellation_manager_.handle_cancelled(*request_id, reason);
            }
        }
    );

    // Register elicitation/create request handler
    set_request_handler("elicitation/create",
        [this](std::string_view /* method */, const JsonValue& params) -> JsonValue {
            return elicitation_client_.handle_elicitation_create(params);
        }
    );

    // Register notifications/elicitation/complete handler for URL mode completion
    set_notification_handler("notifications/elicitation/complete",
        [this](std::string_view /* method */, const JsonValue& params) {
            elicitation_client_.handle_elicitation_complete(params);
        }
    );
}

McpClient::~McpClient() {
    if (transport_ && transport_->is_connected()) {
        disconnect();
    }
}

// ============================================================================
// Connection management
// ============================================================================

bool McpClient::connect() {
    return transport_->connect();
}

void McpClient::disconnect() {
    transport_->disconnect();
}

bool McpClient::is_connected() const {
    return transport_->is_connected();
}

void McpClient::cancel_request(core::RequestId id) {
    cancellation_manager_.handle_cancelled(id, std::nullopt);
}

// ============================================================================
// Sending requests
// ============================================================================

void McpClient::send_request(
    std::string_view method,
    const JsonValue& params,
    async::ResponseCallback on_success,
    async::ErrorCallback on_error,
    std::optional<std::chrono::milliseconds> timeout
) {
    // Generate request ID
    core::RequestId id = request_tracker_.next_id();

    // Create cancellation source and register for this request
    client::CancellationSource cancel_source;
    cancellation_manager_.register_request(id, std::move(cancel_source));

    // Create JSON-RPC request
    core::JsonRpcRequest request;
    request.id = id;
    request.method = method;
    request.params = params;

    // Serialize to JSON string
    std::string message = request.to_string();

    // Determine timeout
    std::chrono::milliseconds actual_timeout = timeout.value_or(default_timeout_);

    // Register pending request with callbacks
    request_tracker_.register_pending(
        id,
        std::move(on_success),
        [this, id, on_error = std::move(on_error)](const core::JsonRpcError& error) mutable {
            // Cancel timeout when error callback is invoked
            timeout_manager_.cancel(id);
            if (on_error) {
                on_error(error);
            }
        }
    );

    // Set timeout with callback that invokes error handler
    timeout_manager_.set_timeout(id, actual_timeout,
        [this, id](core::RequestId timeout_id) {
            // Request timed out - remove from pending and invoke error callback
            auto pending = request_tracker_.complete(timeout_id);
            cancellation_manager_.unregister_request(timeout_id);
            if (pending && pending->on_error) {
                core::JsonRpcError timeout_error{
                    core::INTERNAL_ERROR,
                    "Request timed out"
                };
                pending->on_error(timeout_error);
            }
        }
    );

    // Send via transport
    transport_->send(message);
}

void McpClient::send_notification(std::string_view method, const JsonValue& params) {
    // Create JSON-RPC notification
    core::JsonRpcNotification notification;
    notification.method = method;
    notification.params = params;

    // Serialize and send (no response expected, no tracking needed)
    std::string message = notification.to_string();
    transport_->send(message);
}

// ============================================================================
// Handler registration
// ============================================================================

void McpClient::set_request_handler(std::string_view method, RequestHandler handler) {
    request_handlers_[std::string(method)] = std::move(handler);
}

void McpClient::set_notification_handler(std::string_view method, NotificationHandler handler) {
    notification_handlers_[std::string(method)] = std::move(handler);
}

void McpClient::set_sampling_handler(client::SamplingHandler handler) {
    sampling_client_.set_sampling_handler(std::move(handler));
}

void McpClient::enable_tool_use_for_sampling(bool enable) {
    if (enable) {
        // Set up a synchronous tool caller that blocks on response
        sampling_client_.set_tool_caller([this](std::string_view method, const nlohmann::json& params) -> nlohmann::json {
            // Generate request ID
            core::RequestId id = request_tracker_.next_id();

            // Create promise/future for blocking wait
            auto promise = std::make_shared<std::promise<nlohmann::json>>();
            auto future = promise->get_future();

            // Register pending request
            request_tracker_.register_pending(
                id,
                [promise](const nlohmann::json& result) {
                    promise->set_value(result);
                },
                [promise](const core::JsonRpcError& error) {
                    promise->set_value(nlohmann::json{
                        {"error", true},
                        {"code", error.code},
                        {"message", error.message}
                    });
                }
            );

            // Build and send request
            core::JsonRpcRequest req;
            req.id = id;
            req.method = std::string(method);
            req.params = params;

            std::string message = req.to_string();
            transport_->send(message);

            // Wait for response with a timeout
            // Note: This blocks the event loop - in production would need async approach
            // For MVP, this is acceptable limitation
            std::future_status status = future.wait_for(std::chrono::seconds(30));
            if (status != std::future_status::ready) {
                // Timeout - set error value
                promise->set_value(nlohmann::json{
                    {"error", true},
                    {"code", -32603},
                    {"message", "Tool call timeout"}
                });
            }

            return future.get();
        });
    } else {
        // Clear the tool caller (disables tool loop)
        sampling_client_.clear_tool_caller();
    }
}

void McpClient::set_elicitation_handler(client::ElicitationHandler handler) {
    elicitation_client_.set_elicitation_handler(std::move(handler));
}

// ============================================================================
// MCP initialization
// ============================================================================

namespace {

// Helper to parse InitializeResult from JSON
std::optional<protocol::InitializeResult> parse_initialize_result(const core::JsonValue& result_json) {
    try {
        protocol::InitializeResult result;

        if (result_json.contains("protocolVersion") && result_json["protocolVersion"].is_string()) {
            result.protocolVersion = result_json["protocolVersion"].get<std::string>();
        } else {
            return std::nullopt;
        }

        if (result_json.contains("serverInfo") && result_json["serverInfo"].is_object()) {
            const auto& server_info = result_json["serverInfo"];
            if (server_info.contains("name") && server_info["name"].is_string()) {
                result.serverInfo.name = server_info["name"].get<std::string>();
            }
            if (server_info.contains("version") && server_info["version"].is_string()) {
                result.serverInfo.version = server_info["version"].get<std::string>();
            }
        }

        if (result_json.contains("instructions") && result_json["instructions"].is_string()) {
            result.instructions = result_json["instructions"].get<std::string>();
        }

        if (result_json.contains("capabilities") && result_json["capabilities"].is_object()) {
            const auto& caps = result_json["capabilities"];

            // Experimental capabilities
            if (caps.contains("experimental")) {
                result.capabilities.experimental = caps["experimental"];
            }

            // Logging capability
            if (caps.contains("logging")) {
                protocol::LoggingCapability logging{};
                result.capabilities.logging = logging;
            }

            // Prompts capability
            if (caps.contains("prompts")) {
                protocol::PromptCapability prompts{};
                const auto& prompts_json = caps["prompts"];
                if (prompts_json.contains("listChanged") && prompts_json["listChanged"].is_boolean()) {
                    prompts.listChanged = prompts_json["listChanged"].get<bool>();
                }
                result.capabilities.prompts = prompts;
            }

            // Resources capability
            if (caps.contains("resources")) {
                protocol::ResourceCapability resources{};
                const auto& resources_json = caps["resources"];
                if (resources_json.contains("subscribe") && resources_json["subscribe"].is_boolean()) {
                    resources.subscribe = resources_json["subscribe"].get<bool>();
                }
                if (resources_json.contains("listChanged") && resources_json["listChanged"].is_boolean()) {
                    resources.listChanged = resources_json["listChanged"].get<bool>();
                }
                result.capabilities.resources = resources;
            }

            // Tools capability
            if (caps.contains("tools")) {
                protocol::ToolCapability tools{};
                const auto& tools_json = caps["tools"];
                if (tools_json.contains("listChanged") && tools_json["listChanged"].is_boolean()) {
                    tools.listChanged = tools_json["listChanged"].get<bool>();
                }
                result.capabilities.tools = tools;
            }
        }

        return result;

    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

} // namespace

void McpClient::initialize(
    const protocol::InitializeRequestParams& params,
    std::function<void(const protocol::InitializeResult&)> on_complete,
    async::ErrorCallback on_error
) {
    // Wrap the success callback to parse InitializeResult and send initialized notification
    async::ResponseCallback wrapped_on_success =
        [this, on_complete = std::move(on_complete)](const core::JsonValue& result) mutable {
            // Parse InitializeResult
            auto init_result = parse_initialize_result(result);
            if (init_result && on_complete) {
                on_complete(*init_result);
                // Automatically send initialized notification
                send_initialized_notification();
            } else if (on_complete) {
                // Failed to parse - treat as error
                core::JsonRpcError parse_error{
                    core::INTERNAL_ERROR,
                    "Failed to parse initialize result"
                };
                // Can't invoke on_complete with error - would need separate error path
                // For now, we invoke the error callback if we had captured it
            }
        };

    // Send initialize request
    send_request("initialize", protocol::make_initialize_request(params).params,
        std::move(wrapped_on_success),
        std::move(on_error)
    );
}

void McpClient::send_initialized_notification() {
    send_notification("notifications/initialized", nullptr);
}

// ============================================================================
// Transport callbacks
// ============================================================================

void McpClient::on_message(std::string_view message) {
    try {
        // Parse JSON message
        nlohmann::json j = nlohmann::json::parse(message);

        // Route based on message type
        if (is_response(j)) {
            // Try to parse as JsonRpcResponse
            auto response = core::JsonRpcResponse::from_json(j);
            if (response) {
                handle_response(*response);
                return;
            }
        } else if (is_request(j)) {
            // Try to parse as JsonRpcRequest (incoming server request)
            core::JsonRpcRequest request;
            request.jsonrpc = "2.0";

            // Parse ID
            if (j.contains("id")) {
                if (j["id"].is_number_integer()) {
                    request.id = j["id"].get<int64_t>();
                } else if (j["id"].is_string()) {
                    request.id = j["id"].get<std::string>();
                }
            }

            // Parse method and params
            if (j.contains("method") && j["method"].is_string()) {
                request.method = j["method"].get<std::string>();
            }
            if (j.contains("params")) {
                request.params = j["params"];
            }

            handle_server_request(request);
            return;
        } else if (is_notification(j)) {
            // Try to parse as JsonRpcNotification
            core::JsonRpcNotification notification;
            notification.jsonrpc = "2.0";

            if (j.contains("method") && j["method"].is_string()) {
                notification.method = j["method"].get<std::string>();
            }
            if (j.contains("params")) {
                notification.params = j["params"];
            }

            handle_notification(notification);
            return;
        }

        // Unknown message type - could log or ignore
        // For now, silently ignore

    } catch (const nlohmann::json::parse_error&) {
        // JSON parse error - send error response back
        // (Can't send error without knowing message type)
    } catch (const std::exception&) {
        // Other exception - ignore
    }
}

void McpClient::on_transport_error(std::string_view error) {
    // Transport errors are reported via callback
    // For now, we don't have a user-facing error callback for transport errors
    // This could be added in the future
    (void)error; // Suppress unused warning
}

// ============================================================================
// Message handling
// ============================================================================

void McpClient::handle_response(const core::JsonRpcResponse& response) {
    // Cancel timeout for this request
    timeout_manager_.cancel(response.id);

    // Complete the pending request (removes from tracker)
    auto pending = request_tracker_.complete(response.id);

    // Unregister from cancellation tracking
    cancellation_manager_.unregister_request(response.id);

    if (!pending) {
        // No pending request found - response without matching request
        // This can happen for responses that already timed out
        return;
    }

    // Invoke appropriate callback
    if (response.is_error() && pending->on_error) {
        pending->on_error(*response.error);
    } else if (response.is_success() && pending->on_success) {
        pending->on_success(*response.result);
    }
}

void McpClient::handle_server_request(const core::JsonRpcRequest& request) {
    // Look up handler for this method
    auto it = request_handlers_.find(request.method);
    if (it != request_handlers_.end()) {
        // Handler found - invoke it
        try {
            JsonValue result = it->second(request.method, request.params);
            send_response(request.id, result);
        } catch (const std::exception&) {
            // Handler threw exception - send internal error
            send_error_response(request.id, core::JsonRpcError::internal_error());
        }
    } else {
        // No handler found - send method not found error
        send_error_response(request.id, core::JsonRpcError::method_not_found(request.method));
    }
}

void McpClient::handle_notification(const core::JsonRpcNotification& notification) {
    // Look up handler for this notification
    auto it = notification_handlers_.find(notification.method);
    if (it != notification_handlers_.end()) {
        // Handler found - invoke it
        try {
            it->second(notification.method, notification.params);
        } catch (const std::exception&) {
            // Notification handler threw exception - ignore (no response expected)
        }
    }
    // No handler found - silently ignore notification
}

// ============================================================================
// Sending responses to server
// ============================================================================

void McpClient::send_response(core::RequestId id, const JsonValue& result) {
    core::JsonRpcResponse response;
    response.id = id;
    response.result = result;

    std::string message = response.to_string();
    transport_->send(message);
}

void McpClient::send_error_response(core::RequestId id, const core::JsonRpcError& error) {
    core::JsonRpcResponse response;
    response.id = id;
    response.error = error;

    std::string message = response.to_string();
    transport_->send(message);
}

} // namespace mcpp
