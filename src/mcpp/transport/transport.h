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

#ifndef MCPP_TRANSPORT_TRANSPORT_H
#define MCPP_TRANSPORT_TRANSPORT_H

#include <functional>
#include <string>
#include <string_view>

namespace mcpp {
namespace transport {

/**
 * @brief Abstract base class for transport implementations
 *
 * Transport provides a pluggable abstraction for different communication
 * mechanisms (stdio, SSE, WebSocket). All transports handle their own message
 * framing (e.g., newlines for stdio, SSE protocol for HTTP).
 *
 * The transport layer is responsible for:
 * - Managing the connection lifecycle (explicit, not RAII-based)
 * - Sending complete JSON-RPC messages (already serialized)
 * - Receiving messages and invoking callbacks
 * - Reporting errors asynchronously
 *
 * @note Transport implementations handle their own message framing
 *       (e.g., newlines for stdio, SSE protocol for HTTP).
 */
class Transport {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes
     */
    virtual ~Transport() = default;

    /**
     * @brief Establish the transport connection
     *
     * Initiates the connection handshake or setup required by the transport.
     * Must be called before sending messages.
     *
     * @return true if connection succeeded, false otherwise
     */
    virtual bool connect() = 0;

    /**
     * @brief Close the transport connection
     *
     * Gracefully shuts down the transport connection. After this call,
     * is_connected() should return false.
     */
    virtual void disconnect() = 0;

    /**
     * @brief Check if the transport is currently connected
     *
     * @return true if connected, false otherwise
     */
    virtual bool is_connected() const = 0;

    /**
     * @brief Send a complete JSON-RPC message
     *
     * The message is already serialized JSON. The transport is responsible
     * for adding any framing required (e.g., newline delimiter for stdio).
     *
     * @param message The complete JSON-RPC message to send
     * @return true if the message was sent successfully, false otherwise
     */
    virtual bool send(std::string_view message) = 0;

    /**
     * @brief Callback type for received messages
     *
     * Invoked when a complete message is received from the transport.
     * The callback receives a string_view pointing to the message data.
     *
     * @note The library stores a copy of the std::function. Users should
     *       capture by value to avoid dangling reference issues.
     */
    using MessageCallback = std::function<void(std::string_view)>;

    /**
     * @brief Set the callback for received messages
     *
     * Register a callback to be invoked when a complete message is received.
     * Only one callback can be active at a time; calling this again replaces
     * the previous callback.
     *
     * @param cb The callback function to invoke for each received message
     */
    virtual void set_message_callback(MessageCallback cb) = 0;

    /**
     * @brief Callback type for transport errors
     *
     * Invoked when an error occurs asynchronously (e.g., connection lost,
     * read failure). The callback receives a string_view describing the error.
     *
     * @note The library stores a copy of the std::function. Users should
     *       capture by value to avoid dangling reference issues.
     */
    using ErrorCallback = std::function<void(std::string_view)>;

    /**
     * @brief Set the callback for asynchronous error reporting
     *
     * Register a callback to be invoked when transport errors occur.
     * Only one callback can be active at a time; calling this again replaces
     * the previous callback.
     *
     * @param cb The callback function to invoke on transport errors
     */
    virtual void set_error_callback(ErrorCallback cb) = 0;

protected:
    /**
     * @brief Protected default constructor - Transport is an abstract base class
     */
    Transport() = default;

    // Non-copyable
    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;

    // Non-movable (derived classes can opt-in if needed)
    Transport(Transport&&) = delete;
    Transport& operator=(Transport&&) = delete;
};

} // namespace transport
} // namespace mcpp

#endif // MCPP_TRANSPORT_TRANSPORT_H
