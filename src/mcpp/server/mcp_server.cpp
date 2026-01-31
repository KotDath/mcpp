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
    } else if (method == "prompts/complete") {
        result = handle_prompts_complete(params);
    } else if (method == "resources/complete") {
        result = handle_resources_complete(params);
    } else if (method == "tasks/send") {
        result = handle_tasks_send(params);
    } else if (method == "tasks/get") {
        result = handle_tasks_get(params);
    } else if (method == "tasks/cancel") {
        result = handle_tasks_cancel(params);
    } else if (method == "tasks/result") {
        result = handle_tasks_result(params);
    } else if (method == "tasks/list") {
        result = handle_tasks_list(params);
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
    // Extract and store client capabilities if provided
    if (params.contains("capabilities") && params["capabilities"].is_object()) {
        const nlohmann::json& caps_json = params["capabilities"];

        // Build ClientCapabilities from JSON
        protocol::ClientCapabilities client_caps;

        // Parse experimental capabilities (contains tools/resources/prompts with listChanged)
        if (caps_json.contains("experimental")) {
            client_caps.experimental = caps_json["experimental"];
        }

        // Store client capabilities
        client_capabilities_ = client_caps;

        // Set up registry callbacks now that we have client capabilities
        setup_registry_callbacks();
    }

    // Build server capabilities
    nlohmann::json capabilities = {
        {"tools", nlohmann::json::object()},
        {"resources", {{"subscribe", false}}},
        {"prompts", nlohmann::json::object()},
        {"tasks", nlohmann::json::object()}  // Experimental tasks capability
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

    if (!transport_) {
        // No transport set - cannot create RequestContext (requires Transport reference)
        // Return error response indicating transport must be set
        return nlohmann::json{
            {"jsonrpc", "2.0"},
            {"error", {
                {"code", JSONRPC_INTERNAL_ERROR},
                {"message", "Transport not set. Call set_transport() before handling requests."}
            }}
        };
    }

    // Create RequestContext with transport reference
    transport::Transport& transport = **transport_;
    RequestContext ctx(request_id, transport);

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

nlohmann::json McpServer::handle_prompts_complete(const nlohmann::json& params) {
    // Validate required parameters
    if (!params.contains("name")) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Missing 'name' parameter"}
            }}
        };
    }

    if (!params.contains("argument")) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Missing 'argument' parameter"}
            }}
        };
    }

    std::string name = params["name"].get<std::string>();
    std::string argument = params["argument"].get<std::string>();
    nlohmann::json value = params.value("value", nlohmann::json());

    // Extract optional reference
    std::optional<nlohmann::json> reference;
    if (params.contains("reference") && !params["reference"].is_null()) {
        reference = params["reference"];
    }

    // Get completion suggestions from the prompt registry
    std::optional<std::vector<Completion>> completions =
        prompts_.get_completion(name, argument, value, reference);

    // Build MCP CompleteResult format
    nlohmann::json::array_t completion_array;

    if (completions.has_value()) {
        for (const auto& completion : *completions) {
            nlohmann::json completion_item;
            completion_item["value"] = completion.value;

            if (completion.description.has_value()) {
                completion_item["description"] = *completion.description;
            }

            completion_array.push_back(std::move(completion_item));
        }
    }

    return nlohmann::json{
        {"completion", std::move(completion_array)}
    };
}

nlohmann::json McpServer::handle_resources_complete(const nlohmann::json& params) {
    // Validate required parameters
    if (!params.contains("name")) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Missing 'name' parameter"}
            }}
        };
    }

    if (!params.contains("argument")) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Missing 'argument' parameter"}
            }}
        };
    }

    std::string name = params["name"].get<std::string>();
    std::string argument = params["argument"].get<std::string>();
    nlohmann::json value = params.value("value", nlohmann::json());

    // Extract optional reference
    std::optional<nlohmann::json> reference;
    if (params.contains("reference") && !params["reference"].is_null()) {
        reference = params["reference"];
    }

    // Get completion suggestions from the resource registry
    std::optional<std::vector<Completion>> completions =
        resources_.get_completion(name, argument, value, reference);

    // Build MCP CompleteResult format
    nlohmann::json::array_t completion_array;

    if (completions.has_value()) {
        for (const auto& completion : *completions) {
            nlohmann::json completion_item;
            completion_item["value"] = completion.value;

            if (completion.description.has_value()) {
                completion_item["description"] = *completion.description;
            }

            completion_array.push_back(std::move(completion_item));
        }
    }

    return nlohmann::json{
        {"completion", std::move(completion_array)}
    };
}

void McpServer::setup_registry_callbacks() {
    // Set up tools list_changed callback
    tools_.set_notify_callback([this]() {
        if (client_capabilities_ &&
            client_capabilities_->experimental &&
            client_capabilities_->experimental.value().contains("tools")) {
            auto tools_cap = client_capabilities_->experimental.value()["tools"];
            if (tools_cap.contains("listChanged") && tools_cap["listChanged"].get<bool>()) {
                send_list_changed_notification("notifications/tools/list_changed");
            }
        }
    });

    // Set up resources list_changed callback
    resources_.set_notify_callback([this]() {
        if (client_capabilities_ &&
            client_capabilities_->experimental &&
            client_capabilities_->experimental.value().contains("resources")) {
            auto res_cap = client_capabilities_->experimental.value()["resources"];
            if (res_cap.contains("listChanged") && res_cap["listChanged"].get<bool>()) {
                send_list_changed_notification("notifications/resources/list_changed");
            }
        }
    });

    // Set up prompts list_changed callback
    prompts_.set_notify_callback([this]() {
        if (client_capabilities_ &&
            client_capabilities_->experimental &&
            client_capabilities_->experimental.value().contains("prompts")) {
            auto prompts_cap = client_capabilities_->experimental.value()["prompts"];
            if (prompts_cap.contains("listChanged") && prompts_cap["listChanged"].get<bool>()) {
                send_list_changed_notification("notifications/prompts/list_changed");
            }
        }
    });
}

void McpServer::send_list_changed_notification(const std::string& method) {
    if (!transport_.has_value()) {
        return;  // No transport - cannot send notification
    }

    nlohmann::json notification = {
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", nlohmann::json::object()}
    };

    (*transport_)->send(notification.dump());
}

nlohmann::json McpServer::handle_tasks_send(const nlohmann::json& params) {
    // Extract optional TTL and poll interval
    std::optional<uint64_t> ttl_ms;
    if (params.contains("ttl") && !params["ttl"].is_null()) {
        ttl_ms = params["ttl"].get<uint64_t>();
    }

    std::optional<uint64_t> poll_interval_ms;
    if (params.contains("pollIntervalMs") && !params["pollIntervalMs"].is_null()) {
        poll_interval_ms = params["pollIntervalMs"].get<uint64_t>();
    }

    // Create the task
    std::string task_id = task_manager_.create_task(ttl_ms, poll_interval_ms);

    // Get the created task metadata
    std::optional<Task> task = task_manager_.get_task(task_id);
    if (!task) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INTERNAL_ERROR},
                {"message", "Failed to create task"}
            }}
        };
    }

    // Return MCP CreateTaskResult format
    return nlohmann::json{
        {"id", task->task_id},
        {"status", to_string(task->status)},
        {"createdAt", task->created_at},
        {"lastUpdatedAt", task->last_updated_at},
        {"ttlMs", task->ttl_ms.has_value() ? nlohmann::json(*task->ttl_ms) : nlohmann::json()},
        {"pollIntervalMs", task->poll_interval_ms.has_value() ? nlohmann::json(*task->poll_interval_ms) : nlohmann::json()}
    };
}

nlohmann::json McpServer::handle_tasks_get(const nlohmann::json& params) {
    // Validate required parameters
    if (!params.contains("id")) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Missing 'id' parameter"}
            }}
        };
    }

    std::string task_id = params["id"].get<std::string>();

    // Get the task
    std::optional<Task> task = task_manager_.get_task(task_id);
    if (!task) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Task not found: " + task_id}
            }}
        };
    }

    // Build result object
    nlohmann::json result = {
        {"id", task->task_id},
        {"status", to_string(task->status)},
        {"createdAt", task->created_at},
        {"lastUpdatedAt", task->last_updated_at}
    };

    if (task->status_message.has_value()) {
        result["statusMessage"] = *task->status_message;
    }
    if (task->ttl_ms.has_value()) {
        result["ttlMs"] = *task->ttl_ms;
    }
    if (task->poll_interval_ms.has_value()) {
        result["pollIntervalMs"] = *task->poll_interval_ms;
    }

    return result;
}

nlohmann::json McpServer::handle_tasks_cancel(const nlohmann::json& params) {
    // Validate required parameters
    if (!params.contains("id")) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Missing 'id' parameter"}
            }}
        };
    }

    std::string task_id = params["id"].get<std::string>();

    // Cancel the task
    bool cancelled = task_manager_.cancel_task(task_id);
    if (!cancelled) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Task not found or already terminal: " + task_id}
            }}
        };
    }

    // Get the updated task
    std::optional<Task> task = task_manager_.get_task(task_id);

    // Build result object
    nlohmann::json result = {
        {"id", task_id},
        {"status", "cancelled"},
        {"lastUpdatedAt", task_manager_.get_timestamp()}
    };

    if (task && task->status_message.has_value()) {
        result["statusMessage"] = *task->status_message;
    }

    return result;
}

nlohmann::json McpServer::handle_tasks_result(const nlohmann::json& params) {
    // Validate required parameters
    if (!params.contains("id")) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Missing 'id' parameter"}
            }}
        };
    }

    std::string task_id = params["id"].get<std::string>();

    // Get the task result
    std::optional<nlohmann::json> result = task_manager_.get_result(task_id);
    if (!result) {
        return nlohmann::json{
            {"error", {
                {"code", JSONRPC_INVALID_PARAMS},
                {"message", "Task not found or no result available: " + task_id}
            }}
        };
    }

    return nlohmann::json{
        {"id", task_id},
        {"result", *result}
    };
}

nlohmann::json McpServer::handle_tasks_list(const nlohmann::json& params) {
    // Extract optional cursor
    std::optional<std::string> cursor;
    if (params.contains("cursor") && !params["cursor"].is_null()) {
        cursor = params["cursor"].get<std::string>();
    }

    // List tasks
    TaskManager::PaginatedResult<Task> page = task_manager_.list_tasks(cursor);

    // Build result items array
    nlohmann::json::array_t items_array;
    for (const auto& task : page.items) {
        nlohmann::json item = {
            {"id", task.task_id},
            {"status", to_string(task.status)},
            {"createdAt", task.created_at},
            {"lastUpdatedAt", task.last_updated_at}
        };

        if (task.status_message.has_value()) {
            item["statusMessage"] = *task.status_message;
        }
        if (task.ttl_ms.has_value()) {
            item["ttlMs"] = *task.ttl_ms;
        }
        if (task.poll_interval_ms.has_value()) {
            item["pollIntervalMs"] = *task.poll_interval_ms;
        }

        items_array.push_back(std::move(item));
    }

    // Build result
    nlohmann::json result = {{"items", items_array}};
    if (page.nextCursor.has_value()) {
        result["nextCursor"] = *page.nextCursor;
    }

    return result;
}

} // namespace server
} // namespace mcpp
