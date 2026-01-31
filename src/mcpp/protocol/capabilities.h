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
struct ElicitationCapability;
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
    std::optional<ElicitationCapability> elicitation;

    // Builder typedef for fluent API
    class Builder;
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

    RootsCapability() = default;
    explicit RootsCapability(bool list_changed) : listChanged(list_changed) {}
};

/**
 * Sampling capability - indicates client supports sampling.
 *
 * Corresponds to "SamplingCapability" in MCP 2025-11-25 schema.
 */
struct SamplingCapability {
    std::optional<bool> tools;  // Client supports tool use in sampling (CLNT-03)

    SamplingCapability() = default;
    explicit SamplingCapability(bool tools_enabled) : tools(tools_enabled) {}
};

/**
 * Elicitation capability - indicates client supports elicitation.
 *
 * Corresponds to "ElicitationCapability" in MCP 2025-11-25 schema.
 */
struct ElicitationCapability {
    std::optional<bool> form;  // Client supports form mode (CLNT-04)
    std::optional<bool> url;   // Client supports URL mode (CLNT-05)

    ElicitationCapability() = default;
};

/**
 * Logging capability - indicates server supports logging.
 *
 * Corresponds to "LoggingCapability" in MCP 2025-11-25 schema.
 */
struct LoggingCapability {
    // No fields defined in current spec
};

/**
 * Builder helper for constructing ClientCapabilities.
 *
 * Provides a fluent interface for configuring which capabilities
 * the client advertises during initialization.
 *
 * Example usage:
 *   auto caps = ClientCapabilities::Builder()
 *       .with_roots(true)           // Enable roots with listChanged
 *       .with_sampling(true)        // Enable sampling without tool use
 *       .with_elicitation_form()    // Enable elicitation form mode
 *       .build();
 */
class ClientCapabilities::Builder {
public:
    Builder() = default;

    // Enable roots capability
    Builder& with_roots(bool list_changed = true) {
        caps_.roots = RootsCapability{list_changed};
        return *this;
    }

    // Enable sampling capability
    Builder& with_sampling(bool tools = false) {
        caps_.sampling = SamplingCapability{tools};
        return *this;
    }

    // Enable elicitation form mode
    Builder& with_elicitation_form() {
        if (!caps_.elicitation) {
            caps_.elicitation = ElicitationCapability{};
        }
        caps_.elicitation->form = true;
        return *this;
    }

    // Enable elicitation URL mode
    Builder& with_elicitation_url() {
        if (!caps_.elicitation) {
            caps_.elicitation = ElicitationCapability{};
        }
        caps_.elicitation->url = true;
        return *this;
    }

    // Enable both elicitation modes
    Builder& with_elicitation() {
        return with_elicitation_form().with_elicitation_url();
    }

    // Add experimental capabilities
    Builder& with_experimental(CapabilitySet experimental) {
        caps_.experimental = std::move(experimental);
        return *this;
    }

    // Build the ClientCapabilities struct (rvalue)
    ClientCapabilities build() && {
        return std::move(caps_);
    }

    // Build the ClientCapabilities struct (lvalue)
    ClientCapabilities build() const & {
        return caps_;
    }

private:
    ClientCapabilities caps_;
};

/**
 * Convenience free function for simple client capability configuration.
 *
 * Provides a quick way to create ClientCapabilities for common cases.
 * For more complex configurations, use ClientCapabilities::Builder.
 *
 * @param roots Enable roots capability with listChanged
 * @param sampling Enable sampling capability without tool use
 * @param elicitation_form Enable elicitation form mode
 * @param elicitation_url Enable elicitation URL mode
 * @return Configured ClientCapabilities
 */
inline ClientCapabilities build_client_capabilities(
    bool roots = false,
    bool sampling = false,
    bool elicitation_form = false,
    bool elicitation_url = false
) {
    ClientCapabilities::Builder builder;
    if (roots) builder.with_roots();
    if (sampling) builder.with_sampling();
    if (elicitation_form || elicitation_url) {
        if (elicitation_form) builder.with_elicitation_form();
        if (elicitation_url) builder.with_elicitation_url();
    }
    return std::move(builder).build();
}

} // namespace mcpp::protocol

#endif // MCPP_PROTOCOL_CAPABILITIES_H
