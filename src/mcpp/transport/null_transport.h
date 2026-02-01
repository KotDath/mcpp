// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#ifndef MCPP_TRANSPORT_NULL_TRANSPORT_H
#define MCPP_TRANSPORT_NULL_TRANSPORT_H

#include "mcpp/transport/transport.h"

#include <string_view>

namespace mcpp {
namespace transport {

/**
 * @brief Null/no-op transport for servers that don't need outbound notifications
 *
 * This transport implementation does nothing - all sends are no-ops,
 * connection status is always true. This is useful for servers that
 * handle their own I/O (like the inspector_server example) but still
 * need to satisfy the McpServer's transport requirement for RequestContext.
 */
class NullTransport : public Transport {
public:
    /**
     * @brief Constructor
     */
    NullTransport() = default;

    /**
     * @brief Destructor
     */
    ~NullTransport() override = default;

    /**
     * @brief Connect - no-op for null transport
     */
    bool connect() override { return true; }

    /**
     * @brief Disconnect - no-op for null transport
     */
    void disconnect() override {}

    /**
     * @brief Check if connected - always true for null transport
     */
    bool is_connected() const override { return true; }

    /**
     * @brief Send - no-op for null transport (always succeeds)
     */
    bool send(std::string_view message) override {
        (void)message;
        return true;
    }

    /**
     * @brief Set message callback - no-op for null transport
     */
    void set_message_callback(MessageCallback cb) override {
        (void)cb;
    }

    /**
     * @brief Set error callback - no-op for null transport
     */
    void set_error_callback(ErrorCallback cb) override {
        (void)cb;
    }
};

} // namespace transport
} // namespace mcpp

#endif // MCPP_TRANSPORT_NULL_TRANSPORT_H
