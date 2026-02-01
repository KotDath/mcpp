// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

/**
 * @file inspector_server.cpp
 * @brief Example MCP server for testing with MCP Inspector
 *
 * This server demonstrates the complete mcpp API:
 * - Tool registration and execution
 * - Resource registration and reading
 * - Prompt registration and retrieval
 * - Stdio transport for communication
 *
 * Usage:
 *   mcp-inspector connect stdio ./build/examples/inspector_server
 *
 * Or directly:
 *   ./build/examples/inspector_server
 */

#include "mcpp/server/mcp_server.h"
#include "mcpp/transport/null_transport.h"
#include "mcpp/util/logger.h"
#include "mcpp/core/json_rpc.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <string>

using namespace mcpp;
using namespace mcpp::server;

using json = nlohmann::json;

// ============================================================================
// Tool Handlers
// ============================================================================

/**
 * @brief Simple calculator tool
 */
json handle_calculate(const std::string& name, const json& args, RequestContext& ctx) {
    try {
        std::string operation = args.value("operation", "add");
        double a = args.value("a", 0.0);
        double b = args.value("b", 0.0);

        double result = 0.0;
        if (operation == "add") {
            result = a + b;
        } else if (operation == "subtract") {
            result = a - b;
        } else if (operation == "multiply") {
            result = a * b;
        } else if (operation == "divide") {
            if (b == 0.0) {
                return json{
                    {"content", json::array({
                        {{"type", "text"}, {"text", "Error: Division by zero"}}
                    })},
                    {"isError", true}
                };
            }
            result = a / b;
        } else {
            return json{
                {"content", json::array({
                    {{"type", "text"}, {"text", "Unknown operation: " + operation}}
                })},
                {"isError", true}
            };
        }

        std::ostringstream oss;
        oss << result;
        return json{
            {"content", json::array({
                {{"type", "text"}, {"text", oss.str()}}
            })}
        };
    } catch (const std::exception& e) {
        return json{
            {"content", json::array({
                {{"type", "text"}, {"text", std::string("Error: ") + e.what()}}
            })},
            {"isError", true}
        };
    }
}

/**
 * @brief Echo tool - returns the input text
 */
json handle_echo(const std::string& name, const json& args, RequestContext& ctx) {
    std::string text = args.value("text", "");
    return json{
        {"content", json::array({
            {{"type", "text"}, {"text", "Echo: " + text}}
        })}
    };
}

/**
 * @brief Get current time
 */
json handle_get_time(const std::string& name, const json& args, RequestContext& ctx) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::string time_str = std::ctime(&time);
    // Remove trailing newline
    if (!time_str.empty() && time_str.back() == '\n') {
        time_str.pop_back();
    }

    return json{
        {"content", json::array({
            {{"type", "text"}, {"text", "Current time: " + time_str}}
        })}
    };
}

// ============================================================================
// Resource Handlers
// ============================================================================

/**
 * @brief Read a text file resource
 */
ResourceContent handle_read_file(const std::string& uri) {
    // Extract file path from URI (file://path or just path)
    std::string path = uri;
    if (path.find("file://") == 0) {
        path = path.substr(7);
    }

    // For security, only allow reading from /tmp directory
    if (path.find("/tmp/") != 0 && path != "/tmp/mcpp_test.txt") {
        return ResourceContent{
            .uri = uri,
            .mime_type = "text/plain",
            .is_text = true,
            .text = "",
            .blob = ""
        };
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return ResourceContent{
            .uri = uri,
            .mime_type = "text/plain",
            .is_text = true,
            .text = "",
            .blob = ""
        };
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return ResourceContent{
        .uri = uri,
        .mime_type = "text/plain",
        .is_text = true,
        .text = buffer.str(),
        .blob = ""
    };
}

/**
 * @brief Get server info as a resource
 */
ResourceContent handle_server_info(const std::string& uri) {
    json info = {
        {"name", "mcpp Inspector Server"},
        {"version", "0.1.0"},
        {"description", "Example MCP server for testing with Inspector"},
        {"capabilities", {
            {"tools", true},
            {"resources", true},
            {"prompts", true}
        }}
    };

    return ResourceContent{
        .uri = uri,
        .mime_type = "application/json",
        .is_text = true,
        .text = info.dump(2),
        .blob = ""
    };
}

// ============================================================================
// Prompt Handlers
// ============================================================================

/**
 * @brief Generate a greeting prompt
 */
std::vector<PromptMessage> handle_greeting(const std::string& name, const json& args) {
    std::string target_name = args.value("name", "World");

    return std::vector<PromptMessage>{
        PromptMessage{
            .role = "user",
            .content = json::array({
                {{"type", "text"}, {"text", "Hello, " + target_name + "!"}}
            })
        }
    };
}

/**
 * @brief Generate a code review prompt template
 */
std::vector<PromptMessage> handle_code_review(const std::string& name, const json& args) {
    std::string language = args.value("language", "C++");
    std::string focus = args.value("focus", "general");

    std::string prompt_text = "Please review the following " + language + " code.\n";
    prompt_text += "Focus on: " + focus + "\n\n";
    prompt_text += "[Code will be provided here]";

    return std::vector<PromptMessage>{
        PromptMessage{
            .role = "user",
            .content = json::array({
                {{"type", "text"}, {"text", prompt_text}}
            })
        }
    };
}

// ============================================================================
// Main Server Setup
// ============================================================================

int main(int argc, char* argv[]) {
    std::cerr << "=== mcpp Inspector Server ===" << std::endl;
    std::cerr << "This server communicates via stdio for MCP Inspector." << std::endl;
    std::cerr << "Connect with: mcp-inspector connect stdio ./build/examples/inspector_server" << std::endl;
    std::cerr << std::endl;

    // Create null transport for progress notifications (no-op for stdio server)
    auto null_transport = std::make_unique<transport::NullTransport>();

    // Create MCP server
    McpServer server("mcpp Inspector Server", "0.1.0");
    server.set_transport(*null_transport);

    // Register tools
    json calculate_schema = json::parse(R"({
        "type": "object",
        "properties": {
            "operation": {
                "type": "string",
                "enum": ["add", "subtract", "multiply", "divide"],
                "description": "The operation to perform"
            },
            "a": {"type": "number", "description": "First operand"},
            "b": {"type": "number", "description": "Second operand"}
        },
        "required": ["operation", "a", "b"]
    })");

    server.register_tool(
        "calculate",
        "Perform basic arithmetic operations",
        calculate_schema,
        handle_calculate
    );

    json echo_schema = json::parse(R"({
        "type": "object",
        "properties": {
            "text": {"type": "string", "description": "Text to echo back"}
        },
        "required": ["text"]
    })");

    server.register_tool(
        "echo",
        "Echo the input text back to the caller",
        echo_schema,
        handle_echo
    );

    server.register_tool(
        "get_time",
        "Get the current server time",
        json::parse(R"({"type": "object"})"),
        handle_get_time
    );

    // Register resources
    server.register_resource(
        "file://tmp/mcpp_test.txt",
        "Test File",
        "A test file in /tmp directory",
        "text/plain",
        handle_read_file
    );

    server.register_resource(
        "info://server",
        "Server Info",
        "Server information and capabilities",
        "application/json",
        handle_server_info
    );

    // Register prompts
    PromptArgument name_arg;
    name_arg.name = "name";
    name_arg.description = "Name to greet";
    name_arg.required = false;

    PromptArgument lang_arg;
    lang_arg.name = "language";
    lang_arg.description = "Programming language";
    lang_arg.required = false;

    PromptArgument focus_arg;
    focus_arg.name = "focus";
    focus_arg.description = "Review focus area";
    focus_arg.required = false;

    std::vector<PromptArgument> greeting_args = {name_arg};
    std::vector<PromptArgument> review_args = {lang_arg, focus_arg};

    server.register_prompt(
        "code_review",
        "Generate a code review prompt template",
        review_args,
        handle_code_review
    );

    std::cerr << "Registered:" << std::endl;
    std::cerr << "  - 3 tools (calculate, echo, get_time)" << std::endl;
    std::cerr << "  - 2 resources (file://tmp/mcpp_test.txt, info://server)" << std::endl;
    std::cerr << "  - 2 prompts (greeting, code_review)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Starting server loop..." << std::endl;

    // Main event loop: read JSON-RPC from stdin, process, write response to stdout
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }

        try {
            json request = json::parse(line);
            std::optional<json> response = server.handle_request(request);

            if (response.has_value()) {
                std::cout << response->dump() << std::endl;
            }
            // Notifications have no response
        } catch (const json::exception& e) {
            // Try to extract the ID from the partially malformed request
            // This is a best-effort attempt to provide a valid ID for the error response
            json id = nullptr;

            // Try to find "id" field in the raw line using simple string search
            size_t id_pos = line.find("\"id\"");
            if (id_pos != std::string::npos) {
                size_t colon_pos = line.find(":", id_pos);
                if (colon_pos != std::string::npos) {
                    size_t value_start = line.find_first_not_of(" \t\n\r", colon_pos + 1);
                    if (value_start != std::string::npos) {
                        char next_char = line[value_start];
                        if (next_char == '"') {
                            // String ID - extract until closing quote
                            size_t value_end = line.find('"', value_start + 1);
                            if (value_end != std::string::npos) {
                                std::string id_str = line.substr(value_start + 1, value_end - value_start - 1);
                                id = id_str;
                            }
                        } else if (next_char >= '0' && next_char <= '9' || next_char == '-') {
                            // Numeric ID - try to extract
                            size_t value_end = line.find_first_not_of("0123456789-", value_start);
                            if (value_end != std::string::npos) {
                                std::string id_str = line.substr(value_start, value_end - value_start);
                                try {
                                    int id_val = std::stoi(id_str);
                                    id = id_val;
                                } catch (...) {
                                    // Fall through to use default
                                }
                            }
                        }
                    }
                }
            }

            json error = {
                {"jsonrpc", "2.0"},
                {"error", {
                    {"code", -32700},
                    {"message", "Parse error"},
                    {"data", e.what()}
                }},
                {"id", id}
            };
            std::cout << error.dump() << std::endl;
        }
    }

    std::cerr << "Server shutting down..." << std::endl;
    return 0;
}
