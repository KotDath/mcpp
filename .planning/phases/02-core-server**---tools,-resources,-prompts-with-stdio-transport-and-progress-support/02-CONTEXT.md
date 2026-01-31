# Phase 2: Core Server - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

## Phase Boundary

Implement a working MCP server that can list and call tools, list and read resources, and list and get prompts. Support stdio transport with subprocess spawning and progress token support for long-running operations.

## Implementation Decisions

### Tool registration
- Method registration pattern following gopher-mcp: `registerTool(name, description, schema, handler)`
- Handler signature: `std::function<ToolResult(const std::string& name, const json& args, RequestContext& ctx)>`
- Error handling: Handler returns structured `ToolResult` with `isError` flag (not exception-based)
- JSON Schema validation: Automatic pre-validation by library before calling handler (handler receives validated JSON)

### Resource serving
- Handler-based registration like tools: `registerResource(uri, mime_type, handler)`
- URI scheme: Scheme-agnostic - user decides what schemes to support
- MIME types: Declared at registration, library reports to client
- Content representation: Unified `ResourceContent` type that holds text or blob (gopher-mcp style)

### Progress reporting
- Progress token: Opaque string token following MCP spec (client-generated, server echoes back)
- Reporting mechanism: Via `RequestContext` - handler calls `ctx.reportProgress(token, value, message)`
- Progress values: Percentage (0-100) - universally understood, maps cleanly to JSON
- Messages: Optional message parameter included with progress updates

### Stdio transport
- Creation: Subprocess spawning - library spawns command and manages process lifecycle
- Spawn API: `spawn(command, args)` - simple command spawning interface
- Spawn failures: Reported via `std::expected<Transport, Error>` for explicit error handling
- Cleanup: Auto-kill subprocess on transport destruction (RAII cleanup)

### Claude's Discretion
- Exact message framing implementation (newline-delimited JSON per MCP spec)
- Buffer sizes and I/O optimization
- Process signal handling details

## Specific Ideas

- Follow gopher-mcp patterns where applicable for C++ idioms
- All JSON must be cross-language compatible - not C++-specific objects
- Subprocess spawning is preferred over wrapping stdio directly

## Deferred Ideas

None - discussion stayed within phase scope.

---

*Phase: 02-core-server*
*Context gathered: 2026-01-31*
