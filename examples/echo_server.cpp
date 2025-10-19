#include "mcpp/mcpp.h"
#include <iostream>
#include <memory>

using namespace mcpp;

// Helper function to convert Request ID to Response ID
core::JsonRpcResponse::IdType request_to_response_id(const core::JsonRpcRequest::IdType& request_id) {
    if (std::holds_alternative<std::nullptr_t>(request_id)) {
        // For notifications with null ID, use default string ID
        return std::string("0");
    } else if (std::holds_alternative<std::string>(request_id)) {
        return std::get<std::string>(request_id);
    } else if (std::holds_alternative<int64_t>(request_id)) {
        return std::get<int64_t>(request_id);
    }
    return std::string("0"); // Fallback
}

/**
 * @brief Simple MCP Echo Server
 *
 * This server demonstrates the basic MCP functionality:
 * - Handles initialize requests
 * - Responds to ping requests
 * - Echoes back any other requests
 * - Sends notifications
 */
class EchoServer {
public:
    EchoServer(std::unique_ptr<transport::StdioTransport> transport)
        : transport_(std::move(transport)) {
    }

    void run() {
        std::cout << "MCP Echo Server starting..." << std::endl;

        // Initialize the library
        mcpp::initialize();

        // Start the transport
        transport_->start();

        std::cout << "Echo Server ready. Waiting for messages..." << std::endl;

        // Main server loop
        while (transport_->is_open()) {
            try {
                // Receive a message
                auto future = transport_->receive();
                auto message = future.get();

                if (!message) {
                    continue;
                }

                // Process the message
                handle_message(std::move(message));
            }
            catch (const std::exception& e) {
                MCP_LOG_ERROR("Error in main loop: {}", e.what());
                break;
            }
        }

        std::cout << "Echo Server shutting down..." << std::endl;
        transport_->stop();
    }

private:
    std::unique_ptr<transport::StdioTransport> transport_;

    void handle_message(std::unique_ptr<core::JsonRpcMessage> message) {
        switch (message->get_type()) {
            case core::JsonRpcMessageType::Request:
                handle_request(static_cast<core::JsonRpcRequest*>(message.get()));
                break;

            case core::JsonRpcMessageType::Notification:
                handle_notification(static_cast<core::JsonRpcNotification*>(message.get()));
                break;

            default:
                MCP_LOG_WARN("Received unsupported message type");
                break;
        }
    }

    void handle_request(core::JsonRpcRequest* request) {
        const std::string& method = request->get_method();
        MCP_LOG_INFO("Received request: {}", method);

        try {
            if (method == "initialize") {
                handle_initialize(request);
            } else if (method == "ping") {
                handle_ping(request);
            } else {
                // Echo back the request
                handle_echo(request);
            }
        }
        catch (const std::exception& e) {
            MCP_LOG_ERROR("Error handling request {}: {}", method, e.what());
            send_error_response(request_to_response_id(request->get_id()), core::JsonRpcErrorCode::InternalError, e.what());
        }
    }

    void handle_initialize(core::JsonRpcRequest* request) {
        // Parse initialize parameters
        const auto& params = request->get_params();
        std::string protocol_version = params["protocolVersion"];
        MCP_LOG_INFO("Initialize request with protocol version: {}", protocol_version);

        // Create initialize response
        model::InitializeResponse::Result result;
        result.protocol_version = core::ProtocolVersion::LATEST;
        result.capabilities = nlohmann::json{
            {"tools", nlohmann::json{{"listChanged", true}}},
            {"logging", nlohmann::json{}},
            {"experimental", nlohmann::json{}}
        };
        result.server_info = R"({
            "name": "mcpp-echo-server",
            "version": "1.0.0"
        })";

        auto response = model::InitializeResponse(result, request_to_response_id(request->get_id()));
        transport_->send(*response.to_json_rpc_response());

        // Send initialized notification
        auto notification = model::InitializedNotification();
        transport_->send(*notification.to_json_rpc_notification());

        MCP_LOG_INFO("Initialize completed");
    }

    void handle_ping(core::JsonRpcRequest* request) {
        MCP_LOG_INFO("Ping request received");

        auto response = model::PingResponse(request_to_response_id(request->get_id()));
        transport_->send(*response.to_json_rpc_response());

        MCP_LOG_INFO("Ping response sent");
    }

    void handle_echo(core::JsonRpcRequest* request) {
        const std::string& method = request->get_method();
        const auto& params = request->get_params();

        MCP_LOG_INFO("Echo request: {} with params: {}", method, params.dump());

        // Echo back the parameters as result
        auto response = std::make_unique<core::JsonRpcResponse>(request_to_response_id(request->get_id()), params);
        transport_->send(*response);
    }

    void handle_notification(core::JsonRpcNotification* notification) {
        const std::string& method = notification->get_method();
        MCP_LOG_INFO("Received notification: {}", method);

        // Handle different notifications
        if (method == "notifications/cancelled") {
            const auto& params = notification->get_params();
            MCP_LOG_INFO("Request cancelled: {}", params.dump());
        } else if (method == "notifications/initialized") {
            MCP_LOG_INFO("Client initialized");
        } else {
            MCP_LOG_INFO("Unknown notification: {}", method);
        }
    }

    void send_error_response(const core::JsonRpcResponse::IdType& id,
                           core::JsonRpcErrorCode code,
                           const std::string& message) {
        auto error_response = utils::create_error_response(id, code, message);
        transport_->send(*error_response);
    }
};

int main() {
    try {
        // Create transport
        auto transport = std::make_unique<transport::StdioTransport>();

        // Create and run server
        EchoServer server(std::move(transport));
        server.run();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}