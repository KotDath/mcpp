# Features Research: MCP Inspector CLI Integration

**Project:** mcpp - C++ MCP Library v1.1
**Research Date:** 2026-02-01
**Confidence:** HIGH

## Summary

MCP Inspector integration requires a stdio-transport MCP server that correctly implements JSON-RPC 2.0 protocol with proper message formatting. The Inspector CLI (`--cli` flag) expects servers to handle standard MCP methods (tools/list, tools/call, resources/list, prompts/list, etc.) and return valid JSON responses. The critical JSON-RPC parse error (-32700) indicates the server is sending malformed JSON or violating JSON-RPC spec expectations. Key requirements include proper request ID handling, correct error response formatting, and support for all MCP resource types.

## Table Stakes (Must Have)

### Inspector CLI Compatibility

- **Stdio transport support** — Inspector communicates via stdin/stdout; server must read complete JSON-RPC messages line-by-line from stdin and write responses to stdout
- **JSON-RPC 2.0 compliance** — All messages must include `jsonrpc: "2.0"` field; requests need `method` and `id`; responses need matching `id` and either `result` or `error`
- **Request ID preservation** — Echo the exact `id` from request in response (supports numeric, string, and null IDs per JSON-RPC 2.0 spec)
- **Initialize handshake** — Respond to `initialize` method with server info and capabilities
- **Tools/list endpoint** — Return array of available tools with name, description, and inputSchema
- **Tools/call endpoint** — Accept tool name and arguments, return content array or error
- **Resources/list endpoint** — Return list of available resources with URIs
- **Prompts/list endpoint** — Return list of available prompts
- **Shutdown handling** — Graceful exit on shutdown notification

### Bug Fix Features

- **Parse error fix (-32700)** — Root cause likely in JSON serialization or request/response matching; verify nlohmann/json output format matches Inspector expectations
- **Proper error responses** — Error messages must include `code`, `message`, and optional `data` fields per JSON-RPC spec
- **Message boundary handling** — Ensure messages are delimited by newlines and complete JSON objects are sent
- **Null ID handling** — JSON-RPC 2.0 allows null IDs (used for parse error responses when request ID cannot be determined)

### Automated Testing

- **CLI integration test script** — Shell script that spawns inspector_server, calls tools/list via Inspector CLI, validates JSON output
- **Tools coverage test** — Test each registered tool (calculate, echo, get_time) with valid and invalid arguments
- **Resource read test** — Verify resource endpoints return proper content with correct MIME types
- **Prompt retrieval test** — Test prompt list and get with various argument combinations
- **Error handling test** — Verify proper error responses for malformed requests, unknown methods, invalid parameters
- **CI/CD integration** — Test script must run non-interactively and return proper exit codes

## Differentiators (Nice to Have)

- **Streaming tool output** — Support progress token notifications during long-running operations
- **Server capabilities metadata** — Expose detailed server info including protocol version, capabilities, server info in initialize response
- **Configurable logging** — Optional debug output to stderr for troubleshooting (without interfering with stdio JSON-RPC)
- **Test data fixtures** — Pre-defined test scenarios covering edge cases (unicode, special characters, large payloads)
- **Performance metrics** — Optional timing information in test output
- **Multi-transport testing** — Extend tests to cover SSE transport when available

## Anti-Features (Deliberately NOT Build)

- **Built-in HTTP server** — MCP Inspector connects via stdio for CLI mode; HTTP server is user's responsibility
- **Custom protocol extensions** — Stick to MCP 2025-11-25 spec; no custom message types
- **Interactive mode** — Inspector CLI is the interactive tool; mcpp should be a headless server
- **Bundled Inspector** — Use `npx @modelcontextprotocol/inspector` rather than vendoring the tool
- **GUI elements** — Inspector provides the UI; mcpp is the backend server only
- **Authentication in server** — Inspector handles proxy authentication; server should not implement auth

## Complexity Assessment

| Feature | Complexity | Notes |
|---------|-----------|-------|
| Stdio transport fix | Low | Already implemented; need to verify message format |
| JSON-RPC parse error fix | High | Root cause analysis needed; may be in serialization layer |
| Request ID preservation | Low | Existing code should handle this; verify variant type handling |
| Error response formatting | Low | Well-defined spec; existing JsonRpcError type |
| CLI integration test script | Medium | Requires subprocess management and JSON validation |
| Tools coverage test | Low | Straightforward tool calls with argument variations |
| Resource read test | Low | Simple resource URIs and content validation |
| Prompt retrieval test | Low | Similar to tools; arguments schema validation |
| Error handling test | Medium | Need to generate various malformed requests |
| CI/CD integration | Low | Standard script execution in build pipeline |

## Dependencies on Existing Features

| New Feature | Depends On | Status |
|-------------|------------|--------|
| Stdio transport | mcpp::transport::StdioTransport | Exists |
| JSON-RPC handling | mcpp::core::JsonRpcRequest/Response | Exists |
| Tool registry | mcpp::server::ToolRegistry | Exists |
| Resource registry | mcpp::server::ResourceRegistry | Exists |
| Prompt registry | mcpp::server::PromptRegistry | Exists |
| Example server | examples/inspector_server.cpp | Exists (needs fix) |

## Inspector CLI Expected Behavior

Based on analysis of [MCP Inspector source code](https://github.com/modelcontextprotocol/inspector):

1. **Server invocation**: Inspector spawns server process and connects stdio
2. **Initialize**: Sends `initialize` request with client info
3. **Discovery**: Calls `tools/list`, `resources/list`, `prompts/list`
4. **Invocation**: Calls `tools/call` with tool name and arguments
5. **Validation**: Expects valid JSON responses with matching IDs
6. **Error handling**: Expects JSON-RPC error responses for failures

Expected message format:
```json
// Request
{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"echo","arguments":{"text":"hello"}}}

// Success Response
{"jsonrpc":"2.0","id":1,"result":{"content":[{"type":"text","text":"Echo: hello"}]}}

// Error Response
{"jsonrpc":"2.0","id":1,"error":{"code":-32602,"message":"Invalid params"}}
```

## CLI Testing Patterns from Inspector Test Suite

Based on Inspector's test suite analysis:

### Basic Tool Discovery Test
```bash
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server \
  --method tools/list
```
Expected: JSON array of tools with `name`, `description`, `inputSchema`

### Tool Call Test
```bash
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server \
  --method tools/call \
  --tool-name echo \
  --tool-arg "message=Hello World"
```
Expected: JSON with `content` array containing text content

### Resource Read Test
```bash
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server \
  --method resources/read \
  --uri "info://server"
```
Expected: JSON with `contents` array with URI, mimeType, and text

### Error Handling Test
```bash
# Missing required parameter
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server \
  --method tools/call \
  --tool-name calculate \
  --tool-arg "a=5"
```
Expected: Error response with code -32602 (Invalid params)

## Sources

### HIGH Confidence (Official Source Code)
- [MCP Inspector Repository](https://github.com/modelcontextprotocol/inspector) — Official Inspector implementation with CLI mode
- [thirdparty/inspector/cli/src/cli.ts](https://github.com/modelcontextprotocol/inspector/blob/main/cli/src/cli.ts) — CLI entry point showing server invocation
- [thirdparty/inspector/cli/__tests__/cli.test.ts](https://github.com/modelcontextprotocol/inspector/blob/main/cli/__tests__/cli.test.ts) — CLI test suite showing expected behavior
- [thirdparty/inspector/cli/__tests__/tools.test.ts](https://github.com/modelcontextprotocol/inspector/blob/main/cli/__tests__/tools.test.ts) — Tool testing patterns
- [thirdparty/inspector/cli/__tests__/helpers/test-server-stdio.ts](https://github.com/modelcontextprotocol/inspector/blob/main/cli/__tests__/helpers/test-server-stdio.ts) — Reference stdio server implementation

### HIGH Confidence (Official Documentation)
- [MCP Inspector Documentation](https://modelcontextprotocol.io/docs/tools/inspector) — Official Inspector usage guide
- [MCP 2025-11-25 Specification](https://modelcontextprotocol.io/spec) — Protocol specification

### MEDIUM Confidence (Web Search + Verification)
- [MCP Inspector Setup Guide](https://mcpcat.io/guides/setting-up-mcp-inspector-server-testing/) — Testing patterns and CLI usage
- [How to Test Your MCP Server using MCP Inspector](https://medium.com/@anil-goyal0057/how-to-test-your-mcp-server-using-mcp-inspector-c873c417eec1) — Testing workflow examples

### Project Files Analyzed
- `examples/inspector_server.cpp` — Current example server (has parse error handling)
- `tests/unit/test_json_rpc.cpp` — JSON-RPC unit tests showing expected formats
- `thirdparty/gopher-mcp/sdk/typescript/mcp-example/` — TypeScript reference implementation
