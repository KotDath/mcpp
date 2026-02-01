# mcpp - MCP C++ Library

A modern C++ library for building Model Context Protocol (MCP) clients and servers. Provides full MCP 2025-11-25 spec coverage with all transports, fast JSON-RPC handling, and minimal blocking operations.

## Features

- **Full MCP 2025-11-25 spec support**: Tools, resources, prompts, logging, sampling, roots, progress, cancellations, streaming
- **Multiple transports**: stdio and Streamable HTTP (SSE)
- **Fast and correct**: JSON-RPC 2.0 compliant with minimal blocking operations
- **Thread-safe**: All public methods safe to call from any thread
- **Layered API**: Low-level callback core + high-level RAII wrappers
- **Permissive license**: MIT for both static and shared library distribution

## Requirements

- C++20 compiler (gcc-11+, clang-12+, MSVC 2019+)
- CMake 3.16+
- nlohmann/json 3.11+ (included in third_party/)
- spdlog 1.8+ (optional, for logging)

## Building

### Quick Start

```bash
# Clone the repository
git clone https://github.com/mcpp-project/mcpp.git
cd mcpp

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure

# Test with MCP Inspector (requires Node.js)
npx @modelcontextprotocol/inspector connect stdio ./build/examples/inspector_server

# Install (optional)
cmake --install build --prefix /usr/local
```

### Build Options

```bash
# Build static library only
cmake -B build -DBUILD_SHARED_LIBS=OFF

# Disable tests
cmake -B build -DMCPP_BUILD_TESTS=OFF

# Disable examples
cmake -B build -DMCPP_BUILD_EXAMPLES=OFF

# Debug build with sanitizers
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=leak -g"
```

## Usage

### Basic Server

```cpp
#include "mcpp/server/mcp_server.h"
#include "mcpp/transport/stdio_transport.h"

int main() {
    auto transport = std::make_unique<mcpp::StdioTransport>();
    mcpp::server::McpServer server(std::move(transport));

    // Register a tool
    server.get_tool_registry().register_tool(
        "calculate",
        {{"description", "Perform calculations"}},
        [](const nlohmann::json& params) {
            double a = params["a"];
            double b = params["b"];
            return mcpp::server::ToolResult{
                {"content", nlohmann::json::array({{
                    {"type", "text"},
                    {"text", std::to_string(a + b)}
                }})}
            };
        }
    );

    // Run server
    server.run();
    return 0;
}
```

### Basic Client

```cpp
#include "mcpp/client.h"
#include "mcpp/transport/stdio_transport.h"

int main() {
    auto transport = std::make_unique<mcpp::StdioTransport>();
    mcpp::McpClient client(std::move(transport));

    // Set message handler
    client.set_notification_handler([](const nlohmann::json& notification) {
        std::cout << "Received: " << notification.dump(2) << std::endl;
    });

    // Initialize
    auto init_result = client.initialize({
        {"protocolVersion", "2025-11-25"},
        {"capabilities", nlohmann::json::object()},
        {"clientInfo", {{"name", "my_client"}, {"version", "1.0"}}}
    });

    // Call a tool
    auto result = client.call_tool("calculate", {{"a", 5}, {"b", 3}});

    return 0;
}
```

## Examples

### Inspector Server

An example MCP server for testing with MCP Inspector:

```bash
# Build the example
cmake --build build --target inspector_server

# Run with MCP Inspector (opens browser UI)
npx @modelcontextprotocol/inspector connect stdio ./build/examples/inspector_server
```

The inspector server demonstrates:
- Tool registration (calculate, echo, get_time)
- Resource serving (file reading, server info)
- Prompt templates (greeting, code review)

## Inspector Testing

MCP Inspector is the official testing tool for MCP servers. It supports both interactive UI mode (opens a browser-based interface) and scriptable CLI mode for automated testing. Inspector uses stdio transport to communicate with your server, sending JSON-RPC messages over stdin/stdout.

### Communication Flow

```
Inspector          Your Server
   (Client)         (stdio mode)
      │                  │
      │ 1. initialize    │
      │ ────────────────>│
      │                  │
      │ 2. initialize    │
      │ <────────────────│
      │                  │
      │ 3. tools/list    │
      │ ────────────────>│
      │                  │
      │ 4. tools/list    │
      │ <────────────────│
```

### UI Mode (Interactive)

The easiest way to test your MCP server is with Inspector's UI mode:

```bash
# Build the example server first
cmake --build build --target inspector_server

# Connect via Inspector (opens browser to http://localhost:6274)
npx @modelcontextprotocol/inspector connect stdio ./build/examples/inspector_server
```

The Inspector UI lets you:
- List and call available tools
- Browse resources and prompts
- View server capabilities
- Inspect JSON-RPC messages in real-time

### CLI Mode (Scriptable)

For automated testing or scripting, use Inspector's CLI mode:

```bash
# List available tools
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server --method tools/list

# Call a tool
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server \
    --method tools/call --tool-name calculate --tool-arg operation=add --tool-arg a=5 --tool-arg b=3

# List resources
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server --method resources/list

# Read a resource
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server \
    --method resources/read --uri file:///path/to/file.txt
```

### Initialize Handshake

MCP requires an initialize handshake before most operations. Inspector handles this automatically. For manual testing with raw stdio, you need to send initialize first:

```bash
{ echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{...}}'; echo '{"jsonrpc":"2.0","id":2,"method":"tools/list"}'; } | ./build/examples/inspector_server
```

See [TESTING.md](TESTING.md) for detailed BATS testing guide and writing custom tests.

## Testing

```bash
# Run all tests
ctest --test-dir build

# Run specific test suites
ctest --test-dir build -L unit      # Unit tests only
ctest --test-dir build -L integration  # Integration tests only
ctest --test-dir build -L compliance   # JSON-RPC compliance tests

# Run with verbose output
ctest --test-dir build --verbose

# Run specific test
./build/tests/mcpp_unit_tests --gtest_filter="JsonRpcRequest.*"
```

### Sanitizer Testing

```bash
# AddressSanitizer + LeakSanitizer
cmake -B build/sanitizer -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -g" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address -fsanitize=leak"
cmake --build build/sanitizer
ctest --test-dir build/sanitizer

# ThreadSanitizer (separate build)
cmake -B build/tsan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
cmake --build build/tsan
ctest --test-dir build/tsan
```

## Installation

```bash
# Install to system (requires sudo)
sudo cmake --install build --prefix /usr/local

# Install to custom location
cmake --install build --prefix $HOME/.local

# Use in CMake project
find_package(mcpp REQUIRED)
target_link_libraries(my_app PRIVATE mcpp::mcpp)
```

## Project Structure

```
mcpp/
├── CMakeLists.txt          # Root build configuration
├── LICENSE                 # MIT license
├── README.md               # This file
├── src/
│   └── mcpp/              # Public headers
│       ├── api/           # High-level API
│       ├── async/         # Callback types and timeout
│       ├── client/        # Client implementation
│       ├── core/          # JSON-RPC and request tracking
│       ├── content/       # Content types and pagination
│       ├── protocol/      # MCP protocol types
│       ├── server/        # Server implementation
│       ├── transport/     # Transport layer
│       └── util/          # Utilities (logging, retry, etc.)
├── tests/
│   ├── unit/             # Unit tests
│   ├── integration/      # Integration tests
│   ├── compliance/       # JSON-RPC spec tests
│   └── fixtures/         # Test utilities
├── examples/             # Example programs
└── third_party/          # Vendored dependencies
```

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## Resources

- [MCP Specification](https://modelcontextprotocol.io)
- [MCP Inspector](https://github.com/modelcontextprotocol/inspector)
- [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification)

## Status

This project is in active development. Version 0.1.0-alpha targets full MCP 2025-11-25 spec support.
