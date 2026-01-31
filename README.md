# mcpp

C++ SDK for [MCP (Model Context Protocol)](https://modelcontextprotocol.io). Build MCP clients and servers in C++ with full protocol support.

## Features

- **Full MCP Protocol Support**: Implements MCP protocol version 2025-11-25
- **Tools**: Register and call tools with JSON Schema validation
- **Resources**: Expose data resources with URI-based discovery
- **Prompts**: Create reusable prompt templates
- **Transports**: Stdio transport for subprocess communication
- **Type-Safe C++ API**: Modern C++20 with clean, idiomatic interface
- **Header-Only Components**: Many utilities are header-only for easy integration

## Building

### Prerequisites

- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.16+
- [nlohmann/json](https://github.com/nlohmann/json) (included as header-only)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/mcpp-project/mcpp.git
cd mcpp

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure
```

### Build Options

- `BUILD_SHARED_LIBS=ON`: Build shared libraries (default: ON)
- `MCPP_BUILD_TESTS=ON`: Build tests (default: ON)
- `MCPP_BUILD_EXAMPLES=ON`: Build examples (default: ON)

## Quick Start

### Creating an MCP Server

```cpp
#include "mcpp/server/mcp_server.h"
#include <nlohmann/json.hpp>

using namespace mcpp;
using namespace mcpp::server;

int main() {
    // Create MCP server
    McpServer server("my-server", "1.0.0");

    // Register a tool
    server.register_tool(
        "calculate",
        "Perform arithmetic operations",
        nlohmann::json::parse(R"({
            "type": "object",
            "properties": {
                "x": {"type": "number"},
                "y": {"type": "number"}
            }
        })"),
        [](const std::string& name, const nlohmann::json& args, RequestContext& ctx) {
            double x = args["x"];
            double y = args["y"];
            return nlohmann::json{
                {"content", nlohmann::json::array({
                    {{"type", "text"}, {"text", std::to_string(x + y)}}
                })}
            };
        }
    );

    // Main event loop
    std::string line;
    while (std::getline(std::cin, line)) {
        auto request = nlohmann::json::parse(line);
        auto response = server.handle_request(request);
        if (response) {
            std::cout << response->dump() << std::endl;
        }
    }

    return 0;
}
```

### Creating an MCP Client

```cpp
#include "mcpp/client.h"

using namespace mcpp;

int main() {
    // Create client with stdio transport
    auto transport = std::make_unique<StdioTransport>();
    McpClient client(std::move(transport));

    // Set message handler
    client.set_message_handler([](std::string_view method, const nlohmann::json& params) {
        // Handle server requests
        return nlohmann::json{};
    });

    // Connect and initialize
    client.connect();

    client.initialize(
        InitializeRequestParams{
            {"protocolVersion", "2025-11-25"},
            {"capabilities", nlohmann::json::object()},
            {"clientInfo", {{"name", "my-client"}, {"version", "1.0"}}}
        },
        [](const InitializeResult& result) {
            std::cout << "Connected to " << result.serverInfo.name << std::endl;
        },
        [](const JsonRpcError& error) {
            std::cerr << "Init failed: " << error.message << std::endl;
        }
    );

    // List available tools
    client.list_tools(
        [](const nlohmann::json& tools) {
            for (const auto& tool : tools["tools"]) {
                std::cout << "Tool: " << tool["name"] << std::endl;
            }
        }
    );

    return 0;
}
```

## Examples

### Inspector Server

An example MCP server compatible with [MCP Inspector](https://github.com/modelcontextprotocol/inspector) is provided:

```bash
# Build the example
cmake --build build --target inspector_server

# Run with MCP Inspector
mcp-inspector connect stdio ./build/examples/inspector_server
```

The inspector server demonstrates:
- **Tools**: `calculate`, `echo`, `get_time`
- **Resources**: `file://tmp/mcpp_test.txt`, `info://server`
- **Prompts**: `greeting`, `code_review`

### Testing with MCP Inspector

[MCP Inspector](https://github.com/modelcontextprotocol/inspector) is an interactive tool for testing MCP servers:

1. Install Inspector:
   ```bash
   npm install -g @modelcontextprotocol/inspector
   ```

2. Connect to your server:
   ```bash
   mcp-inspector connect stdio ./build/examples/inspector_server
   ```

3. In the Inspector UI:
   - View available tools, resources, and prompts
   - Call tools with parameters
   - Read resources
   - Generate prompts with arguments

## Running Tests

```bash
# Build all tests
cmake --build build

# Run unit tests
ctest --test-dir build -L unit --output-on-failure

# Run integration tests
ctest --test-dir build -L integration --output-on-failure

# Run specific test
./build/tests/mcpp_unit_tests --gtest_filter="JsonRpc*"
```

## Protocol Support

mcpp implements the following MCP capabilities:

| Capability | Status |
|------------|--------|
| Tools | Full support |
| Resources | Full support |
| Prompts | Full support |
| Roots | Client support |
| Sampling | Client support |
| Tasks | Experimental support |

## License

MIT License - see [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Links

- [MCP Specification](https://spec.modelcontextprotocol.io/)
- [MCP Inspector](https://github.com/modelcontextprotocol/inspector)
- [Example Servers](https://github.com/modelcontextprotocol/servers)
