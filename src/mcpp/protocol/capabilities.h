#ifndef MCPP_PROTOCOL_CAPABILITIES_H
#define MCPP_PROTOCOL_CAPABILITIES_H

#include <nlohmann/json.hpp>
#include <optional>

#include "types.h"

namespace mcpp::protocol {

/**
 * Type alias for experimental capabilities.
 * Experimental capabilities are arbitrary JSON objects.
 */
using CapabilitySet = nlohmann::json;

// Forward declarations
struct ToolCapability;
struct ResourceCapability;
struct PromptCapability;
struct RootsCapability;
struct SamplingCapability;
struct LoggingCapability;

/**
 * Client capabilities advertised during initialization.
 *
 * Corresponds to the "ClientCapabilities" type in MCP 2025-11-25 schema.
 * Indicates which optional MCP features the client supports.
 *
 * Example JSON:
 * {
 *   "experimental": {"customFeature": true},
 *   "roots": {"listChanged": true},
 *   "sampling": {}
 * }
 */
struct ClientCapabilities {
    std::optional<CapabilitySet> experimental;
    std::optional<RootsCapability> roots;
    std::optional<SamplingCapability> sampling;
};

/**
 * Server capabilities advertised during initialization.
 *
 * Corresponds to the "ServerCapabilities" type in MCP 2025-11-25 schema.
 * Indicates which optional MCP features the server supports.
 *
 * Example JSON:
 * {
 *   "experimental": {"customFeature": true},
 *   "logging": {},
 *   "prompts": {"listChanged": true},
 *   "resources": {"subscribe": true, "listChanged": true},
 *   "tools": {"listChanged": true}
 * }
 */
struct ServerCapabilities {
    std::optional<CapabilitySet> experimental;
    std::optional<LoggingCapability> logging;
    std::optional<PromptCapability> prompts;
    std::optional<ResourceCapability> resources;
    std::optional<ToolCapability> tools;
};

/**
 * Tool capability - indicates server supports tools.
 *
 * Corresponds to "ToolsCapability" in MCP 2025-11-25 schema.
 */
struct ToolCapability {
    std::optional<bool> listChanged;  // Server supports listChanged notifications
};

/**
 * Resource capability - indicates server supports resources.
 *
 * Corresponds to "ResourcesCapability" in MCP 2025-11-25 schema.
 */
struct ResourceCapability {
    bool subscribe = false;            // Server supports resource subscription
    std::optional<bool> listChanged;   // Server supports listChanged notifications
};

/**
 * Prompt capability - indicates server supports prompts.
 *
 * Corresponds to "PromptsCapability" in MCP 2025-11-25 schema.
 */
struct PromptCapability {
    std::optional<bool> listChanged;  // Server supports listChanged notifications
};

/**
 * Roots capability - indicates client supports roots.
 *
 * Corresponds to "RootsCapability" in MCP 2025-11-25 schema.
 */
struct RootsCapability {
    std::optional<bool> listChanged;  // Client supports listChanged notifications
};

/**
 * Sampling capability - indicates client supports sampling.
 *
 * Corresponds to "SamplingCapability" in MCP 2025-11-25 schema.
 */
struct SamplingCapability {
    // No fields defined in current spec
};

/**
 * Logging capability - indicates server supports logging.
 *
 * Corresponds to "LoggingCapability" in MCP 2025-11-25 schema.
 */
struct LoggingCapability {
    // No fields defined in current spec
};

} // namespace mcpp::protocol

#endif // MCPP_PROTOCOL_CAPABILITIES_H
