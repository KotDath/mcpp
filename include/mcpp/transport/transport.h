#pragma once

#include "mcpp/core/json_rpc_message.h"
#include <memory>
#include <future>

namespace mcpp::transport {

/**
 * @brief Abstract transport interface for MCP communication
 *
 * This class defines the interface that all MCP transports must implement.
 * Different transport mechanisms (stdio, HTTP, SSE) can be used by implementing
 * this interface.
 */
class Transport {
public:
    virtual ~Transport() = default;

    /**
     * @brief Send a JSON-RPC message
     * @param message The message to send
     * @throws TransportException if sending fails
     */
    virtual void send(const core::JsonRpcMessage& message) = 0;

    /**
     * @brief Receive a JSON-RPC message asynchronously
     * @return Future containing the received message
     * @throws TransportException if receiving fails
     */
    virtual std::future<std::unique_ptr<core::JsonRpcMessage>> receive() = 0;

    /**
     * @brief Close the transport
     */
    virtual void close() = 0;

    /**
     * @brief Check if the transport is open
     */
    virtual bool is_open() const = 0;

    /**
     * @brief Get transport description
     */
    virtual std::string get_description() const = 0;
};

} // namespace mcpp::transport