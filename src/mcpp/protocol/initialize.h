#ifndef MCPP_PROTOCOL_INITIALIZE_H
#define MCPP_PROTOCOL_INITIALIZE_H

#include <string_view>

#include "../core/json_rpc.h"
#include "capabilities.h"
#include "types.h"

namespace mcpp::protocol {

/**
 * MCP protocol version string.
 *
 * Current supported version as per MCP 2025-11-25 schema.
 */
constexpr const char* PROTOCOL_VERSION = "2025-11-25";

/**
 * Parameters for the initialize request from client to server.
 *
 * Corresponds to the "InitializeRequestParams" type in MCP 2025-11-25 schema.
 * Sent by the client to initiate the MCP session.
 *
 * Example JSON:
 * {
 *   "protocolVersion": "2025-11-25",
 *   "capabilities": {"roots": {"listChanged": true}},
 *   "clientInfo": {"name": "my-client", "version": "1.0.0"},
 *   "_meta": {"progressToken": "init-123"}
 * }
 */
struct InitializeRequestParams {
    std::string protocolVersion;
    ClientCapabilities capabilities;
    Implementation clientInfo;
    std::optional<RequestMeta> _meta;
};

/**
 * Result returned by the server in response to initialize.
 *
 * Corresponds to the "InitializeResult" type in MCP 2025-11-25 schema.
 * Contains server capabilities and information.
 *
 * Example JSON:
 * {
 *   "protocolVersion": "2025-11-25",
 *   "capabilities": {"tools": {"listChanged": true}},
 *   "serverInfo": {"name": "my-server", "version": "1.0.0"},
 *   "instructions": "This server provides..."
 * }
 */
struct InitializeResult {
    std::string protocolVersion;
    ServerCapabilities capabilities;
    Implementation serverInfo;
    std::optional<std::string> instructions;
};

/**
 * Parameters for the initialized notification from client to server.
 *
 * Corresponds to the "InitializedNotificationParams" type in MCP 2025-11-25 schema.
 * Sent by the client after receiving the initialize result.
 * This notification has no parameters - it's a signal that initialization is complete.
 *
 * Example JSON:
 * {"jsonrpc": "2.0", "method": "notifications/initialized"}
 */
struct InitializedNotificationParams {
    // Empty struct - no fields required
};

/**
 * Create an initialize request JSON-RPC message.
 *
 * Helper function to construct a properly formatted initialize request.
 *
 * @param params The initialize request parameters
 * @param id The request ID (default: 1)
 * @return A JsonRpcRequest for initialization
 */
inline mcpp::core::JsonRpcRequest make_initialize_request(
    const InitializeRequestParams& params,
    mcpp::core::RequestId id = int64_t{1}
) {
    mcpp::core::JsonRpcRequest request;
    request.id = id;
    request.method = "initialize";
    request.params = mcpp::core::JsonValue{
        {"protocolVersion", params.protocolVersion},
        {"capabilities", mcpp::core::JsonValue{
            {"experimental", params.capabilities.experimental.has_value()
                ? *params.capabilities.experimental : mcpp::core::JsonValue()},
            {"roots", params.capabilities.roots.has_value()
                ? mcpp::core::JsonValue{} : mcpp::core::JsonValue(nullptr)},
            {"sampling", params.capabilities.sampling.has_value()
                ? mcpp::core::JsonValue{} : mcpp::core::JsonValue(nullptr)}
        }},
        {"clientInfo", mcpp::core::JsonValue{
            {"name", params.clientInfo.name},
            {"version", params.clientInfo.version}
        }}
    };

    // Add optional _meta if present
    if (params._meta.has_value()) {
        request.params["_meta"] = mcpp::core::JsonValue{};
        if (params._meta->progressToken.has_value()) {
            request.params["_meta"]["progressToken"] = *params._meta->progressToken;
        }
        if (params._meta->mimeType.has_value()) {
            request.params["_meta"]["mimeType"] = *params._meta->mimeType;
        }
    }

    return request;
}

/**
 * Validate a protocol version string.
 *
 * Checks if the given protocol version is supported.
 * Currently requires exact match with PROTOCOL_VERSION.
 *
 * @param version The protocol version to validate
 * @return true if the version is supported, false otherwise
 */
inline bool validate_protocol_version(std::string_view version) {
    return version == PROTOCOL_VERSION;
}

} // namespace mcpp::protocol

#endif // MCPP_PROTOCOL_INITIALIZE_H
