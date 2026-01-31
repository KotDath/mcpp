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

/**
 * @file http_server_integration.cpp
 * @brief Example demonstrating user HTTP server integration with HttpTransport
 *
 * This example shows how to integrate HttpTransport with a user-provided HTTP server.
 * The library does NOT own the HTTP server lifecycle - users bring their own server
 * (nginx, Apache, drogon, oat++, cpp-httplib, etc.) and integrate via adapter pattern.
 *
 * Key concepts demonstrated:
 * 1. HttpResponseAdapter - wraps user's HTTP response for POST handlers
 * 2. HttpSseWriterAdapter - wraps user's HTTP response for SSE streaming
 * 3. Session management via Mcp-Session-Id header
 * 4. Non-blocking I/O patterns
 *
 * NOTE: This is pseudo-code for demonstration. To run with a real HTTP server,
 * adapt the adapter classes to your server's API (e.g., cpp-httplib, drogon).
 */

#include "mcpp/transport/http_transport.h"

#include <iostream>
#include <string>
#include <thread>

namespace mcpp {
namespace examples {

// ============================================================================
// User HTTP Server Pseudo-API (adapt to your server)
// ============================================================================

/**
 * @brief Pseudo HTTP request structure
 *
 * User adapts this to their HTTP server's request type.
 */
struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::string get_header(const std::string& name) const;
};

/**
 * @brief Pseudo HTTP response structure
 *
 * User adapts this to their HTTP server's response type.
 */
struct HttpResponse {
    int status = 200;
    std::string content_type = "text/plain";
    std::string body;

    void set_header(const std::string& name, const std::string& value) {
        if (name == "Content-Type") {
            content_type = value;
        }
        // Other headers would be stored in a map in a real implementation
    }

    void write(const std::string& data) {
        body = data;
    }

    void set_status(int code) {
        status = code;
    }

    void flush() {
        // Send response to client
        std::cout << "Sending response: status=" << status
                  << ", body=" << body.substr(0, 100) << "..." << std::endl;
    }
};

/**
 * @brief Pseudo SSE writer for HTTP responses
 *
 * Extends HttpResponse with SSE-specific functionality.
 */
struct HttpSseWriter : public HttpResponse {
    void write_sse(const std::string& data) {
        body += data;
        flush();  // Immediate delivery for SSE
    }
};

/**
 * @brief Pseudo HTTP server
 *
 * User replaces this with their actual HTTP server (e.g., cpp-httplib::Server).
 */
class HttpServer {
public:
    using PostHandler = std::function<void(const HttpRequest&, HttpResponse&)>;
    using GetHandler = std::function<void(const HttpRequest&, HttpResponse&)>;

    void Post(const std::string& path, PostHandler handler) {
        post_handlers_[path] = std::move(handler);
    }

    void Get(const std::string& path, GetHandler handler) {
        get_handlers_[path] = std::move(handler);
    }

    void listen(const std::string& host, int port) {
        std::cout << "Server listening on " << host << ":" << port << std::endl;
        // In real implementation, this would block and handle requests
        running_ = true;
    }

    void stop() {
        running_ = false;
    }

    bool is_running() const { return running_; }

private:
    std::unordered_map<std::string, PostHandler> post_handlers_;
    std::unordered_map<std::string, GetHandler> get_handlers_;
    bool running_ = false;
};

} // namespace examples
} // namespace mcpp

// ============================================================================
// HttpTransport Integration Example
// ============================================================================

using namespace mcpp;
using namespace mcpp::examples;
using namespace mcpp::transport;

int main() {
    std::cout << "=== MCP HTTP/SSE Transport Integration Example ===" << std::endl;

    // Create HTTP transport
    HttpTransport http_transport;

    // Connect to establish initial session
    if (!http_transport.connect()) {
        std::cerr << "Failed to connect HTTP transport" << std::endl;
        return 1;
    }
    std::cout << "Created session: " << http_transport.get_session_id() << std::endl;

    // Set message callback for incoming POST requests
    http_transport.set_message_callback([](std::string_view message) {
        std::cout << "Received POST: " << message << std::endl;
        // In real usage, this would be passed to McpServer for JSON-RPC handling
    });

    // Set error callback for transport errors
    http_transport.set_error_callback([](std::string_view error) {
        std::cerr << "Transport error: " << error << std::endl;
    });

    // Create user's HTTP server
    HttpServer server;

    // -------------------------------------------------------------------------
    // POST /mcp endpoint - Client sends JSON-RPC requests
    // -------------------------------------------------------------------------
    server.Post("/mcp", [&](const HttpRequest& req, HttpResponse& res) {
        std::string session_id = req.get_header("Mcp-Session-Id");

        // Create adapter wrapping user's HTTP response
        HttpResponseAdapter<HttpResponse> adapter(res);

        // Handle POST via HttpTransport
        http_transport.handle_post_request(req.body, session_id, adapter);

        std::cout << "POST /mcp - Session: " << (session_id.empty() ? "new" : session_id)
                  << ", Status: " << res.status << std::endl;
    });

    // -------------------------------------------------------------------------
    // GET /mcp endpoint - Client receives SSE stream
    // -------------------------------------------------------------------------
    server.Get("/mcp", [&](const HttpRequest& req, HttpResponse& res) {
        std::string session_id = req.get_header("Mcp-Session-Id");
        std::string last_event_id = req.get_header("Last-Event-ID");

        // Create SSE writer adapter wrapping user's HTTP response
        HttpSseWriterAdapter<HttpSseWriter> writer(static_cast<HttpSseWriter&>(res));

        // Handle GET via HttpTransport (sends buffered messages via SSE)
        http_transport.handle_get_request(session_id, last_event_id, writer);

        std::cout << "GET /mcp - Session: " << (session_id.empty() ? "new" : session_id);
        if (!last_event_id.empty()) {
            std::cout << ", Last-Event-ID: " << last_event_id;
        }
        std::cout << std::endl;
    });

    // -------------------------------------------------------------------------
    // Non-blocking I/O demonstration
    // -------------------------------------------------------------------------
    std::cout << "\n--- Non-blocking I/O demonstration ---" << std::endl;

    // Send a notification (buffers immediately, returns - doesn't block)
    nlohmann::json notification = {
        {"jsonrpc", "2.0"},
        {"method", "notifications/message"},
        {"params", {
            {"message", "Hello from HTTP transport!"}
        }}
    };
    http_transport.send_notification(notification);
    std::cout << "Notification buffered (non-blocking)" << std::endl;

    // Send another message (also non-blocking)
    http_transport.send(R"({"jsonrpc":"2.0","method":"test","params":{},"id":1})");
    std::cout << "Message buffered (non-blocking)" << std::endl;

    // In real usage, messages are delivered when client makes GET request
    std::cout << "Messages will be delivered on next GET /mcp request" << std::endl;

    // -------------------------------------------------------------------------
    // Session management demonstration
    // -------------------------------------------------------------------------
    std::cout << "\n--- Session management ---" << std::endl;

    // Validate current session
    std::string session = http_transport.get_session_id();
    bool valid = http_transport.validate_session(session);
    std::cout << "Session " << session << " valid: " << (valid ? "yes" : "no") << std::endl;

    // Validate non-existent session
    bool fake_valid = http_transport.validate_session("fake-session-id");
    std::cout << "Session 'fake-session-id' valid: " << (fake_valid ? "yes" : "no") << std::endl;

    // Create additional session
    std::string new_session = http_transport.create_session();
    std::cout << "Created new session: " << new_session << std::endl;

    // Terminate session
    bool terminated = http_transport.terminate_session(new_session);
    std::cout << "Session terminated: " << (terminated ? "yes" : "no") << std::endl;

    // -------------------------------------------------------------------------
    // Server would start here in real usage
    // -------------------------------------------------------------------------
    std::cout << "\n--- Server setup complete ---" << std::endl;
    std::cout << "In real usage, server.listen(\"0.0.0.0\", 8080) would block here" << std::endl;
    std::cout << "Adapt this example to your HTTP server (cpp-httplib, drogon, etc.)" << std::endl;

    // Cleanup
    http_transport.disconnect();
    std::cout << "Disconnected - session terminated" << std::endl;

    return 0;
}

// ============================================================================
// Integration Notes for Different HTTP Servers
// ============================================================================

/**
 * @page HTTP Server Integration Guide
 *
 * ADAPTING TO CPP-HTTPLIB:
 * @code
 *   #include <httplib.h>
 *
 *   httplib::Server server;
 *   HttpTransport http_transport;
 *   http_transport.connect();
 *
 *   server.Post("/mcp", [&](const httplib::Request& req, httplib::Response& res) {
 *       std::string session_id = req.get_header_value("Mcp-Session-Id");
 *       HttpResponseAdapter<httplib::Response> adapter(res);
 *       http_transport.handle_post_request(req.body, session_id, adapter);
 *   });
 *
 *   server.Get("/mcp", [&](const httplib::Request& req, httplib::Response& res) {
 *       std::string session_id = req.get_header_value("Mcp-Session-Id");
 *       std::string last_event_id = req.get_header_value("Last-Event-ID");
 *       HttpSseWriterAdapter<httplib::Response> writer(res);
 *       http_transport.handle_get_request(session_id, last_event_id, writer);
 *   });
 *
 *   server.listen("0.0.0.0", 8080);
 * @endcode
 *
 * ADAPTING TO DROGON:
 * @code
 *   #include <drogon/drogon.h>
 *
 *   // In your controller
 *   void MCPController::handlePost(const HttpRequestPtr& req,
 *                                   std::function<void(const HttpResponsePtr&)>&& callback) {
 *       auto response = drogon::HttpResponse::newHttpJsonResponse(Json::objectValue);
 *       HttpResponseAdapter<drogon::HttpResponsePtr> adapter(response);
 *       http_transport.handle_post_request(req->body(), req->getHeader("Mcp-Session-Id"), adapter);
 *       callback(response);
 *   }
 * @endcode
 *
 * ADAPTING TO OAT++:
 * @code
 *   #include <oatpp/web/server/HttpConnectionHandler.hpp>
 *
 *   ENDPOINT_INFO(MCP) {
 *       info->summary = "MCP endpoint";
 *   }
 *
 *   ENDPOINT("POST", "/mcp", MCP, POST) {
 *       auto response = createResponse(Status::CODE_200, {});
 *       HttpResponseAdapter<decltype(response)> adapter(response);
 *       http_transport.handle_post_request(
 *           request->readBodyToString(),
 *           request->getHeader("Mcp-Session-Id"),
 *           adapter
 *       );
 *       return response;
 *   }
 * @endcode
 */
