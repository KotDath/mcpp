#ifndef MCPP_ASYNC_CALLBACKS_H
#define MCPP_ASYNC_CALLBACKS_H

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <variant>

#include <nlohmann/json.hpp>

namespace mcpp::core {

// Type alias for JSON values - using nlohmann::json
using JsonValue = nlohmann::json;

// Request ID can be either a number or a string per JSON-RPC 2.0 spec
using RequestId = std::variant<int64_t, std::string>;

// Forward declaration for JsonRpcError (defined in error.h)
struct JsonRpcError;

} // namespace mcpp::core

namespace mcpp::async {

// Import core types into async namespace for convenience
using core::JsonRpcError;
using core::JsonValue;
using core::RequestId;

/**
 * Callback invoked when a JSON-RPC request receives a successful response.
 *
 * @param result The result value from the response
 *
 * Usage:
 *   ResponseCallback on_success = [](const JsonValue& result) {
 *       std::cout << "Got result: " << result << std::endl;
 *   };
 */
using ResponseCallback = std::function<void(const JsonValue& result)>;

/**
 * Callback invoked when a JSON-RPC request receives an error response.
 *
 * @param error The JSON-RPC error object
 *
 * Usage:
 *   ErrorCallback on_error = [](const JsonRpcError& error) {
 *       std::cerr << "Error " << error.code << ": " << error.message << std::endl;
 *   };
 */
using ErrorCallback = std::function<void(const JsonRpcError& error)>;

/**
 * Callback invoked when a JSON-RPC notification is received.
 *
 * Notifications are one-way messages that do not have a request ID
 * and do not expect a response.
 *
 * @param method The method name of the notification
 * @param params The parameters object (may be null)
 *
 * Usage:
 *   NotificationCallback on_notify = [](std::string_view method, const JsonValue& params) {
 *       std::cout << "Notification: " << method << std::endl;
 *   };
 */
using NotificationCallback = std::function<void(std::string_view method, const JsonValue& params)>;

/**
 * Callback invoked when a request times out.
 *
 * @param id The request ID that timed out
 *
 * Usage:
 *   TimeoutCallback on_timeout = [](RequestId id) {
 *       std::cout << "Request timed out" << std::endl;
 *   };
 */
using TimeoutCallback = std::function<void(RequestId id)>;

/**
 * @brief Callback lifetime management
 *
 * Callbacks are stored as std::function copies by the library.
 * After registering a callback with the library, users may discard
 * their original copy - the library maintains its own copy for
 * the lifetime of the pending request.
 *
 * Example:
 *   client.call_tool("my_tool", args,
 *       [](const JsonValue& result) { /* handle success */ },
 *       [](const JsonRpcError& error) { /* handle error */ }
 *   );
 *   // Lambda is copied; can go out of scope here safely
 *
 * Thread safety: Callbacks are invoked on the same thread that
 * processes incoming messages from the transport layer. Avoid
 * blocking operations in callbacks to prevent message processing stalls.
 */

} // namespace mcpp::async

#endif // MCPP_ASYNC_CALLBACKS_H
