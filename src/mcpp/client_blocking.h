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

#ifndef MCPP_CLIENT_BLOCKING_H
#define MCPP_CLIENT_BLOCKING_H

#include <chrono>
#include <exception>
#include <future>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "mcpp/client.h"
#include "mcpp/client/future_wrapper.h"
#include "mcpp/client/roots.h"
#include "mcpp/client/sampling.h"
#include "mcpp/protocol/initialize.h"

namespace mcpp {

/**
 * @brief Exception thrown by McpClientBlocking operations
 *
 * Provides a typed exception for blocking client operations,
 * including the error code from the underlying JSON-RPC error.
 */
class McpClientBlockingError : public std::runtime_error {
public:
    /**
     * @brief Construct a blocking error
     *
     * @param msg Error message
     * @param code Optional error code (0 if not applicable)
     */
    explicit McpClientBlockingError(const std::string& msg, int code = 0)
        : std::runtime_error(msg), code_(code) {}

    /**
     * @brief Get the error code
     *
     * @return The JSON-RPC error code, or 0 if not applicable
     */
    int code() const { return code_; }

private:
    int code_;
};

/**
 * @brief Blocking API wrapper for McpClient
 *
 * McpClientBlocking provides synchronous/blocking methods on top of the
 * callback-based McpClient. This enables straightforward synchronous code
 * for users who prefer simple call/wait over callback-based async.
 *
 * The callback API remains the foundation (required for streaming and
 * non-blocking I/O), but futures enable straightforward synchronous code.
 *
 * Usage:
 *   McpClient client(std::make_unique<StdioTransport>());
 *   client.connect();
 *
 *   McpClientBlocking blocking(client);
 *   auto result = blocking.initialize(params);  // Blocks until complete
 *
 * Thread safety: Not thread-safe. Use external synchronization if needed.
 */
class McpClientBlocking {
public:
    /**
     * @brief Construct a blocking wrapper around a client
     *
     * The client must remain valid for the lifetime of this wrapper.
     * The wrapper holds a reference (not ownership) to the client.
     *
     * @param client Reference to the async client to wrap
     * @param default_timeout Default timeout for blocking operations
     */
    explicit McpClientBlocking(
        McpClient& client,
        std::chrono::milliseconds default_timeout = std::chrono::milliseconds(30000)
    );

    /**
     * @brief Destructor
     */
    ~McpClientBlocking() = default;

    // Non-copyable, non-movable (holds reference to client)
    McpClientBlocking(const McpClientBlocking&) = delete;
    McpClientBlocking& operator=(const McpClientBlocking&) = delete;
    McpClientBlocking(McpClientBlocking&&) = delete;
    McpClientBlocking& operator=(McpClientBlocking&&) = delete;

    /**
     * @brief Blocking initialize call
     *
     * Sends an initialize request and blocks until the response is received.
     *
     * @param params Initialize request parameters
     * @return The server's initialize result
     * @throws McpClientBlockingError on error or timeout
     */
    protocol::InitializeResult initialize(
        const protocol::InitializeRequestParams& params
    );

    /**
     * @brief Blocking initialize call with timeout
     *
     * @param params Initialize request parameters
     * @param timeout Maximum duration to wait
     * @return The server's initialize result
     * @throws McpClientBlockingError on error or timeout
     */
    protocol::InitializeResult initialize(
        const protocol::InitializeRequestParams& params,
        std::chrono::milliseconds timeout
    );

    /**
     * @brief Blocking roots/list call
     *
     * Sends a roots/list request and blocks until the response is received.
     *
     * @return The list of roots
     * @throws McpClientBlockingError on error or timeout
     */
    client::ListRootsResult list_roots();

    /**
     * @brief Blocking roots/list call with timeout
     *
     * @param timeout Maximum duration to wait
     * @return The list of roots
     * @throws McpClientBlockingError on error or timeout
     */
    client::ListRootsResult list_roots(std::chrono::milliseconds timeout);

    /**
     * @brief Generic blocking request
     *
     * Sends any JSON-RPC request and blocks until the response is received.
     *
     * @param method The RPC method name
     * @param params Optional parameters object
     * @return The JSON result
     * @throws McpClientBlockingError on error or timeout
     */
    nlohmann::json send_request(
        std::string_view method,
        const nlohmann::json& params = nullptr
    );

    /**
     * @brief Generic blocking request with timeout
     *
     * @param method The RPC method name
     * @param params Optional parameters object
     * @param timeout Maximum duration to wait
     * @return The JSON result
     * @throws McpClientBlockingError on error or timeout
     */
    nlohmann::json send_request(
        std::string_view method,
        const nlohmann::json& params,
        std::chrono::milliseconds timeout
    );

    /**
     * @brief Get the default timeout
     *
     * @return Current default timeout duration
     */
    std::chrono::milliseconds default_timeout() const { return default_timeout_; }

    /**
     * @brief Set the default timeout
     *
     * @param timeout New default timeout duration
     */
    void set_default_timeout(std::chrono::milliseconds timeout) {
        default_timeout_ = timeout;
    }

    /**
     * @brief Get the underlying client
     *
     * @return Reference to the wrapped async client
     */
    McpClient& client() { return client_; }

    /**
     * @brief Get the underlying client (const overload)
     *
     * @return Const reference to the wrapped async client
     */
    const McpClient& client() const { return client_; }

private:
    /// Reference to the async client (not owned)
    McpClient& client_;

    /// Default timeout for blocking operations
    std::chrono::milliseconds default_timeout_;

    /**
     * @brief Wrap a future call with error handling
     *
     * Converts std::future exceptions into McpClientBlockingError.
     *
     * @tparam T The result type
     * @param future The future to wait for
     * @param timeout Maximum duration to wait
     * @return The result value
     * @throws McpClientBlockingError on error or timeout
     */
    template<typename T>
    T wrap_future(std::future<T> future, std::chrono::milliseconds timeout) {
        try {
            return client::FutureBuilder<T>::with_timeout(std::move(future), timeout);
        } catch (const std::runtime_error& e) {
            throw McpClientBlockingError(e.what());
        } catch (const std::exception& e) {
            throw McpClientBlockingError(e.what());
        } catch (...) {
            throw McpClientBlockingError("Unknown error");
        }
    }
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline McpClientBlocking::McpClientBlocking(
    McpClient& client,
    std::chrono::milliseconds default_timeout)
    : client_(client), default_timeout_(default_timeout) {}

inline protocol::InitializeResult McpClientBlocking::initialize(
    const protocol::InitializeRequestParams& params) {
    return initialize(params, default_timeout_);
}

inline protocol::InitializeResult McpClientBlocking::initialize(
    const protocol::InitializeRequestParams& params,
    std::chrono::milliseconds timeout) {
    auto future = client::FutureBuilder<protocol::InitializeResult>::wrap(
        [&](auto on_success, auto on_error) {
            client_.initialize(params, on_success, on_error);
        }
    );
    return wrap_future(std::move(future), timeout);
}

inline client::ListRootsResult McpClientBlocking::list_roots() {
    return list_roots(default_timeout_);
}

inline client::ListRootsResult McpClientBlocking::list_roots(
    std::chrono::milliseconds timeout) {
    auto json_result = client::FutureBuilder<nlohmann::json>::wrap(
        [&](auto on_success, auto on_error) {
            client_.send_request("roots/list", nullptr, on_success, on_error);
        }
    );

    auto parsed_result = wrap_future(std::move(json_result), timeout);

    // Parse JSON into ListRootsResult
    auto result = client::ListRootsResult::from_json(parsed_result);
    if (!result.has_value()) {
        throw McpClientBlockingError("Failed to parse roots/list response");
    }
    return *result;
}

inline nlohmann::json McpClientBlocking::send_request(
    std::string_view method,
    const nlohmann::json& params) {
    return send_request(method, params, default_timeout_);
}

inline nlohmann::json McpClientBlocking::send_request(
    std::string_view method,
    const nlohmann::json& params,
    std::chrono::milliseconds timeout) {
    auto future = client::FutureBuilder<nlohmann::json>::wrap(
        [&](auto on_success, auto on_error) {
            client_.send_request(method, params, on_success, on_error);
        }
    );
    return wrap_future(std::move(future), timeout);
}

} // namespace mcpp

#endif // MCPP_CLIENT_BLOCKING_H
