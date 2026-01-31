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

#ifndef MCPP_SERVER_TOOL_REGISTRY_H
#define MCPP_SERVER_TOOL_REGISTRY_H

#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// JSON Schema validator - using nlohmann/json-schema-validator
// Note: The include path assumes the library is available.
// Users must link against nlohmann_json_schema_validator.
#if __has_include(<nlohmann/json-schema.hpp>)
    #include <nlohmann/json-schema.hpp>
    #define MCPP_HAS_JSON_SCHEMA 1
#else
    #define MCPP_HAS_JSON_SCHEMA 0
    // Placeholder validator type when json-schema-validator is not available
    namespace nlohmann {
    namespace json_schema {
    struct json_validator {
        void set_root_schema(const nlohmann::json&) {}
        void validate(const nlohmann::json&) {}
    };
    }
    }
#endif

#include "mcpp/content/pagination.h"
#include "mcpp/server/request_context.h"

namespace mcpp {
namespace server {

/**
 * @brief Tool handler function type
 *
 * Tool handlers receive:
 * - name: The tool's name (useful for shared handlers)
 * - args: Validated arguments according to the tool's input_schema
 * - ctx: Request context for progress reporting (plan 02-04)
 *
 * Handlers return nlohmann::json in the MCP CallToolResult format:
 * ```json
 * {
 *   "content": [
 *     { "type": "text", "text": "result" }
 *   ],
 *   "isError": false
 * }
 * ```
 *
 * For errors, handlers return:
 * ```json
 * {
 *   "content": [
 *     { "type": "text", "text": "error message" }
 *   ],
 *   "isError": true
 * }
 * ```
 *
 * @note The isError flag distinguishes tool errors from protocol errors.
 *       JSON-RPC errors are only for protocol-level issues.
 */
using ToolHandler = std::function<nlohmann::json(
    const std::string& name,
    const nlohmann::json& args,
    RequestContext& ctx
)>;

/**
 * @brief Callback type for list_changed notifications
 *
 * Invoked when the registry contents change (tools added/removed).
 */
using NotifyCallback = std::function<void()>;

/**
 * @brief Tool annotations for rich tool discovery
 *
 * Annotations provide metadata about tool behavior for UI/UX purposes:
 * - Audience filtering (user/assistant/system)
 * - Priority sorting (lower = higher priority)
 * - Destructive operation warnings
 * - Read-only indication
 */
struct ToolAnnotations {
    bool destructive = false;           ///< Indicates destructive operation
    bool read_only = true;              ///< Indicates read-only operation
    std::string audience = "user";      ///< "user" | "assistant" | "system"
    int priority = 0;                   ///< Lower = higher priority

    /**
     * @brief Default constructor with sensible defaults
     */
    ToolAnnotations() = default;

    /**
     * @brief Constructor with explicit values
     */
    ToolAnnotations(bool destr, bool ro, const std::string& aud, int prio)
        : destructive(destr), read_only(ro), audience(aud), priority(prio) {}
};

/**
 * @brief Registration data for a single tool
 *
 * Stores all metadata needed for tool discovery and execution.
 * The validator is compiled during registration for efficient
 * argument validation on every call.
 */
struct ToolRegistration {
    std::string name;                      ///< Tool identifier (unique in registry)
    std::string description;               ///< Human-readable description
    nlohmann::json input_schema;           ///< JSON Schema for argument validation
    std::unique_ptr<nlohmann::json_schema::json_validator> validator;  ///< Compiled validator for input
    std::optional<nlohmann::json> output_schema;  ///< JSON Schema for output validation
    std::unique_ptr<nlohmann::json_schema::json_validator> output_validator;  ///< Compiled validator for output
    ToolAnnotations annotations;          ///< Tool metadata for discovery
    ToolHandler handler;                   ///< Function to call when tool is invoked
};

/**
 * @brief Registry for MCP tools with discovery and execution
 *
 * The ToolRegistry manages tool registration, discovery (tools/list),
 * and execution (tools/call) with JSON Schema validation.
 *
 * Typical usage:
 * ```cpp
 * ToolRegistry registry;
 *
 * // Register a tool
 * registry.register_tool("echo", "Echo back the input",
 *     R"({"type": "object", "properties": {"message": {"type": "string"}}})"_json,
 *     [](const std::string& name, const json& args, RequestContext& ctx) {
 *         return json{
 *             {"content", {{{"type", "text"}, {"text", args["message"]}}}},
 *             {"isError", false}
 *         };
 *     }
 * );
 *
 * // List tools for discovery
 * auto tools = registry.list_tools();
 *
 * // Call a tool
 * RequestContext ctx("request-id", transport);
 * auto result = registry.call_tool("echo", {{"message", "hello"}}, ctx);
 * ```
 *
 * @note Thread-safety: Callers must synchronize access if using from multiple threads
 */
class ToolRegistry {
public:
    /**
     * @brief Default constructor
     */
    ToolRegistry() = default;

    /**
     * @brief Default destructor
     */
    ~ToolRegistry() = default;

    // Non-copyable
    ToolRegistry(const ToolRegistry&) = delete;
    ToolRegistry& operator=(const ToolRegistry&) = delete;

    // Movable
    ToolRegistry(ToolRegistry&&) noexcept = default;
    ToolRegistry& operator=(ToolRegistry&&) noexcept = default;

    /**
     * @brief Register a tool with its schema and handler
     *
     * Registers a tool for discovery and execution. The input_schema is used
     * to validate arguments before calling the handler.
     *
     * @param name Unique tool identifier
     * @param description Human-readable description of what the tool does
     * @param input_schema JSON Schema for argument validation (Draft 7)
     * @param handler Function to invoke when the tool is called
     * @return true if registration succeeded, false if a tool with this name exists
     *
     * @note The schema is compiled once during registration for efficient validation
     */
    bool register_tool(
        const std::string& name,
        const std::string& description,
        const nlohmann::json& input_schema,
        ToolHandler handler
    );

    /**
     * @brief Register a tool with annotations and output schema
     *
     * Extended registration with tool annotations (audience, priority, destructive/read-only)
     * and optional output schema for result validation.
     *
     * @param name Unique tool identifier
     * @param description Human-readable description of what the tool does
     * @param input_schema JSON Schema for argument validation (Draft 7)
     * @param output_schema Optional JSON Schema for output validation
     * @param annotations Tool metadata (audience, priority, destructive, read_only)
     * @param handler Function to invoke when the tool is called
     * @return true if registration succeeded, false if a tool with this name exists
     *
     * @note Output schema validation ensures tool results match declared structure
     */
    bool register_tool(
        const std::string& name,
        const std::string& description,
        const nlohmann::json& input_schema,
        const std::optional<nlohmann::json>& output_schema,
        const ToolAnnotations& annotations,
        ToolHandler handler
    );

    /**
     * @brief List all registered tools for discovery
     *
     * Returns an array of tool metadata in the format required by the
     * MCP tools/list method. Each tool object contains:
     * - name: Tool identifier
     * - description: Human-readable description
     * - inputSchema: JSON Schema for arguments
     * - annotations: Tool metadata (destructive, readOnly, audience, priority)
     * - outputSchema: JSON Schema for output (if declared)
     *
     * @return Array of tool objects
     */
    std::vector<nlohmann::json> list_tools() const;

    /**
     * @brief List all registered tools with pagination
     *
     * Returns paginated results with cursor-based navigation.
     * The cursor is an opaque token; clients should not interpret its contents.
     *
     * @param cursor Optional cursor from previous page for continuation
     * @return PaginatedResult containing tool metadata and optional next cursor
     */
    content::PaginatedResult<nlohmann::json> list_tools_paginated(
        const std::optional<std::string>& cursor = std::nullopt
    ) const;

    /**
     * @brief Call a tool by name with validated arguments
     *
     * Validates the arguments against the tool's schema, then calls the handler.
     * If validation fails, returns a JSON-RPC INVALID_PARAMS error.
     * If the tool is not found, returns std::nullopt.
     *
     * @param name Tool identifier to call
     * @param args Arguments to pass to the tool handler
     * @param ctx Request context for progress reporting
     * @return Tool result (CallToolResult), or std::nullopt if tool not found
     */
    std::optional<nlohmann::json> call_tool(
        const std::string& name,
        const nlohmann::json& args,
        RequestContext& ctx
    ) const;

    /**
     * @brief Check if a tool is registered
     *
     * @param name Tool identifier to check
     * @return true if the tool exists, false otherwise
     */
    bool has_tool(const std::string& name) const;

    /**
     * @brief Get the number of registered tools
     *
     * @return Number of tools in the registry
     */
    size_t size() const noexcept { return tools_.size(); }

    /**
     * @brief Check if the registry is empty
     *
     * @return true if no tools are registered, false otherwise
     */
    bool empty() const noexcept { return tools_.empty(); }

    /**
     * @brief Remove all registered tools
     */
    void clear() noexcept { tools_.clear(); }

    /**
     * @brief Set the callback for sending list_changed notifications
     *
     * The callback will be invoked when notify_changed() is called.
     * Typically used to send notifications/tools/list_changed to clients.
     *
     * @param cb Callback function (empty function to clear)
     */
    void set_notify_callback(NotifyCallback cb);

    /**
     * @brief Send tools/list_changed notification
     *
     * If a notify callback is registered, invokes it to send the notification.
     * Call this after registering/unregistering tools to notify clients.
     */
    void notify_changed();

private:
    /// Map of tool name to registration data (O(1) lookup)
    std::unordered_map<std::string, ToolRegistration> tools_;

    /// Callback for sending list_changed notifications
    NotifyCallback notify_cb_;
};

} // namespace server
} // namespace mcpp

#endif // MCPP_SERVER_TOOL_REGISTRY_H
