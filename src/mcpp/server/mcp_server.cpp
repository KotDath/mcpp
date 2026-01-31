// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/mcp_server.h"

#include "mcpp/transport/transport.h"

namespace mcpp {
namespace server {

namespace {

// JSON-RPC error codes
constexpr int JSONRPC_PARSE_ERROR = -32700;
constexpr int JSONRPC_INVALID_REQUEST = -32600;
constexpr int JSONRPC_METHOD_NOT_FOUND = -32601;
constexpr int JSONRPC_INVALID_PARAMS = -32602;
constexpr int JSONRPC_INTERNAL_ERROR = -32603;

/**
 * @brief Create a JSON-RPC error response
 */
nlohmann::json make_error(int code, const std::string& message, const nlohmann::json& id) {
    return nlohmann::json{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", {
            {"code", code},
            {"message", message}
        }}
    };
}

/**
 * @brief Create a JSON-RPC error response with data
 */
nlohmann::json make_error_with_data(
    int code,
    const std::string& message,
    const nlohmann::json& data,
    const nlohmann::json& id
) {
    return nlohmann::json{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", {
            {"code", code},
            {"message", message},
            {"data", data}
        }}
    };
}

} // anonymous namespace

McpServer::McpServer(const std::string& name, const std::string& version)
    : server_info_{name, version},
      transport_(std::nullopt) {}

void McpServer::set_transport(transport::Transport& transport) {
    transport_ = &transport;
}

bool McpServer::register_tool(
    const std::string& name,
    const std::string& description,
    const nlohmann::json& input_schema,
    ToolHandler handler
) {
    return tools_.register_tool(name, description, input_schema, std::move(handler));
}

bool McpServer::register_resource(
    const std::string& uri,
    const std::string& name,
    const std::optional<std::string>& description,
    const std::string& mime_type,
    ResourceHandler handler
) {
    return resources_.register_resource(uri, name, description, mime_type, std::move(handler));
}

bool McpServer::register_prompt(
    const std::string& name,
    const std::optional<std::string>& description,
    const std::vector<PromptArgument>& arguments,
    PromptHandler handler
) {
    return prompts_.register_prompt(name, description, arguments, std::move(handler));
}

std::optional<nlohmann::json> McpServer::handle_request(
    const nlohmann::json& request_json
) {
    // Extract method, params, and id
    if (!request_json.contains("method")) {
        return make_error(JSONRPC_INVALID_REQUEST, "Missing 'method' field", nullptr);
    }

    std::string method = request_json["method"].get<std::string>();
    nlohmann::json params = request_json.value("params", nlohmann::json::object());
    nlohmann::json id = request_json.value("id", nlohmann::json());

    // Route to appropriate handler
    nlohmann::json result;

    if (method == "initialize") {
        result = handle_initialize(params);
    } else if (method == "tools/list") {
        result = handle_tools_list();
    } else if (method == "tools/call") {
        result = handle_tools_call(params);
    } else if (method == "resources/list") {
        result = handle_resources_list();
    } else if (method == "resources/read") {
        result = handle_resources_read(params);
    } else if (method == "prompts/list") {
        result = handle_prompts_list();
    } else if (method == "prompts/get") {
        result = handle_prompts_get(params);
    } else {
        return make_error(JSONRPC_METHOD_NOT_FOUND, "Method not found", id);
    }

    // Build successful response
    return nlohmann::json{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    };
}

nlohmann::json McpServer::handle_initialize(const nlohmann::json& params) {
    // Build server capabilities
    nlohmann::json capabilities = {
        {"tools", nlohmann::json::object()},
        {"resources", {{"subscribe", false}}},
        {"prompts", nlohmann::json::object()}
    };

    // Return initialize result
    return nlohmann::json{
        {"protocolVersion", "2025-11-25"},
        {"serverInfo", {
            {"name", server_info_.name},
            {"version", server_info_.version}
        }},
        {"capabilities", capabilities}
    };
}

nlohmann::json McpServer::handle_tools_list() {
    nlohmann::json::array_t tools_array;
    std::vector<nlohmann::json> tools = tools_.list_tools();
    for (auto& tool : tools) {
        tools_array.push_back(std::move(tool));
    }
    return nlohmann::json{
        {"tools", tools_array}
    };
}

nlohmann::json McpServer::handle_tools_call(const nlohmann::json& params) {
    // Extract tool name
    if (!params.contains("name")) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Missing 'name' parameter"}
            }}
        };
    }

    std::string name = params["name"].get<std::string>();
    nlohmann::json arguments = params.value("arguments", nlohmann::json::object());

    // Extract progress token for this request
    std::optional<std::string> progress_token = extract_progress_token(params);

    // Create request context for tool handler
    // We need a request ID - extract from params or use a default
    std::string request_id = params.value("__request_id", "unknown");

    if (transport_) {
        RequestContext ctx(request_id, **transport_);
        if (progress_token) {
            ctx.set_progress_token(*progress_token);
        }

        // Call the tool
        std::optional<nlohmann::json> result = tools_.call_tool(name, arguments, ctx);
        if (result) {
            return std::move(*result);
        } else {
            return nlohmann::json{
                {"error", {
                    {"code", JSONRPC_INVALID_PARAMS},
                    {"message", "Tool not found: " + name}
                }}
            };
        }
    } else {
        // No transport set - create context without transport (progress won't work)
        // This is a fallback; ideally transport should be set
        RequestContext ctx(request_id, **transport_);
        if (progress_token) {
            ctx.set_progress_token(*progress_token);
        }

        std::optional<nlohmann::json> result = tools_.call_tool(name, arguments, ctx);
        if (result) {
            return std::move(*result);
        } else {
            return nlohmann::json{
                {"error", {
                    {"code", JSONRPC_INVALID_PARAMS},
                    {"message", "Tool not found: " + name}
                }}
            };
        }
    }
}

nlohmann::json McpServer::handle_resources_list() {
    std::vector<nlohmann::json> resources = resources_.list_resources();
    nlohmann::json::array_t resources_array;
    for (auto& resource : resources) {
        resources_array.push_back(std::move(resource));
    }
    return nlohmann::json{
        {"resources", resources_array}
    };
}

nlohmann::json McpServer::handle_resources_read(const nlohmann::json& params) {
    if (!params.contains("uri")) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Missing 'uri' parameter"}
            }}
        };
    }

    std::string uri = params["uri"].get<std::string>();
    std::optional<nlohmann::json> result = resources_.read_resource(uri);

    if (result) {
        return std::move(*result);
    } else {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Resource not found: " + uri}
            }}
        };
    }
}

nlohmann::json McpServer::handle_prompts_list() {
    std::vector<nlohmann::json> prompts = prompts_.list_prompts();
    nlohmann::json::array_t prompts_array;
    for (auto& prompt : prompts) {
        prompts_array.push_back(std::move(prompt));
    }
    return nlohmann::json{
        {"prompts", prompts_array}
    };
}

nlohmann::json McpServer::handle_prompts_get(const nlohmann::json& params) {
    if (!params.contains("name")) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Missing 'name' parameter"}
            }}
        };
    }

    std::string name = params["name"].get<std::string>();
    nlohmann::json arguments = params.value("arguments", nlohmann::json::object());

    std::optional<nlohmann::json> result = prompts_.get_prompt(name, arguments);

    if (result) {
        return std::move(*result);
    } else {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Prompt not found: " + name}
            }}
        };
    }
}

std::optional<std::string> McpServer::extract_progress_token(const nlohmann::json& params) {
    if (params.contains("_meta") && params["_meta"].is_object()) {
        const auto& meta = params["_meta"];
        if (meta.contains("progressToken")) {
            return meta["progressToken"].get<std::string>();
        }
    }
    return std::nullopt;
}

} // namespace server
} // namespace mcpp
