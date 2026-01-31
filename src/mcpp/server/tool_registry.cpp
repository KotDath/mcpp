// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/tool_registry.h"

#include <nlohmann/json-schema.hpp>

namespace mcpp {
namespace server {

namespace {

// JSON-RPC error codes for tool validation
constexpr int JSONRPC_INVALID_PARAMS = -32602;

/**
 * @brief Create a JSON-RPC error response for invalid arguments
 *
 * @param message Error message describing what failed validation
 * @return JSON error object in JSON-RPC format
 */
nlohmann::json make_validation_error(const std::string& message) {
    return nlohmann::json{
        {"error", {
            {"code", JSONRPC_INVALID_PARAMS},
            {"message", "Invalid arguments"},
            {"data", message}
        }}
    };
}

} // anonymous namespace

bool ToolRegistry::register_tool(
    const std::string& name,
    const std::string& description,
    const nlohmann::json& input_schema,
    ToolHandler handler
) {
    // Check if tool already exists
    if (tools_.find(name) != tools_.end()) {
        return false;
    }

    // Create the validator and compile the schema
    // The schema is compiled once during registration for efficient validation
    auto validator = std::make_unique<nlohmann::json_schema::json_validator>();
    try {
        validator->set_root_schema(input_schema);
    } catch (const std::exception& e) {
        // Schema compilation failed - invalid schema
        // We return false to indicate registration failure
        return false;
    }

    // Create and store the tool registration
    ToolRegistration registration{
        name,
        description,
        input_schema,
        std::move(validator),
        std::move(handler)
    };

    tools_[name] = std::move(registration);
    return true;
}

std::vector<nlohmann::json> ToolRegistry::list_tools() const {
    std::vector<nlohmann::json> result;
    result.reserve(tools_.size());

    for (const auto& [name, registration] : tools_) {
        nlohmann::json tool{
            {"name", registration.name},
            {"description", registration.description},
            {"inputSchema", registration.input_schema}
        };
        result.push_back(std::move(tool));
    }

    return result;
}

std::optional<nlohmann::json> ToolRegistry::call_tool(
    const std::string& name,
    const nlohmann::json& args,
    RequestContext& ctx
) const {
    // Find the tool
    auto it = tools_.find(name);
    if (it == tools_.end()) {
        return std::nullopt;
    }

    const ToolRegistration& registration = it->second;

    // Validate arguments against the compiled schema
    try {
        // The validate() method throws a std::exception if validation fails
        // and returns the validated JSON if it succeeds
        registration.validator->validate(args);
    } catch (const std::exception& e) {
        // Validation failed - return JSON-RPC INVALID_PARAMS error
        return make_validation_error(e.what());
    }

    // Call the handler with validated arguments
    return registration.handler(name, args, ctx);
}

bool ToolRegistry::has_tool(const std::string& name) const {
    return tools_.find(name) != tools_.end();
}

} // namespace server
} // namespace mcpp
