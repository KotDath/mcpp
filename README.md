# MCPP - Model Context Protocol C++ Library

A modern C++17 implementation of the Model Context Protocol (MCP) version 2025-06-18.

## Features

- ✅ **JSON-RPC 2.0 Foundation** - Complete implementation of JSON-RPC 2.0 specification
- ✅ **Stdio Transport** - Primary transport for Claude Desktop and local integrations
- ✅ **Core Protocol Methods** - initialize, ping, notifications support
- ✅ **Modern C++17** - RAII, smart pointers, exception safety
- ✅ **Thread-Safe** - Safe concurrent operations
- ✅ **Header-Only Dependencies** - Easy integration with git submodules
- ✅ **Comprehensive Logging** - spdlog integration
- ✅ **Error Handling** - Proper exception hierarchy

## Supported Transports

- **Stdio** - stdin/stdout communication (primary transport)
- **HTTP** - Planned for web integrations
- **SSE** - Planned for notifications

## Dependencies

All dependencies are included as git submodules:

- **nlohmann/json v3.12.0** - JSON parsing and serialization
- **cpp-httplib v0.26.0** - HTTP transport (future use)
- **spdlog v1.16.0** - High-performance logging
- **base64.hpp** - Base64 encoding for binary content

## Building

### Prerequisites

- **CMake** 3.19 or later
- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **Git** (for submodules)

### Build Steps

```bash
# Clone the repository with submodules
git clone --recurse-submodules <repository-url>
cd mcpp

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run examples
./examples/echo_server
./examples/stdio_client
```

### Build Options

- `MCPP_BUILD_TESTS=ON` - Build tests (default: ON)
- `MCPP_BUILD_EXAMPLES=ON` - Build examples (default: ON)
- `MCPP_ENABLE_SSL=ON` - Enable SSL/TLS support (default: OFF)

## Usage

### Basic Echo Server

```cpp
#include "mcpp/mcpp.h"

int main() {
    // Initialize library
    mcpp::initialize();

    // Create transport
    auto transport = std::make_unique<mcpp::transport::StdioTransport>();
    transport->start();

    // Your server logic here...

    return 0;
}
```

### Client Example

```cpp
#include "mcpp/mcpp.h"

int main() {
    mcpp::initialize();

    auto transport = std::make_unique<mcpp::transport::StdioTransport>();

    // Send initialize request
    model::InitializeRequest::Params params;
    params.protocol_version = core::ProtocolVersion::LATEST;
    // ... set other params

    auto request = model::InitializeRequest(params);
    transport->send(*request.to_json_rpc_request());

    // Receive response...

    return 0;
}
```

## Examples

### Echo Server

A simple server that echoes back all requests:

```bash
# Build
cmake --build . --target echo_server

# Run
./examples/echo_server
```

### Stdio Client

Interactive client for testing:

```bash
# Build
cmake --build . --target stdio_client

# Run
./examples/stdio_client
```

Available commands:
- `ping` - Send ping request
- `echo <json>` - Send custom echo request
- `quit` - Exit client

## Architecture

The library follows a layered architecture:

```
Application Layer (Your code)
    ↓
Service Layer (McpClient, McpServer)
    ↓
Handler Layer (ToolHandler, ResourceHandler, PromptHandler)
    ↓
Model Layer (Requests, Responses, Notifications)
    ↓
Protocol Layer (JSON-RPC 2.0, MCP Protocol)
    ↓
Transport Layer (Stdio, HTTP, SSE)
```

## Protocol Support

### Currently Supported (Phase 1) ✅

- ✅ `initialize` - Protocol initialization
- ✅ `notifications/initialized` - Initialization complete
- ✅ `ping` - Connection health check
- ✅ `notifications/cancelled` - Request cancellation

### Planned (Phase 2+)

- 🔄 `tools/list` - List available tools
- 🔄 `tools/call` - Execute tools
- 🔄 `resources/list` - List resources
- 🔄 `resources/read` - Read resource content
- 🔄 `prompts/list` - List prompts
- 🔄 `prompts/get` - Get prompt content

## Development Status

**Phase 1: Foundation** ✅ COMPLETE
- [x] JSON-RPC 2.0 implementation
- [x] Stdio transport
- [x] Core protocol methods
- [x] Basic examples
- [x] Error handling
- [x] Logging

**Phase 2: Tools Framework** (Next)
- [ ] Tool registration and execution
- [ ] Tool schemas and validation
- [ ] Tool discovery

**Phase 3: Resources Framework**
- [ ] Resource management
- [ ] Subscription system
- [ ] File system integration

**Phase 4: Prompts Framework**
- [ ] Prompt management
- [ ] Template system

**Phase 5: Advanced Features**
- [ ] HTTP transport
- [ ] SSE transport
- [ ] SSL/TLS support
- [ ] Performance optimization

## License

This project is licensed under the MIT License.

## Contributing

Contributions are welcome! Please ensure:
- C++17 compliance
- Thread safety
- Proper error handling
- Comprehensive tests
- Documentation updates

## Related Projects

- [Model Context Protocol](https://modelcontextprotocol.io/) - Official specification
- [Rust SDK](https://github.com/modelcontextprotocol/rust-sdk) - Official Rust implementation
- [Java SDK](https://github.com/modelcontextprotocol/java-sdk) - Official Java implementation
- [Go SDK](https://github.com/modelcontextprotocol/go-sdk) - Official Go implementation 
