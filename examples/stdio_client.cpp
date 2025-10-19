#include "mcpp/mcpp.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace mcpp;

/**
 * @brief Simple MCP Client for testing
 *
 * This client demonstrates how to:
 * - Initialize connection with server
 * - Send ping requests
 * - Send custom requests
 * - Handle responses and notifications
 */
class StdioClient {
public:
    StdioClient(std::unique_ptr<transport::StdioTransport> transport)
        : transport_(std::move(transport)), next_id_(1) {
    }

    bool connect() {
        std::cout << "Connecting to MCP server..." << std::endl;

        // Initialize the library
        mcpp::initialize();

        // Start the transport
        transport_->start();

        // Send initialize request
        if (!initialize()) {
            std::cerr << "Failed to initialize" << std::endl;
            return false;
        }

        std::cout << "Connected successfully!" << std::endl;
        return true;
    }

    void run_interactive() {
        std::cout << "\nMCP Client Interactive Mode" << std::endl;
        std::cout << "Commands: ping, echo <json>, quit" << std::endl;
        std::cout << "> ";

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) {
                std::cout << "> ";
                continue;
            }

            if (line == "quit") {
                break;
            } else if (line == "ping") {
                send_ping();
            } else if (line.substr(0, 4) == "echo") {
                if (line.length() > 5) {
                    std::string json_str = line.substr(5);
                    try {
                        auto json = nlohmann::json::parse(json_str);
                        send_custom_request("echo", json);
                    } catch (const std::exception& e) {
                        std::cout << "Invalid JSON: " << e.what() << std::endl;
                    }
                } else {
                    send_custom_request("echo", nlohmann::json{{"message", "hello"}});
                }
            } else {
                std::cout << "Unknown command. Available: ping, echo <json>, quit" << std::endl;
            }

            std::cout << "> ";
        }

        transport_->close();
    }

    void send_ping() {
        auto request = model::PingRequest();
        auto json_request = request.to_json_rpc_request();
        transport_->send(*json_request);

        std::cout << "Ping sent, waiting for response..." << std::endl;

        // Wait for response
        auto future = transport_->receive();
        try {
            auto response = future.get();
            if (response && response->get_type() == core::JsonRpcMessageType::Response) {
                auto json_response = static_cast<core::JsonRpcResponse*>(response.get());
                if (!json_response->is_error()) {
                    std::cout << "Pong!" << std::endl;
                } else {
                    std::cout << "Ping error: " << json_response->get_error()->message << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "Error receiving ping response: " << e.what() << std::endl;
        }
    }

    void send_custom_request(const std::string& method, const nlohmann::json& params) {
        auto json_request = std::make_unique<core::JsonRpcRequest>(next_id_++, method, params);
        transport_->send(*json_request);

        std::cout << "Request sent: " << method << " with params: " << params.dump() << std::endl;
        std::cout << "Waiting for response..." << std::endl;

        // Wait for response
        auto future = transport_->receive();
        try {
            auto response = future.get();
            if (response && response->get_type() == core::JsonRpcMessageType::Response) {
                auto json_response = static_cast<core::JsonRpcResponse*>(response.get());
                if (!json_response->is_error()) {
                    std::cout << "Response: " << json_response->get_result().dump() << std::endl;
                } else {
                    std::cout << "Error response: " << json_response->get_error()->message << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "Error receiving response: " << e.what() << std::endl;
        }
    }

private:
    std::unique_ptr<transport::StdioTransport> transport_;
    int64_t next_id_;

    bool initialize() {
        // Create initialize request
        model::InitializeRequest::Params params;
        params.protocol_version = core::ProtocolVersion::LATEST;
        params.capabilities = nlohmann::json{
            {"experimental", nlohmann::json{}},
            {"tools", nlohmann::json{}}
        };
        params.client_info = R"({
            "name": "mcpp-stdio-client",
            "version": "1.0.0"
        })";

        auto request = model::InitializeRequest(params);
        auto json_request = request.to_json_rpc_request();
        transport_->send(*json_request);

        // Wait for initialize response
        auto future = transport_->receive();
        try {
            auto response = future.get();
            if (response && response->get_type() == core::JsonRpcMessageType::Response) {
                auto json_response = static_cast<core::JsonRpcResponse*>(response.get());
                if (!json_response->is_error()) {
                    std::cout << "Initialized successfully!" << std::endl;
                    return true;
                } else {
                    std::cerr << "Initialize error: " << json_response->get_error()->message << std::endl;
                    return false;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error during initialize: " << e.what() << std::endl;
            return false;
        }

        return false;
    }
};

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        // Create transport
        auto transport = std::make_unique<transport::StdioTransport>();

        // Create and run client
        StdioClient client(std::move(transport));

        if (client.connect()) {
            client.run_interactive();
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}