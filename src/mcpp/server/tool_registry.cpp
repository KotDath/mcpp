// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/tool_registry.h"

namespace mcpp {
namespace server {

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

    // Create and store the tool registration
    ToolRegistration registration{
        name,
        description,
        input_schema,
        std::move(handler)
    };

    // Note: validator will be compiled in Task 3
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

    // Note: JSON Schema validation will be added in Task 3
    // For now, call the handler directly

    // Call the handler
    return it->second.handler(name, args, ctx);
}

bool ToolRegistry::has_tool(const std::string& name) const {
    return tools_.find(name) != tools_.end();
}

} // namespace server
} // namespace mcpp
