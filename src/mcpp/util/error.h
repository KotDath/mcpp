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

#ifndef MCPP_UTIL_ERROR_H
#define MCPP_UTIL_ERROR_H

#include <map>
#include <optional>
#include <stdexcept>
#include <string>

namespace mcpp::util {

/**
 * @brief Base class for all service-level errors
 *
 * ServiceError provides structured error information with context preservation
 * for debugging. All error types inherit from std::runtime_error, making them
 * compatible with standard catch blocks.
 *
 * @par Example
 * @code
 * try {
 *     // ... code that may throw ...
 * } catch (const mcpp::util::ServiceError& e) {
 *     std::cerr << "Error: " << e.what() << "\n";
 *     if (e.code()) {
 *         std::cerr << "Code: " << *e.code() << "\n";
 *     }
 *     for (const auto& [key, value] : e.context()) {
 *         std::cerr << "  " << key << ": " << value << "\n";
 *     }
 * }
 * @endcode
 */
class ServiceError : public std::runtime_error {
public:
    /**
     * @brief Construct a ServiceError
     *
     * @param message Human-readable error message
     * @param code Optional error code (e.g., JSON-RPC error code)
     * @param context Optional key-value context for debugging
     */
    ServiceError(
        std::string_view message,
        std::optional<int> code = std::nullopt,
        std::map<std::string, std::string> context = {}
    );

    /**
     * @brief Virtual destructor for proper inheritance
     */
    ~ServiceError() override;

    /**
     * @brief Get the error code if present
     *
     * @return Optional error code (e.g., JSON-RPC error code)
     */
    [[nodiscard]] std::optional<int> code() const noexcept;

    /**
     * @brief Get the context map
     *
     * @return Constant reference to the context key-value pairs
     */
    [[nodiscard]] const std::map<std::string, std::string>& context() const noexcept;

    /**
     * @brief Get the error message
     *
     * @return C string message (from std::runtime_error)
     */
    [[nodiscard]] const char* what() const noexcept override;

protected:
    std::string message_;
    std::optional<int> code_;
    std::map<std::string, std::string> context_;
    mutable std::string what_buffer_; // For constructing what() string
};

/**
 * @brief Error for transport-layer failures
 *
 * Used when the underlying transport fails in ways that prevent
 * communication (connection lost, timeout, etc.).
 */
class TransportError : public ServiceError {
public:
    /**
     * @brief Construct a TransportError
     *
     * @param message Human-readable error message
     * @param code Optional error code
     * @param transport_type Optional transport type (e.g., "stdio", "http", "sse")
     * @param context Additional context key-value pairs
     */
    TransportError(
        std::string_view message,
        std::optional<int> code = std::nullopt,
        std::optional<std::string> transport_type = std::nullopt,
        std::map<std::string, std::string> context = {}
    );

    /**
     * @brief Get the transport type
     *
     * @return Optional transport type string
     */
    [[nodiscard]] std::optional<std::string> transport_type() const noexcept;

private:
    std::optional<std::string> transport_type_;
};

/**
 * @brief Error for protocol violations
 *
 * Used when the received data doesn't conform to the expected protocol
 * (invalid JSON, unexpected message type, missing required fields, etc.).
 */
class ProtocolError : public ServiceError {
public:
    /**
     * @brief Construct a ProtocolError
     *
     * @param message Human-readable error message
     * @param code Optional error code (e.g., JSON-RPC PARSE_ERROR)
     * @param protocol_version Optional protocol version string
     * @param context Additional context key-value pairs
     */
    ProtocolError(
        std::string_view message,
        std::optional<int> code = std::nullopt,
        std::optional<std::string> protocol_version = std::nullopt,
        std::map<std::string, std::string> context = {}
    );

    /**
     * @brief Get the protocol version
     *
     * @return Optional protocol version string
     */
    [[nodiscard]] std::optional<std::string> protocol_version() const noexcept;

private:
    std::optional<std::string> protocol_version_;
};

/**
 * @brief Error for request-specific failures
 *
 * Used when a request fails due to invalid parameters, method not found,
 * or other application-level errors.
 */
class RequestError : public ServiceError {
public:
    /**
     * @brief Construct a RequestError
     *
     * @param message Human-readable error message
     * @param code Optional error code (e.g., JSON-RPC METHOD_NOT_FOUND)
     * @param method Optional method name that failed
     * @param request_id Optional request ID for correlation
     * @param context Additional context key-value pairs
     */
    RequestError(
        std::string_view message,
        std::optional<int> code = std::nullopt,
        std::optional<std::string> method = std::nullopt,
        std::optional<std::string> request_id = std::nullopt,
        std::map<std::string, std::string> context = {}
    );

    /**
     * @brief Get the method name
     *
     * @return Optional method name
     */
    [[nodiscard]] std::optional<std::string> method() const noexcept;

    /**
     * @brief Get the request ID
     *
     * @return Optional request ID string
     */
    [[nodiscard]] std::optional<std::string> request_id() const noexcept;

private:
    std::optional<std::string> method_;
    std::optional<std::string> request_id_;
};

} // namespace mcpp::util

#endif // MCPP_UTIL_ERROR_H
