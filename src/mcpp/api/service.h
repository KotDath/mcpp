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

#ifndef MCPP_API_SERVICE_H
#define MCPP_API_SERVICE_H

#include <concepts>
#include <map>
#include <memory>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

#include "mcpp/api/role.h"
#include "mcpp/core/error.h"
#include "mcpp/core/json_rpc.h"
// Note: atomic_id.h is not directly used in this header but is available
// for RequestContext when it is expanded in future plans

namespace mcpp::api {

/**
 * @brief Request execution context for handling incoming requests
 *
 * RequestContext provides request-scoped information and utilities
 * for handlers processing incoming requests from the remote peer.
 *
 * Thread-safe: Multiple threads may have access to different RequestContext instances.
 * Not thread-safe: A single RequestContext instance should not be shared across threads.
 *
 * TODO: In future plans (06-02), this will be expanded to include:
 * - Cancellation token for cooperative cancellation
 * - Peer reference for sending requests back to the peer
 * - Metadata and extensions from the request
 */
template<ServiceRole Role>
class RequestContext {
public:
    /**
     * @brief Construct a request context
     *
     * @param id Request ID from the JSON-RPC request
     */
    explicit RequestContext(core::RequestId id)
        : id_(std::move(id)) {}

    /**
     * @brief Get the request ID
     *
     * @return The request ID for this context
     */
    const core::RequestId& id() const noexcept { return id_; }

private:
    core::RequestId id_;
};

/**
 * @brief Notification execution context for handling incoming notifications
 *
 * NotificationContext provides notification-scoped information for handlers
 * processing incoming notifications from the remote peer.
 *
 * Thread-safe: Multiple threads may have access to different NotificationContext instances.
 * Not thread-safe: A single NotificationContext instance should not be shared across threads.
 *
 * TODO: In future plans (06-02), this will be expanded to include:
 * - Peer reference for sending requests back to the peer
 * - Metadata and extensions from the notification
 */
template<ServiceRole Role>
class NotificationContext {
public:
    /**
     * @brief Default constructor for notification context
     */
    NotificationContext() = default;
};

/**
 * @brief Client information structure
 *
 * ClientInfo contains identifying information about an MCP client,
 * including name, version, and experimental capabilities for custom
 * feature negotiation beyond the standard MCP capabilities (UTIL-03).
 *
 * The experimental field allows clients to advertise support for
 * custom features not yet standardized in the MCP protocol.
 */
struct ClientInfo {
    /// Client name (e.g., "my-mcp-client")
    std::string name;

    /// Client version (e.g., "1.0.0")
    std::string version;

    /**
     * @brief Experimental capabilities for custom feature negotiation
     *
     * The experimental field allows custom feature negotiation beyond
     * standard MCP capabilities. This is keyed by feature name with
     * JSON values describing the feature support (UTIL-03).
     *
     * Example:
     *   experimental["customStreaming"] = true;
     *   experimental["myCustomProtocol"] = {{"version", 2}};
     */
    std::map<std::string, nlohmann::json> experimental;

    /**
     * @brief Default constructor
     */
    ClientInfo() = default;

    /**
     * @brief Construct with name and version
     *
     * @param n Client name
     * @param v Client version
     */
    ClientInfo(std::string n, std::string v)
        : name(std::move(n)), version(std::move(v)) {}

    /**
     * @brief Convert ClientInfo to JSON
     *
     * Serializes the client info to a JSON object following the
     * MCP Implementation format with experimental capabilities.
     *
     * @return JSON representation of this ClientInfo
     */
    nlohmann::json to_json() const {
        nlohmann::json j;
        j["name"] = name;
        j["version"] = version;
        if (!experimental.empty()) {
            j["experimental"] = experimental;
        }
        return j;
    }
};

/**
 * @brief Server information structure
 *
 * ServerInfo contains identifying information about an MCP server,
 * including name, version, and experimental capabilities for custom
 * feature negotiation beyond the standard MCP capabilities (UTIL-03).
 *
 * The experimental field allows servers to advertise support for
 * custom features not yet standardized in the MCP protocol.
 */
struct ServerInfo {
    /// Server name (e.g., "my-mcp-server")
    std::string name;

    /// Server version (e.g., "1.0.0")
    std::string version;

    /**
     * @brief Experimental capabilities for custom feature negotiation
     *
     * The experimental field allows custom feature negotiation beyond
     * standard MCP capabilities. This is keyed by feature name with
     * JSON values describing the feature support (UTIL-03).
     *
     * Example:
     *   experimental["customStreaming"] = true;
     *   experimental["myCustomProtocol"] = {{"version", 2}};
     */
    std::map<std::string, nlohmann::json> experimental;

    /**
     * @brief Default constructor
     */
    ServerInfo() = default;

    /**
     * @brief Construct with name and version
     *
     * @param n Server name
     * @param v Server version
     */
    ServerInfo(std::string n, std::string v)
        : name(std::move(n)), version(std::move(v)) {}

    /**
     * @brief Convert ServerInfo to JSON
     *
     * Serializes the server info to a JSON object following the
     * MCP Implementation format with experimental capabilities.
     *
     * @return JSON representation of this ServerInfo
     */
    nlohmann::json to_json() const {
        nlohmann::json j;
        j["name"] = name;
        j["version"] = version;
        if (!experimental.empty()) {
            j["experimental"] = experimental;
        }
        return j;
    }
};

/**
 * @brief Type trait for role-specific types
 *
 * The RoleTypes trait provides role-specific type aliases used by
 * the Service trait abstraction. This allows the same Service template
 * to work with both client and server roles while maintaining type safety.
 *
 * For RoleClient:
 *   - PeerReq: Request type from the server (JsonValue for now)
 *   - PeerResp: Response type to the server (JsonValue for now)
 *   - PeerNot: Notification type from the server (JsonValue for now)
 *   - Info: ClientInfo for self-description
 *   - PeerInfo: ServerInfo for describing the remote peer
 *
 * For RoleServer:
 *   - PeerReq: Request type from the client (JsonValue for now)
 *   - PeerResp: Response type to the client (JsonValue for now)
 *   - PeerNot: Notification type from the client (JsonValue for now)
 *   - Info: ServerInfo for self-description
 *   - PeerInfo: ClientInfo for describing the remote peer
 */
template<ServiceRole Role>
struct RoleTypes;

/**
 * @brief Role types specialization for RoleClient
 */
template<>
struct RoleTypes<RoleClient> {
    using PeerReq = core::JsonValue;
    using PeerResp = core::JsonValue;
    using PeerNot = core::JsonValue;
    using Info = ClientInfo;
    using PeerInfo = ServerInfo;
};

/**
 * @brief Role types specialization for RoleServer
 */
template<>
struct RoleTypes<RoleServer> {
    using PeerReq = core::JsonValue;
    using PeerResp = core::JsonValue;
    using PeerNot = core::JsonValue;
    using Info = ServerInfo;
    using PeerInfo = ClientInfo;
};

/**
 * @brief Service trait abstraction for MCP handlers
 *
 * Service is an abstract interface that defines the contract for handling
 * MCP requests and notifications. It is polymorphic on the Role type,
 * providing compile-time type safety for role-specific behavior.
 *
 * Classes inherit from Service<Role> and implement the pure virtual
 * methods to handle incoming requests and notifications from the remote peer.
 *
 * Thread safety: Service implementations must be thread-safe if they
 * will be accessed from multiple threads. The Service interface itself
 * does not provide synchronization.
 *
 * Example usage:
 *   class MyServerHandler : public Service<RoleServer> {
 *   public:
 *       void handle_request(const PeerReq& req, RequestContext& ctx) override {
 *           // Handle request from client
 *       }
 *       void handle_notification(const PeerNot& not, NotificationContext& ctx) override {
 *           // Handle notification from client
 *       }
 *       ServerInfo get_info() const override {
 *           return ServerInfo{"my-server", "1.0.0"};
 *       }
 *   };
 *
 * @tparam Role The service role (RoleClient or RoleServer)
 */
template<ServiceRole Role>
class Service {
public:
    // Import role-specific types from RoleTypes trait
    using PeerReq = typename RoleTypes<Role>::PeerReq;
    using PeerResp = typename RoleTypes<Role>::PeerResp;
    using PeerNot = typename RoleTypes<Role>::PeerNot;
    using Info = typename RoleTypes<Role>::Info;
    using PeerInfo = typename RoleTypes<Role>::PeerInfo;

    /**
     * @brief Virtual destructor for safe polymorphic deletion
     */
    virtual ~Service() = default;

    /**
     * @brief Handle an incoming request from the remote peer
     *
     * Called when the remote peer sends a JSON-RPC request to this service.
     * The implementation should process the request and return a response.
     *
     * Thread safety: This method may be called from multiple threads concurrently.
     * Implementations must provide their own synchronization if needed.
     *
     * @param request The request from the remote peer
     * @param ctx Context for this request (contains request ID, metadata, etc.)
     * @return Response to send back to the remote peer
     */
    virtual void handle_request(
        const PeerReq& request,
        RequestContext<Role>& ctx
    ) = 0;

    /**
     * @brief Handle an incoming notification from the remote peer
     *
     * Called when the remote peer sends a JSON-RPC notification to this service.
     * Notifications are one-way and do not expect a response.
     *
     * Thread safety: This method may be called from multiple threads concurrently.
     * Implementations must provide their own synchronization if needed.
     *
     * @param notification The notification from the remote peer
     * @param ctx Context for this notification
     */
    virtual void handle_notification(
        const PeerNot& notification,
        NotificationContext<Role>& ctx
    ) = 0;

    /**
     * @brief Get information about this service
     *
     * Returns information describing this service, including name, version,
     * and experimental capabilities (UTIL-03). This is used during the
     * MCP initialize handshake.
     *
     * Thread safety: This method may be called from multiple threads concurrently.
     * Implementations must ensure thread-safe access to internal state.
     *
     * @return Info object describing this service
     */
    virtual Info get_info() const = 0;
};

} // namespace mcpp::api

#endif // MCPP_API_SERVICE_H
