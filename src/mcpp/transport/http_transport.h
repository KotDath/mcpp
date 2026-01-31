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

#ifndef MCPP_TRANSPORT_HTTP_TRANSPORT_H
#define MCPP_TRANSPORT_HTTP_TRANSPORT_H

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "mcpp/transport/transport.h"
#include "mcpp/util/sse_formatter.h"
#include <nlohmann/json.hpp>

namespace mcpp {
namespace transport {

/**
 * @brief HTTP/SSE transport for Streamable HTTP transport per MCP spec
 *
 * HttpTransport implements the Transport interface for MCP Streamable HTTP
 * (POST for client->server, GET/SSE for server->client) as defined in the
 * MCP 2025-11-25 transport specification.
 *
 * Key features:
 * - Single endpoint design: POST for requests, GET for SSE stream
 * - Session management via Mcp-Session-Id header
 * - Resumable connections via Last-Event-ID header
 * - User-provided HTTP server integration (library doesn't own server)
 * - Non-blocking I/O patterns (send() buffers, handle_post returns immediately)
 *
 * User HTTP server integration pattern:
 * - User creates their own HTTP server (nginx, Apache, drogon, etc.)
 * - User defines HttpResponse and HttpSseWriter adapter types
 * - User calls handle_post_request() and handle_get_request() from their handlers
 *
 * Example integration (pseudo-code):
 * @code
 *   // User's HTTP server setup
 *   HttpTransport http_transport;
 *   http_transport.connect();
 *
 *   // POST /mcp endpoint - client sends JSON-RPC
 *   server.Post("/mcp", [&](const Request& req, Response& res) {
 *       std::string body = req.body;
 *       HttpResponse response(res);  // User adapter wrapping server response
 *       http_transport.handle_post_request(body, response);
 *   });
 *
 *   // GET /mcp endpoint - client receives SSE stream
 *   server.Get("/mcp", [&](const Request& req, Response& res) {
 *       HttpSseWriter writer(res);  // User adapter wrapping server response
 *       http_transport.handle_get_request(writer);
 *   });
 * @endcode
 *
 * @note HttpResponse and HttpSseWriter are user-provided adapter types.
 *       Users must wrap their HTTP server's response object to provide:
 *       - HttpResponse: set_header(name, value), write(data), get_status()
 *       - HttpSseWriter: set_header(name, value), write_sse(data), get_status()
 *
 * @see Transport (base interface)
 * @see SseFormatter (SSE event formatting utility)
 */
class HttpTransport : public Transport {
public:
    /**
     * @brief User-provided HTTP response adapter interface
     *
     * Users must adapt their HTTP server's response object to provide this interface.
     * The adapter wraps the server's native response object and provides methods
     * for setting headers and writing response data.
     *
     * Example adapter implementation:
     * @code
     *   class HttpResponse {
     *   public:
     *       HttpResponse(ServerResponse& res) : res_(res) {}
     *
     *       void set_header(const std::string& name, const std::string& value) {
     *           res_.set_header(name, value);
     *       }
     *
     *       void write(const std::string& data) {
     *           res_.body = data;
     *       }
     *
     *       void set_status(int code) {
     *           res_.status = code;
     *       }
     *
     *   private:
     *       ServerResponse& res_;
     *   };
     * @endcode
     */
    template<typename T>
    class HttpResponseAdapter {
    public:
        explicit HttpResponseAdapter(T& response) : response_(response) {}

        /**
         * @brief Set an HTTP response header
         * @param name Header name
         * @param value Header value
         */
        void set_header(const std::string& name, const std::string& value) {
            response_.set_header(name, value);
        }

        /**
         * @brief Write response body data
         * @param data Response body content
         */
        void write(const std::string& data) {
            response_.write(data);
        }

        /**
         * @brief Set HTTP status code
         * @param code HTTP status code (200, 400, 500, etc.)
         */
        void set_status(int code) {
            response_.set_status(code);
        }

    private:
        T& response_;
    };

    /**
     * @brief User-provided SSE writer adapter interface
     *
     * Users must adapt their HTTP server's response object to provide SSE streaming.
     * The adapter wraps the server's native response object and provides methods
     * for setting SSE headers and writing SSE event data.
     *
     * Example adapter implementation:
     * @code
     *   class HttpSseWriter {
     *   public:
     *       HttpSseWriter(ServerResponse& res) : res_(res) {
     *           res_.set_header("Content-Type", "text/event-stream");
     *           res_.set_header("Cache-Control", "no-cache");
     *           res_.set_header("Connection", "keep-alive");
     *       }
     *
     *       void set_header(const std::string& name, const std::string& value) {
     *           res_.set_header(name, value);
     *       }
     *
     *       void write_sse(const std::string& data) {
     *           res_.write(data);
     *           res_.flush();  // Ensure immediate delivery
     *       }
     *
     *   private:
     *       ServerResponse& res_;
     *   };
     * @endcode
     */
    template<typename T>
    class HttpSseWriterAdapter {
    public:
        explicit HttpSseWriterAdapter(T& response) : response_(response) {}

        /**
         * @brief Set an HTTP response header
         * @param name Header name
         * @param value Header value
         */
        void set_header(const std::string& name, const std::string& value) {
            response_.set_header(name, value);
        }

        /**
         * @brief Write SSE event data to the stream
         * @param data SSE-formatted event data
         *
         * @note This should flush the stream to ensure immediate delivery.
         */
        void write_sse(const std::string& data) {
            response_.write_sse(data);
        }

    private:
        T& response_;
    };

    /**
     * @brief Session data for tracking active HTTP sessions
     */
    struct SessionData {
        std::string session_id;                                    ///< Unique session identifier (UUID v4)
        std::vector<std::string> pending_messages;                 ///< Messages pending SSE delivery
        std::chrono::steady_clock::time_point last_activity;       ///< Last activity timestamp for timeout
        uint64_t last_event_id;                                    ///< Last SSE event ID sent (for resumability)

        SessionData() : last_event_id(0) {}
    };

    /**
     * @brief Default constructor - creates HTTP transport in disconnected state
     *
     * @note Call connect() to establish a session before use.
     */
    HttpTransport() = default;

    /**
     * @brief Destructor - cleans up sessions
     */
    ~HttpTransport() override;

    // Non-copyable, non-movable
    HttpTransport(const HttpTransport&) = delete;
    HttpTransport& operator=(const HttpTransport&) = delete;
    HttpTransport(HttpTransport&&) = delete;
    HttpTransport& operator=(HttpTransport&&) = delete;

    /**
     * @brief Establish HTTP transport connection
     *
     * Creates a new session for this transport. HTTP is stateless from the
     * protocol perspective, but sessions enable message buffering and
     * resumability.
     *
     * @return true if session created successfully, false otherwise
     */
    bool connect() override;

    /**
     * @brief Close the HTTP transport connection
     *
     * Clears the current session and any pending messages.
     */
    void disconnect() override;

    /**
     * @brief Check if transport is connected (has active session)
     *
     * @return true if session exists, false otherwise
     */
    bool is_connected() const override;

    /**
     * @brief Send a message via SSE (non-blocking)
     *
     * Buffers the message for delivery on the next GET/SSE request.
     * This is non-blocking - it only stores the message in the buffer.
     *
     * @param message The JSON-RPC message to send (already serialized)
     * @return true if buffered successfully, false otherwise
     *
     * @note This does NOT send immediately. Messages are delivered when
     *       the client makes a GET request and handle_get_request is called.
     */
    bool send(std::string_view message) override;

    /**
     * @brief Set callback for incoming POST request messages
     *
     * The callback is invoked when a POST request is received with JSON-RPC content.
     *
     * @param cb Callback function to invoke for each POST request
     */
    void set_message_callback(MessageCallback cb) override;

    /**
     * @brief Set callback for transport errors
     *
     * The callback is invoked when transport errors occur (e.g., invalid session).
     *
     * @param cb Callback function to invoke on transport errors
     */
    void set_error_callback(ErrorCallback cb) override;

    /**
     * @brief Handle incoming POST request from client
     *
     * Called by the user's HTTP server when a POST request is received.
     * Extracts the Mcp-Session-Id header if present, parses the JSON-RPC
     * request body, and invokes the message callback.
     *
     * @tparam T User's HTTP server response type
     * @param body Request body containing JSON-RPC message
     * @param session_id Mcp-Session-Id header value (empty if not present)
     * @param response User's response adapter for sending the reply
     *
     * Example usage:
     * @code
     *   server.Post("/mcp", [&](const Request& req, Response& res) {
     *       std::string session_id = req.get_header("Mcp-Session-Id");
     *       HttpResponseAdapter<Response> response(res);
     *       http_transport.handle_post_request(req.body, session_id, response);
     *   });
     * @endcode
     */
    template<typename T>
    void handle_post_request(const std::string& body,
                             const std::string& session_id,
                             HttpResponseAdapter<T>& response) {
        // Validate or create session
        if (!session_id.empty()) {
            if (!validate_session(session_id)) {
                response.set_status(404);  // Session not found
                response.write("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32001,\"message\":\"Session not found\"},\"id\":null}\n");
                return;
            }
            current_session_id_ = session_id;
        } else if (current_session_id_.empty()) {
            // No session exists - create one
            create_session();
        }

        // Update session activity
        auto it = sessions_.find(current_session_id_);
        if (it != sessions_.end()) {
            it->second.last_activity = std::chrono::steady_clock::now();
        }

        // Invoke message callback if set
        if (message_callback_) {
            message_callback_(body);
        }

        // Send 200 OK response
        response.set_status(200);
        response.set_header("Content-Type", "application/json");
        response.write("{\"jsonrpc\":\"2.0\",\"result\":{},\"id\":null}\n");
    }

    /**
     * @brief Handle incoming GET request for SSE stream from client
     *
     * Called by the user's HTTP server when a GET request is received.
     * Sends all buffered messages via SSE and clears the buffer.
     *
     * @tparam T User's HTTP server response type
     * @param session_id Mcp-Session-Id header value
     * @param last_event_id Last-Event-ID header value for resumability
     * @param writer User's SSE writer adapter for streaming events
     *
     * Example usage:
     * @code
     *   server.Get("/mcp", [&](const Request& req, Response& res) {
     *       std::string session_id = req.get_header("Mcp-Session-Id");
     *       std::string last_event_id = req.get_header("Last-Event-ID");
     *       HttpSseWriterAdapter<Response> writer(res);
     *       http_transport.handle_get_request(session_id, last_event_id, writer);
     *   });
     * @endcode
     */
    template<typename T>
    void handle_get_request(const std::string& session_id,
                            const std::string& last_event_id,
                            HttpSseWriterAdapter<T>& writer) {
        // Validate session
        if (!session_id.empty() && !validate_session(session_id)) {
            writer.set_header("Content-Type", "application/json");
            writer.write_sse("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32001,\"message\":\"Session not found\"},\"id\":null}\n");
            return;
        }

        // Set session from request if valid
        if (!session_id.empty()) {
            current_session_id_ = session_id;
        } else if (current_session_id_.empty()) {
            // Create session if none exists
            create_session();
        }

        // Update session activity
        auto it = sessions_.find(current_session_id_);
        if (it != sessions_.end()) {
            it->second.last_activity = std::chrono::steady_clock::now();

            // Set SSE headers
            writer.set_header("Content-Type", util::SseFormatter::content_type());
            writer.set_header("Cache-Control", util::SseFormatter::cache_control());
            writer.set_header("Connection", util::SseFormatter::connection());

            // Send buffered messages via SSE
            for (const auto& msg : it->second.pending_messages) {
                // Parse message for SSE formatting
                try {
                    nlohmann::json j = nlohmann::json::parse(msg);
                    std::string event_id = std::to_string(it->second.last_event_id);
                    std::string sse_data = util::SseFormatter::format_event(j, event_id);
                    writer.write_sse(sse_data);
                    ++(it->second.last_event_id);
                } catch (...) {
                    // Invalid JSON - send as-is
                    writer.write_sse("data: " + msg + "\n\n");
                }
            }

            // Clear buffer after sending
            it->second.pending_messages.clear();
        }
    }

    /**
     * @brief Send a notification via SSE
     *
     * Formats a JSON notification and buffers it for SSE delivery.
     *
     * @param notification The notification JSON object
     */
    void send_notification(const nlohmann::json& notification);

    /**
     * @brief Create a new session with cryptographically secure ID
     *
     * @return The new session ID (UUID v4 format, 32 hex digits)
     */
    std::string create_session();

    /**
     * @brief Validate a session ID
     *
     * Checks if the session exists and hasn't timed out (30 minutes).
     *
     * @param session_id The session ID to validate
     * @return true if session is valid, false otherwise
     */
    bool validate_session(const std::string& session_id);

    /**
     * @brief Terminate a session
     *
     * Removes the session from active sessions.
     *
     * @param session_id The session ID to terminate
     * @return true if session was terminated, false if not found
     */
    bool terminate_session(const std::string& session_id);

    /**
     * @brief Get the current session ID
     *
     * @return Current session ID, or empty if not connected
     */
    const std::string& get_session_id() const { return current_session_id_; }

private:
    /**
     * @brief Clean up expired sessions (inactive > 30 minutes)
     */
    void cleanup_expired_sessions();

    static constexpr std::chrono::minutes SESSION_TIMEOUT{30};  ///< Session timeout duration

    std::string current_session_id_;                          ///< Current active session ID
    std::vector<std::string> message_buffer_;                 ///< Messages pending SSE delivery
    std::unordered_map<std::string, SessionData> sessions_;   ///< Active sessions
    MessageCallback message_callback_;                         ///< Callback for incoming POST requests
    ErrorCallback error_callback_;                             ///< Callback for error reporting
    uint64_t last_event_id_;                                   ///< Last SSE event ID sent
};

} // namespace transport
} // namespace mcpp

#endif // MCPP_TRANSPORT_HTTP_TRANSPORT_H
