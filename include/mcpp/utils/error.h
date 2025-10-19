#pragma once

#include "mcpp/core/json_rpc_message.h"
#include <stdexcept>
#include <string>

namespace mcpp::utils {

/**
 * @brief Base MCP exception
 */
class McpException : public std::runtime_error {
public:
    explicit McpException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Protocol exception
 */
class ProtocolException : public McpException {
public:
    explicit ProtocolException(const std::string& message)
        : McpException("Protocol error: " + message) {}
};

/**
 * @brief Transport exception
 */
class TransportException : public McpException {
public:
    explicit TransportException(const std::string& message)
        : McpException("Transport error: " + message) {}
};

/**
 * @brief JSON-RPC exception
 */
class JsonRpcException : public McpException {
public:
    JsonRpcException(core::JsonRpcErrorCode code, const std::string& message)
        : McpException("JSON-RPC error " + std::to_string(static_cast<int>(code)) + ": " + message)
        , code_(code) {}

    core::JsonRpcErrorCode code() const { return code_; }

private:
    core::JsonRpcErrorCode code_;
};

/**
 * @brief Validation exception
 */
class ValidationException : public McpException {
public:
    explicit ValidationException(const std::string& message)
        : McpException("Validation error: " + message) {}
};

/**
 * @brief Create JSON-RPC error response
 */
std::unique_ptr<core::JsonRpcResponse> create_error_response(
    const core::JsonRpcResponse::IdType& id,
    core::JsonRpcErrorCode code,
    const std::string& message,
    const std::optional<nlohmann::json>& data = std::nullopt
);

} // namespace mcpp::utils