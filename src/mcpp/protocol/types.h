#ifndef MCPP_PROTOCOL_TYPES_H
#define MCPP_PROTOCOL_TYPES_H

#include <optional>
#include <string>

namespace mcpp::protocol {

/**
 * Implementation information for an MCP client or server.
 *
 * Corresponds to the "Implementation" type in MCP 2025-11-25 schema.
 * Used to identify the client/server in initialize messages.
 *
 * Example JSON:
 * {"name": "my-client", "version": "1.0.0"}
 */
struct Implementation {
    std::string name;
    std::string version;
};

/**
 * Metadata attached to requests.
 *
 * Corresponds to the "RequestMeta" type in MCP 2025-11-25 schema.
 * Contains optional metadata for requests like progress tokens and MIME types.
 *
 * Example JSON:
 * {"progressToken": "123", "mimeType": "application/json"}
 */
struct RequestMeta {
    std::optional<std::string> progressToken;
    std::optional<std::string> mimeType;
};

} // namespace mcpp::protocol

#endif // MCPP_PROTOCOL_TYPES_H
