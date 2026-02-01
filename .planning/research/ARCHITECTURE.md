# Architecture Research: MCP Inspector Integration and Bug Fixes

**Project:** mcpp - C++ MCP Library v1.1
**Research Date:** 2026-02-01
**Confidence:** HIGH

## Summary

The MCP Inspector CLI integration and JSON-RPC bug fixes are **layered on top of existing architecture** without requiring structural changes. The inspector_server.cpp example already demonstrates the integration pattern, using the existing `McpServer` facade with stdio transport. The JSON-RPC parse error bug (-32700) is isolated to the request/response message handling layer in `inspector_server.cpp` and potentially `McpServer::handle_request()`.

The architecture follows a **layered design** with clear separation:
- **Transport Layer** (stdio/HTTP) - message framing
- **JSON-RPC Layer** - request/response parsing, validation
- **MCP Server/Client Facades** - MCP protocol specifics, registries
- **Example Server** - application-level code

Inspector integration and bug fixes primarily touch the **application layer** and **JSON-RPC validation**, not the core library architecture.

## Integration Points

### With JSON-RPC Layer

**Existing components:**
- `core/json_rpc.h/cpp` - `JsonRpcRequest`, `JsonRpcResponse`, `JsonRpcNotification` types
- `core/json_rpc.cpp` - `JsonRpcResponse::from_json()` for parsing responses
- `core/request_tracker.h/cpp` - Request ID generation and pending request tracking

**Inspector integration:**
- Inspector sends JSON-RPC 2.0 requests via stdio
- `inspector_server.cpp` uses `json::parse()` directly in main loop (line 359)
- Parse error handling is **ad-hoc in example code** (lines 366-413) with manual string parsing for ID extraction

**Bug fix location:**
- The parse error (-32700) handling in `inspector_server.cpp` is a **workaround**, not architectural
- Proper fix: Move JSON-RPC parsing into reusable component
- Consider adding `JsonRpcRequest::from_json()` to mirror existing `JsonRpcResponse::from_json()`

### With Transport Layer

**Existing components:**
- `transport/Transport` - abstract base class
- `transport/StdioTransport` - stdio implementation (subprocess communication)
- `transport/HttpTransport` - HTTP/SSE implementation

**Inspector integration:**
- Inspector uses **stdio transport** via subprocess spawning
- `inspector_server.cpp` uses manual stdio (`std::getline(std::cin, line)`) instead of `StdioTransport`
- This is intentional for the example (keeps it simple and self-contained)

**No transport changes needed** - Inspector spawns the server as a subprocess and communicates via stdin/stdout.

### With Build System

**Existing structure:**
- `CMakeLists.txt` - root build, defines `mcpp_static` and `mcpp_shared` targets
- `examples/CMakeLists.txt` - builds `inspector_server` executable
- `tests/CMakeLists.txt` - unit and integration tests

**Inspector integration:**
- Inspector is **external dependency** in `thirdparty/inspector/` (Node.js-based)
- Example server links against `mcpp_shared` or `mcpp_static` (conditional on `BUILD_SHARED_LIBS`)
- No build system changes required for bug fixes

**Testing infrastructure additions:**
- New test scripts will likely be shell/Node.js scripts that spawn `inspector_server`
- These live in `tests/` or new `tests/integration/inspector/` directory

## New Components

### Example Server Enhancements

| Component | Purpose | Notes |
|-----------|---------|-------|
| **Improved parse error handling** | Extract ID without ad-hoc string parsing | Move to `JsonRpcRequest::from_json()` |
| **Request validation wrapper** | Centralized JSON-RPC validation | Add to `core/json_rpc.h` |
| **Inspector test helper** | Spawn server + connect Inspector | New test utility script |

### Testing Infrastructure

| Component | Purpose | Notes |
|-----------|---------|-------|
| **Automated CLI test script** | Run Inspector commands against example server | Shell or Node.js script |
| **Test fixture server** | Minimal MCP server for testing | May extend existing `inspector_server.cpp` |
| **Integration test suite** | Validate all MCP methods work via Inspector | Tests in `tests/integration/` |

## Modified Components

| Component | Current State | Required Changes |
|-----------|--------------|------------------|
| `inspector_server.cpp` | Manual JSON parsing, ad-hoc error handling | Extract ID from parsed request before catch block; simplify error handling |
| `core/json_rpc.h` | Has `JsonRpcResponse::from_json()` | Add `JsonRpcRequest::from_json()` for proper parsing |
| `core/json_rpc.cpp` | Response parsing only | Implement request parsing with validation |
| `McpServer::handle_request()` | Assumes valid JSON passed in | Add try-catch around method dispatch for proper error responses |

**Key insight:** The bug (-32700 parse error on all tool calls) is likely in how `inspector_server.cpp` handles malformed JSON. The current code catches `json::exception` after parsing fails, then attempts to extract the ID via string parsing. This is fragile.

## Bug Investigation Approach

**JSON-RPC Parse Error (-32700) on all tool calls:**

### Root Cause Hypothesis

The parse error occurs because:
1. Inspector sends a request that fails `json::parse()`
2. The catch block extracts ID via ad-hoc string parsing (lines 371-401)
3. If extraction fails, `id` remains `nullptr`
4. Error response with `null` ID is confusing/misleading

### Investigation Steps

1. **Add request logging** - Log raw incoming requests before parsing
2. **Verify Inspector output** - Capture exact JSON Inspector sends
3. **Test with `JsonRpcRequest::from_json()`** - Implement proper parsing with validation
4. **Check newline handling** - Ensure stdio messages are properly delimited
5. **Validate jsonrpc version field** - Inspector may send requests without `"jsonrpc": "2.0"`

### Specific Areas to Check

```cpp
// Current (inspector_server.cpp:359)
json request = json::parse(line);  // May throw for valid JSON!
```

**Potential issues:**
- Inspector sends requests with embedded newlines in string values
- Inspector sends JSON without `"jsonrpc": "2.0"` field
- Inspector sends requests with `null` id that gets mis-parsed
- Character encoding issues (UTF-8 BOM, etc.)

### Fix Strategy

1. **Implement `JsonRpcRequest::from_json()`:**
```cpp
static std::optional<JsonRpcRequest> from_json(const JsonValue& j);
```
   - Returns nullopt for invalid requests
   - Extracts ID safely before validation
   - Provides clear error messages

2. **Update `inspector_server.cpp` main loop:**
```cpp
auto request = JsonRpcRequest::from_json(request_json);
if (!request) {
    // Send proper error with ID extracted safely
    continue;
}
// Use request->method, request->id, etc.
```

3. **Add request validation in `McpServer::handle_request()`:**
   - Check for required fields before dispatch
   - Return proper `-32600` (Invalid Request) for malformed requests

## Build Order

### Phase 1: Bug Fix Foundation (Independent of Inspector)

**Rationale:** Fix the JSON-RPC parsing layer first, as this affects all request handling regardless of transport.

1. **Add `JsonRpcRequest::from_json()`** - `core/json_rpc.h/cpp`
   - Mirrors existing `JsonRpcResponse::from_json()`
   - Validates jsonrpc version, method, id fields
   - Returns std::optional for error handling

2. **Update `McpServer::handle_request()`** - `server/mcp_server.cpp`
   - Wrap existing logic in try-catch for JSON exceptions
   - Return proper JSON-RPC error responses
   - Don't assume valid JSON input

3. **Add unit tests** - `tests/unit/test_json_rpc.cpp`
   - Test `JsonRpcRequest::from_json()` with valid/invalid input
   - Test error code generation

**Dependencies:** None (pure library changes)

### Phase 2: Example Server Update

**Rationale:** Update the example server to use the improved parsing layer.

1. **Refactor `inspector_server.cpp`** - `examples/inspector_server.cpp`
   - Replace manual `json::parse()` with `JsonRpcRequest::from_json()`
   - Simplify error handling (remove ad-hoc ID extraction)
   - Use proper error response builder

2. **Manual testing with Inspector**
   - Run Inspector against updated server
   - Verify tool calls work without parse errors
   - Verify error responses have correct IDs

**Dependencies:** Phase 1 (needs `JsonRpcRequest::from_json()`)

### Phase 3: Automated Testing

**Rationale:** Add automated tests to prevent regression.

1. **Create Inspector test script** - `tests/integration/inspector/`
   - Shell or Node.js script that spawns `inspector_server`
   - Sends test requests via stdin
   - Validates responses

2. **Add to CMake test suite** - `tests/CMakeLists.txt`
   - Use `add_test()` to register integration tests
   - Requires Inspector to be in PATH

**Dependencies:** Phase 2 (needs working example server)

### Phase 4: Documentation

**Rationale:** Document how to use Inspector for testing.

1. **Update README** - Document Inspector integration
2. **Add testing guide** - How to run automated tests

**Dependencies:** Phase 3 (tests need to exist first)

## Data Flow

### Inspector Connection Flow

```
[Inspector CLI (Node.js)]
       |
       | 1. Spawn subprocess
       v
[inspector_server executable]
       |
       | 2. Initialize McpServer
       v
[McpServer::handle_request()]
       |
       | 3. Route to registry (tools/resources/prompts)
       v
[ToolRegistry::call_tool(), etc.]
       |
       | 4. Return result
       v
[JSON-RPC response via stdout]
       |
       v
[Inspector displays result]
```

### Current Bug Scenario

```
Inspector sends: {"jsonrpc":"2.0","id":1,"method":"tools/call",...}
                          |
                          v
          json::parse(line) succeeds
                          |
                          v
          server.handle_request(request)
                          |
                          v
          [Processing...]
                          |
                          v
          std::cout << response->dump() << std::endl;
                          |
                          v
          Response sent WITHOUT trailing newline?
          OR response contains embedded newlines?
                          |
                          v
          Inspector sees partial/broken response
                          |
                          v
          Inspector sends error response (-32700)
```

**Hypothesis:** The bug may be:
1. Missing newline delimiter (line 363: `response->dump()` without `std::endl`)
2. Embedded newlines in tool results confuse line-based reading
3. Response format mismatch (e.g., extra nesting)

## Example Server Relationship to Architecture

The `inspector_server.cpp` is an **application-level example**, not part of the core library:

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  inspector_server.cpp                                │   │
│  │  - Spawns as subprocess                              │   │
│  │  - Uses stdio directly (not StdioTransport)         │   │
│  │  - Creates McpServer instance                        │   │
│  │  - Registers tools/resources/prompts                 │   │
│  │  - Main loop: read JSON, dispatch, write response    │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            │ uses
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                      Core Library Layer                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ McpServer    │  │ ToolRegistry │  │ JSON-RPC     │     │
│  │ (facade)     │  │              │  │ Types        │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

**Key points:**
- The example server is **user code** demonstrating how to use the library
- Bug fixes in the example don't affect library API
- The example can be rewritten without touching core components
- For production use, users would write their own server using similar patterns

## Sources

- **Code analysis:**
  - `examples/inspector_server.cpp` (lines 352-414) - main request/response loop
  - `src/mcpp/server/mcp_server.cpp` (lines 95-180) - `handle_request()` implementation
  - `src/mcpp/core/json_rpc.h` - JSON-RPC type definitions
  - `src/mcpp/core/json_rpc.cpp` - Response parsing implementation

- **JSON-RPC 2.0 Specification:** https://www.jsonrpc.org/specification
  - Error code -32700: Parse error (Invalid JSON was received)
  - Error code -32600: Invalid Request (JSON is valid but not valid JSON-RPC)

- **MCP Inspector repository:** `thirdparty/inspector/`
  - Node.js-based testing tool
  - Communicates via stdio (subprocess) or HTTP/SSE

- **Existing test patterns:** `tests/unit/test_json_rpc.cpp`
  - Shows how JSON-RPC types are tested
  - Error code testing patterns

---
*Architecture research for: mcpp v1.1 (Inspector CLI integration + bug fixes)*
*Researched: 2026-02-01*
