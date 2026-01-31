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

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <string>
#include <nlohmann/json.hpp>

using namespace mcpp;
using namespace mcpp::server;

using json = nlohmann::json;

// ============================================================================
// Tool Handlers
// ============================================================================

/**
 * @brief Simple calculator tool
 */
json handle_calculate(const json& params) {
    try {
        std::string operation = params.value("operation", "add");
        double a = params.value("a", 0.0);
        double b = params.value("b", 0.0);

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
json handle_echo(const json& params) {
    std::string text = params.value("text", "");
    return json{
        {"content", json::array({
            {{"type", "text"}, {"text", "Echo: " + text}}
        })}
    };
}

/**
 * @brief Get current time
 */
json handle_get_time(const json& params) {
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
json handle_read_file(const std::string& uri_str) {
    // Extract file path from URI (file://path or just path)
    std::string path = uri_str;
    if (path.find("file://") == 0) {
        path = path.substr(7);
    }

    // For security, only allow reading from /tmp directory
    if (path.find("/tmp/") != 0 && path != "/tmp/mcpp_test.txt") {
        return json{
            {"contents", json::array()},
            {"error", "Access denied: only /tmp directory is allowed"}
        };
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return json{
            {"contents", json::array()},
            {"error", "File not found: " + path}
        };
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return json{
        {"contents", json::array({
            {
                {"uri", uri_str},
                {"mimeType", "text/plain"},
                {"text", buffer.str()}
            }
        })}
    };
}

/**
 * @brief Get server info as a resource
 */
json handle_server_info(const std::string& uri) {
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

    return json{
        {"contents", json::array({
            {
                {"uri", uri},
                {"mimeType", "application/json"},
                {"text", info.dump(2)}
            }
        })}
    };
}

// ============================================================================
// Prompt Handlers
// ============================================================================

/**
 * @brief Generate a greeting prompt
 */
json handle_greeting(const json& args) {
    std::string name = args.value("name", "World");
    std::string tone = args.value("tone", "friendly");

    std::string greeting;
    if (tone == "formal") {
        greeting = "Good day, " + name + ". How may I assist you today?";
    } else if (tone == "casual") {
        greeting = "Hey " + name + "! What's up?";
    } else {
        greeting = "Hello, " + name + "! Nice to meet you.";
    }

    return json{
        {"messages", json::array({
            {
                {"role", "user"},
                {"content", json::array({
                    {{"type", "text"}, {"text", greeting}}
                })}
            }
        })},
        {"description", "A " + tone + " greeting for " + name}
    };
}

/**
 * @brief Generate a code review prompt template
 */
json handle_code_review(const json& args) {
    std::string language = args.value("language", "C++");
    std::string focus = args.value("focus", "general");

    std::string prompt_text = "Please review the following " + language + " code.\n";
    prompt_text += "Focus on: " + focus + "\n\n";
    prompt_text += "[Code will be provided here]";

    return json{
        {"messages", json::array({
            {
                {"role", "user"},
                {"content", json::array({
                    {{"type", "text"}, {"text", prompt_text}}
                })}
            }
        })},
        {"description", "Code review prompt for " + language + " with focus on " + focus}
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

    // Create MCP server
    McpServer server("mcpp Inspector Server", "0.1.0");

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
    json greeting_args = json::parse(R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "Name to greet"
            },
            "tone": {
                "type": "string",
                "enum": ["friendly", "formal", "casual"],
                "description": "Tone of the greeting"
            }
        }
    })");

    server.register_prompt(
        "greeting",
        "Generate a personalized greeting",
        {{"name", "name"}, {"description", "Name to greet"}, {"required", false}},
        handle_greeting
    );

    server.register_prompt(
        "greeting",
        "Generate a personalized greeting",
        {{"name", "tone"}, {"description", "Tone of greeting"}, {"required", false}},
        handle_greeting
    );

    json review_args = json::parse(R"({
        "type": "object",
        "properties": {
            "language": {
                "type": "string",
                "description": "Programming language",
                "default": "C++"
            },
            "focus": {
                "type": "string",
                "description": "Review focus area",
                "default": "general"
            }
        }
    })");

    server.register_prompt(
        "code_review",
        "Generate a code review prompt template",
        {{"name", "language"}, {"description", "Programming language"}, {"required", false}},
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
            json error = {
                {"jsonrpc", "2.0"},
                {"error", {
                    {"code", -32700},
                    {"message", "Parse error"},
                    {"data", e.what()}
                }},
                {"id", nullptr}
            };
            std::cout << error.dump() << std::endl;
        }
    }

    std::cerr << "Server shutting down..." << std::endl;
    return 0;
}
