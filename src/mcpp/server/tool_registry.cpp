// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/tool_registry.h"

#include <nlohmann/json-schema.hpp>
#include <sstream>
#include <string>

namespace mcpp {
namespace server {

namespace {

// JSON-RPC error codes for tool validation
// See: https://www.jsonrpc.org/specification
constexpr int JSONRPC_INVALID_PARAMS = -32602;

/**
 * @brief Create a JSON-RPC error response for invalid arguments
 *
 * Generates a properly formatted JSON-RPC error response for
 * validation failures. The error code follows the JSON-RPC 2.0
 * specification with INVALID_PARAMS (-32602) for schema validation.
 *
 * Error format:
 * ```json
 * {
 *   "error": {
 *     "code": -32602,
 *     "message": "Invalid arguments",
 *     "data": "Detailed validation error message"
 *   }
 * }
 * ```
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

/**
 * @brief Create an output validation error response
 *
 * When a tool's output does not match its declared outputSchema,
 * we return a CallToolResult with isError=true rather than a
 * JSON-RPC error. This allows the tool to communicate validation
 * failures to the client without breaking the JSON-RPC response
 * contract.
 *
 * Error format (CallToolResult with isError):
 * ```json
 * {
 *   "content": [
 *     {
 *       "type": "text",
 *       "text": "Output validation failed: [details]"
 *     }
 *   ],
 *   "isError": true
 * }
 * ```
 *
 * @param message Validation error message from the JSON Schema validator
 * @return CallToolResult formatted JSON with isError=true
 */
nlohmann::json make_output_validation_error(const std::string& message) {
    std::ostringstream oss;
    oss << "Output validation failed: " << message;
    return nlohmann::json{
        {"content", {{{"type", "text"}, {"text", oss.str()}}}},
        {"isError", true}
    };
}

} // anonymous namespace

bool ToolRegistry::register_tool(
    const std::string& name,
    const std::string& description,
    const nlohmann::json& input_schema,
    ToolHandler handler
) {
    // Use defaults for annotations and no output schema
    // This provides backward compatibility for existing code
    return register_tool(name, description, input_schema, std::nullopt, ToolAnnotations{}, std::move(handler));
}

bool ToolRegistry::register_tool(
    const std::string& name,
    const std::string& description,
    const nlohmann::json& input_schema,
    const std::optional<nlohmann::json>& output_schema,
    const ToolAnnotations& annotations,
    ToolHandler handler
) {
    // Check if tool already exists - tool names must be unique
    if (tools_.find(name) != tools_.end()) {
        return false;
    }

    // Create the validator and compile the input schema
    //
    // The schema is compiled once during registration for efficient validation.
    // nlohmann/json_schema_validator supports JSON Schema Draft 7, which includes:
    // - type validation (string, number, object, array, boolean, null)
    // - required fields
    // - enum constraints
    // - format validation (email, uri, date-time, etc.)
    // - numeric ranges (minimum, maximum, exclusiveMinimum, exclusiveMaximum)
    // - string length (minLength, maxLength)
    // - array constraints (minItems, maxItems, uniqueItems)
    // - pattern (regex) validation
    //
    auto validator = std::make_unique<nlohmann::json_schema::json_validator>();
    try {
        validator->set_root_schema(input_schema);
    } catch (const std::exception& e) {
        // Schema compilation failed - invalid schema
        // We return false to indicate registration failure
        // Callers should check the return value and handle the error appropriately
        return false;
    }

    // Create output validator if output schema is provided
    //
    // Output schema validation ensures tool results match the declared structure.
    // This is particularly useful for:
    // - API contract enforcement
    // - Type safety in distributed systems
    // - Automatic documentation generation
    // - Client-side code generation
    //
    // Validation happens after the handler returns but before the result is sent
    // to the client. If validation fails, an error response is returned instead.
    std::unique_ptr<nlohmann::json_schema::json_validator> output_validator;
    if (output_schema) {
        output_validator = std::make_unique<nlohmann::json_schema::json_validator>();
        try {
            output_validator->set_root_schema(*output_schema);
        } catch (const std::exception& e) {
            // Output schema compilation failed - invalid schema
            // Return false to indicate registration failure
            return false;
        }
    }

    // Create and store the tool registration
    // The initializer list must match the ToolRegistration struct order
    ToolRegistration registration{
        name,
        description,
        input_schema,
        std::move(validator),
        output_schema,
        std::move(output_validator),
        annotations,
        std::move(handler)
    };

    tools_[name] = std::move(registration);
    notify_changed();
    return true;
}

std::vector<nlohmann::json> ToolRegistry::list_tools() const {
    std::vector<nlohmann::json> result;
    result.reserve(tools_.size());

    for (const auto& [name, registration] : tools_) {
        // Build the tool object following MCP tools/list response format
        nlohmann::json tool{
            {"name", registration.name},
            {"description", registration.description},
            {"inputSchema", registration.input_schema}
        };

        // Add annotations metadata for rich tool discovery
        //
        // The annotations field provides UI/UX hints:
        // - destructive: Warn user before execution (e.g., delete_file, drop_database)
        // - readOnly: Indicates no side effects (e.g., read_file, list_directories)
        // - audience: Filter tools by target user type
        //   * "user" - End-user facing tools
        //   * "assistant" - AI assistant internal tools
        //   * "system" - System administration tools
        // - priority: Sort order (lower values shown first)
        //
        tool["annotations"] = {
            {"destructive", registration.annotations.destructive},
            {"readOnly", registration.annotations.read_only},
            {"audience", registration.annotations.audience},
            {"priority", registration.annotations.priority}
        };

        // Add output schema if declared
        //
        // When present, the outputSchema field tells clients what structure
        // to expect from the tool result. This enables:
        // - Client-side result validation
        // - Type-safe result parsing
        // - IDE autocomplete for tool results
        // - Automatic documentation generation
        //
        if (registration.output_schema) {
            tool["outputSchema"] = *registration.output_schema;
        }

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
    //
    // Input validation ensures handlers receive well-formed arguments.
    // The validate() method throws std::exception on validation failure
    // with a descriptive message about what failed.
    //
    try {
        registration.validator->validate(args);
    } catch (const std::exception& e) {
        // Validation failed - return JSON-RPC INVALID_PARAMS error
        // This is a protocol-level error, not a tool error
        return make_validation_error(e.what());
    }

    // Call the handler with validated arguments
    nlohmann::json result = registration.handler(name, args, ctx);

    // Validate output against output schema if declared
    //
    // Output validation is optional but provides strong guarantees about
    // tool result structure. When validation fails:
    // 1. We return a CallToolResult with isError=true (not a JSON-RPC error)
    // 2. The error message explains what failed validation
    // 3. The client can display this to the user or log it
    //
    // This approach distinguishes between:
    // - Protocol errors (invalid args) → JSON-RPC error response
    // - Tool errors (handler failed) → CallToolResult with isError=true
    // - Validation errors (output mismatch) → CallToolResult with isError=true
    //
    if (registration.output_validator) {
        try {
            registration.output_validator->validate(result);
        } catch (const std::exception& e) {
            // Output validation failed - return error response with isError flag
            // Use the helper function for consistent error formatting
            return make_output_validation_error(e.what());
        }
    }

    return result;
}

bool ToolRegistry::has_tool(const std::string& name) const {
    return tools_.find(name) != tools_.end();
}

void ToolRegistry::set_notify_callback(NotifyCallback cb) {
    notify_cb_ = std::move(cb);
}

void ToolRegistry::notify_changed() {
    if (notify_cb_) {
        notify_cb_();
    }
}

} // namespace server
} // namespace mcpp
