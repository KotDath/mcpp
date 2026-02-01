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

    // Debug logging for registered capabilities (uses MCPP_DEBUG_LOG from Phase 8)
    MCPP_DEBUG_LOG("Registered: 3 tools (calculate, echo, get_time)");
    MCPP_DEBUG_LOG("Registered: 2 resources (file://tmp/mcpp_test.txt, info://server)");
    MCPP_DEBUG_LOG("Registered: 2 prompts (code_review, greeting)");
    MCPP_DEBUG_LOG("Starting server loop...");

    // Main event loop: read JSON-RPC from stdin using MCP stdio transport protocol
    // MCP stdio format: "Content-Length: <bytes>\r\n\r\n<json>"
    // See: https://modelcontextprotocol.io/specification/2024-11-05/basic/transport/
    int64_t auto_id = 1;  // Auto-generated ID for requests without id
    while (true) {
        // Read Content-Length header
        std::string header_line;
        if (!std::getline(std::cin, header_line)) {
            break;  // EOF
        }

        // Skip empty lines (keep-alive)
        if (header_line.empty()) {
            continue;
        }

        // Check if this is a Content-Length header
        if (header_line.find("Content-Length:") != 0) {
            // Not using stdio transport protocol - fall back to line-delimited JSON
            // This handles direct testing with echo pipes
            MCPP_DEBUG_LOG("Non-stdio protocol, treating as line-delimited JSON");

            try {
                json raw = json::parse(header_line);

                if (!raw.contains("method")) {
                    continue;
                }

                std::string method = raw["method"].get<std::string>();
                bool is_notification = (method.find("notifications/") == 0);

                if (!raw.contains("jsonrpc")) {
                    raw["jsonrpc"] = "2.0";
                }
                if (!raw.contains("id") && !is_notification) {
                    raw["id"] = auto_id++;
                }

                if (auto parsed = mcpp::core::JsonRpcRequest::from_json(raw)) {
                    std::optional<json> response = server.handle_request(raw);
                    if (response.has_value()) {
                        std::cout << response->dump() << "\n" << std::flush;
                    }
                }
            } catch (...) {
                // Skip invalid JSON in fallback mode
            }
            continue;
        }

        // Parse Content-Length value
        size_t colon_pos = header_line.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }

        std::string length_str = header_line.substr(colon_pos + 1);
        // Trim whitespace
        size_t start = length_str.find_first_not_of(" \t\r\n");
        size_t end = length_str.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) {
            continue;
        }
        length_str = length_str.substr(start, end - start + 1);

        int content_length;
        try {
            content_length = std::stoi(length_str);
        } catch (...) {
            continue;
        }

        // Read the blank line after headers
        std::string blank;
        std::getline(std::cin, blank);

        // Read exactly content_length bytes for the JSON payload
        std::string json_payload;
        json_payload.resize(content_length);
        std::cin.read(&json_payload[0], content_length);

        // Also consume the trailing newline
        std::cin.get();

        MCPP_DEBUG_LOG("Received (%zu bytes): %s", json_payload.size(), json_payload.c_str());

        try {
            // Parse raw JSON
            json raw = json::parse(json_payload);

            // Check if this looks like a JSON-RPC message (has method field)
            if (!raw.contains("method")) {
                MCPP_DEBUG_LOG("Skipping message without method field");
                continue;
            }

            std::string method = raw["method"].get<std::string>();
            bool is_notification = (method.find("notifications/") == 0);

            // Add jsonrpc field if missing (shouldn't happen with proper MCP clients)
            if (!raw.contains("jsonrpc")) {
                raw["jsonrpc"] = "2.0";
            }

            // Add id for non-notification requests that lack one (Inspector compatibility)
            if (!raw.contains("id") && !is_notification) {
                raw["id"] = auto_id++;
            }

            // Use Phase 8 validation layer - validates full JSON-RPC 2.0 structure
            if (auto parsed = mcpp::core::JsonRpcRequest::from_json(raw)) {
                // Valid JSON-RPC request - process with server
                std::optional<json> response = server.handle_request(raw);
                if (response.has_value()) {
                    // Send response using MCP stdio format with Content-Length header
                    std::string response_str = response->dump();
                    std::cout << "Content-Length: " << response_str.size() << "\r\n\r\n" << response_str << std::flush;
                }
                // Notifications have no response
            } else {
                // Validation failed - extract ID for proper error response
                mcpp::core::RequestId id = mcpp::core::JsonRpcRequest::extract_request_id(json_payload);

                json error_response = {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", -32700},
                        {"message", "Parse error"}
                    }},
                    {"id", std::visit([](auto&& v) -> json {
                        using T = std::decay_t<decltype(v)>;
                        if constexpr (std::is_same_v<T, int64_t>) {
                            return v;
                        } else if constexpr (std::is_same_v<T, std::string>) {
                            return v;
                        }
                        return nullptr;
                    }, id)}
                };
                std::string error_str = error_response.dump();
                std::cout << "Content-Length: " << error_str.size() << "\r\n\r\n" << error_str << std::flush;
            }
        } catch (const json::exception& e) {
            MCPP_DEBUG_LOG("JSON parse error: {}", e.what());
            // Completely malformed JSON - still try to extract ID for error response
            mcpp::core::RequestId id = mcpp::core::JsonRpcRequest::extract_request_id(json_payload);

            json error_response = {
                {"jsonrpc", "2.0"},
                {"error", {
                    {"code", -32700},
                    {"message", "Parse error"}
                }},
                {"id", std::visit([](auto&& v) -> json {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, int64_t>) {
                        return v;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        return v;
                    }
                    return nullptr;
                }, id)}
            };
            std::string error_str = error_response.dump();
            std::cout << "Content-Length: " << error_str.size() << "\r\n\r\n" << error_str << std::flush;
        }
    }

    std::cerr << "Server shutting down..." << std::endl;
    return 0;
}
