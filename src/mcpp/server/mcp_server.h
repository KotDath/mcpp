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

#ifndef MCPP_SERVER_MCP_SERVER_H
#define MCPP_SERVER_MCP_SERVER_H

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "mcpp/protocol/types.h"
#include "mcpp/server/prompt_registry.h"
#include "mcpp/server/resource_registry.h"
#include "mcpp/server/tool_registry.h"

namespace mcpp {

namespace transport {
class Transport;
}

namespace server {

/**
 * @brief Main MCP server class integrating all registries
 *
 * McpServer is the central class that ties together all server components
 * (tools, resources, prompts). It handles the MCP initialize handshake,
 * dispatches JSON-RPC requests to appropriate registries, and returns
 * properly formatted responses.
 *
 * Key features:
 * - Tool registration and execution via tools/list, tools/call
 * - Resource registration and reading via resources/list, resources/read
 * - Prompt registration and retrieval via prompts/list, prompts/get
 * - MCP initialize handshake with ServerCapabilities
 * - Progress token extraction and propagation to handlers
 *
 * Usage example:
 * ```cpp
 * mcpp::server::McpServer server("my-server", "1.0.0");
 *
 * // Register a tool
 * server.register_tool("echo", "Echo back the input",
 *     R"({"type": "object", "properties": {"message": {"type": "string"}}})"_json,
 *     [](const std::string& name, const json& args, RequestContext& ctx) {
 *         return json{
 *             {"content", {{{"type", "text"}, {"text", args["message"]}}}},
 *             {"isError", false}
 *         };
 *     }
 * );
 *
 * // Register a resource
 * server.register_resource(
 *     "file:///config.json",
 *     "Config",
 *     "Application configuration",
 *     "application/json",
 *     [](const std::string& uri) -> ResourceContent {
 *         return ResourceContent{uri, "application/json", true, read_config(), ""};
 *     }
 * );
 *
 * // Register a prompt
 * server.register_prompt(
 *     "greet",
 *     "Greeting prompt",
 *     {{"name", "target"}, {}, false},
 *     [](const std::string& name, const json& args) {
 *         std::string target = args.value("target", "world");
 *         return std::vector<PromptMessage>{
 *             {"user", {{"type", "text"}, {"text", "Hello, " + target + "!"}}}
 *         };
 *     }
 * );
 *
 * // Set transport and handle requests
 * server.set_transport(transport);
 * auto response = server.handle_request(request_json);
 * ```
 */
class McpServer {
public:
    /**
     * @brief Construct an MCP server
     *
     * @param name Server name (reported in initialize response)
     * @param version Server version (reported in initialize response)
     */
    explicit McpServer(
        const std::string& name = "mcpp-server",
        const std::string& version = "1.0.0"
    );

    /**
     * @brief Destructor
     */
    ~McpServer() = default;

    // Non-copyable
    McpServer(const McpServer&) = delete;
    McpServer& operator=(const McpServer&) = delete;

    // Movable
    McpServer(McpServer&&) noexcept = default;
    McpServer& operator=(McpServer&&) noexcept = default;

    /**
     * @brief Set the transport for sending notifications
     *
     * The transport is used by RequestContext to send progress notifications.
     * This must be set before tools that use progress reporting can work.
     *
     * @param transport Reference to the transport (non-owning)
     */
    void set_transport(transport::Transport& transport);

    /**
     * @brief Register a tool with the server
     *
     * @param name Unique tool identifier
     * @param description Human-readable description
     * @param input_schema JSON Schema for argument validation
     * @param handler Function to call when the tool is invoked
     * @return true if registration succeeded, false if tool already exists
     */
    bool register_tool(
        const std::string& name,
        const std::string& description,
        const nlohmann::json& input_schema,
        ToolHandler handler
    );

    /**
     * @brief Register a resource with the server
     *
     * @param uri Unique URI for this resource
     * @param name Human-readable name
     * @param description Optional description
     * @param mime_type MIME type of the resource content
     * @param handler Function to generate resource content
     * @return true if registration succeeded, false if URI already exists
     */
    bool register_resource(
        const std::string& uri,
        const std::string& name,
        const std::optional<std::string>& description,
        const std::string& mime_type,
        ResourceHandler handler
    );

    /**
     * @brief Register a prompt with the server
     *
     * @param name Unique prompt name
     * @param description Optional description
     * @param arguments List of argument definitions
     * @param handler Function to generate prompt messages
     * @return true if registration succeeded, false if name already exists
     */
    bool register_prompt(
        const std::string& name,
        const std::optional<std::string>& description,
        const std::vector<PromptArgument>& arguments,
        PromptHandler handler
    );

    /**
     * @brief Handle a JSON-RPC request
     *
     * Parses the request JSON, routes to the appropriate handler,
     * and returns a JSON-RPC response (result or error).
     *
     * Supported methods:
     * - initialize: MCP handshake
     * - tools/list: List all registered tools
     * - tools/call: Execute a tool
     * - resources/list: List all registered resources
     * - resources/read: Read a resource
     * - resources/complete: Get completion suggestions for resources
     * - prompts/list: List all registered prompts
     * - prompts/get: Get a prompt with arguments
     * - prompts/complete: Get completion suggestions for prompts
     *
     * @param request_json The JSON-RPC request object
     * @return Optional JSON-RPC response (nullopt for notifications)
     */
    std::optional<nlohmann::json> handle_request(
        const nlohmann::json& request_json
    );

private:
    /**
     * @brief Handle the initialize request
     *
     * Returns the server's protocol version, info, and capabilities.
     *
     * @param params Initialize request parameters (client info, capabilities)
     * @return Initialize response with protocolVersion, serverInfo, capabilities
     */
    nlohmann::json handle_initialize(const nlohmann::json& params);

    /**
     * @brief Handle tools/list request
     *
     * @return Array of tool definitions
     */
    nlohmann::json handle_tools_list();

    /**
     * @brief Handle tools/call request
     *
     * @param params Request parameters containing name and arguments
     * @return Tool execution result
     */
    nlohmann::json handle_tools_call(const nlohmann::json& params);

    /**
     * @brief Handle resources/list request
     *
     * @return Array of resource definitions
     */
    nlohmann::json handle_resources_list();

    /**
     * @brief Handle resources/read request
     *
     * @param params Request parameters containing uri
     * @return Resource content
     */
    nlohmann::json handle_resources_read(const nlohmann::json& params);

    /**
     * @brief Handle prompts/list request
     *
     * @return Array of prompt definitions
     */
    nlohmann::json handle_prompts_list();

    /**
     * @brief Handle prompts/get request
     *
     * @param params Request parameters containing name and optional arguments
     * @return Prompt with messages
     */
    nlohmann::json handle_prompts_get(const nlohmann::json& params);

    /**
     * @brief Handle prompts/complete request
     *
     * Returns completion suggestions for prompt arguments.
     *
     * @param params Request parameters containing name, argument, value, and optional reference
     * @return Completion result with suggestions
     */
    nlohmann::json handle_prompts_complete(const nlohmann::json& params);

    /**
     * @brief Handle resources/complete request
     *
     * Returns completion suggestions for resource URIs or values.
     *
     * @param params Request parameters containing name, argument, value, and optional reference
     * @return Completion result with suggestions
     */
    nlohmann::json handle_resources_complete(const nlohmann::json& params);

    /**
     * @brief Extract progress token from request parameters
     *
     * Looks for params._meta.progressToken and returns it if present.
     *
     * @param params Request parameters
     * @return Optional progress token string
     */
    std::optional<std::string> extract_progress_token(const nlohmann::json& params);

    /// Server implementation info (name, version)
    protocol::Implementation server_info_;

    /// Optional pointer to transport (non-owning) for notifications
    std::optional<transport::Transport*> transport_;

    /// Tool registry
    ToolRegistry tools_;

    /// Resource registry
    ResourceRegistry resources_;

    /// Prompt registry
    PromptRegistry prompts_;
};

} // namespace server
} // namespace mcpp

#endif // MCPP_SERVER_MCP_SERVER_H
