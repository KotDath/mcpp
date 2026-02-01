# Phase 9: Inspector Server Integration - Research

**Researched:** 2026-02-01
**Domain:** MCP server example updates, MCP Inspector CLI integration, stdio protocol compliance
**Confidence:** HIGH

## Summary

This phase updates the example server (`inspector_server.cpp`) to consume the Phase 8 validation layer (`JsonRpcRequest::from_json()`) and ensure compatibility with the MCP Inspector CLI tool. The phase involves (1) replacing raw `nlohmann::json` parsing with the new `from_json()` validation, (2) implementing proper parse error response handling with extracted request IDs, (3) creating an `mcp.json` configuration file for Inspector CLI, and (4) documenting a manual testing workflow. This is an application-layer phase using only existing library capabilities.

**Primary recommendation:** Modify the existing `inspector_server.cpp` to use `JsonRpcRequest::from_json()` for request validation, add ID extraction helper to `JsonRpcRequest` class for parse error responses, create `examples/mcp.json` configuration for Inspector CLI, and document testing in `examples/TESTING.md`.

## Standard Stack

### Core (Existing - No Changes)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| mcpp library | v1.1 | MCP server implementation | Phase 8 delivers `JsonRpcRequest::from_json()` and `to_string_delimited()` |
| nlohmann/json | 3.11+ | JSON parsing/dumping | Already in use, required for stdio transport |
| C++20 stdlib | C++20 | std::optional, std::fprintf | Required for from_json() return type and debug macro |

### External Tools (For Testing)
| Tool | Version | Purpose | When to Use |
|---------|---------|-------------|
| MCP Inspector CLI | latest (via npx) | Manual testing of MCP servers | Local verification before release |
| Node.js | ^22.7.5 | Required to run Inspector | Inspector's dependency |

**Installation:**
```bash
# Inspector is run via npx (no installation required)
npx @modelcontextprotocol/inspector node build/examples/inspector_server

# For CLI mode:
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server
```

## Architecture Patterns

### Recommended Project Structure
```
examples/
├── inspector_server.cpp   # MODIFIED: Use JsonRpcRequest::from_json()
├── http_server_integration.cpp  # MODIFIED: For consistency
├── mcp.json               # NEW: Inspector CLI configuration
├── TESTING.md             # NEW: Manual testing documentation
└── CMakeLists.txt         # EXISTING: No changes needed

src/mcpp/core/
└── json_rpc.h             # MODIFIED: Add extract_request_id() helper
```

### Pattern 1: Request Validation via from_json()

**What:** Replace raw `nlohmann::json::parse()` with `JsonRpcRequest::from_json()` for proper JSON-RPC 2.0 validation.

**When to use:** All incoming stdio request parsing in example servers.

**Example (from Phase 8 implementation, now available for use):**
```cpp
// Source: src/mcpp/core/json_rpc.cpp (completed in Phase 8)
// OLD PATTERN (fragile - what inspector_server.cpp currently does):
json request = json::parse(line);  // Throws on malformed JSON
std::string method = request["method"];  // May throw if missing

// NEW PATTERN (robust):
auto parsed_request = JsonRpcRequest::from_json(json::parse(line));
if (!parsed_request) {
    // Validation failed - send parse error response
    json error_response = {
        {"jsonrpc", "2.0"},
        {"error", {{"code", -32700}, {"message", "Parse error"}}},
        {"id", nullptr}  // Or extract ID if possible
    };
    std::cout << error_response.dump() << std::endl;
    continue;
}
// Safe to use parsed_request->method, parsed_request->id, etc.
```

**Key insight:** The example server currently has manual ID extraction code (lines 367-401) that is fragile. The new `from_json()` validates the full JSON-RPC structure but returns `nullopt` on failure—this phase needs to add ID extraction for error responses.

### Pattern 2: ID Extraction for Parse Error Responses

**What:** Add helper to extract request ID from potentially malformed JSON for use in error responses.

**When to use:** When `from_json()` returns `nullopt` but you need to include the request ID in the error response (per JSON-RPC 2.0 spec).

**Example:**
```cpp
// In src/mcpp/core/json_rpc.h - add to JsonRpcRequest struct:
/**
 * Extract request ID from raw JSON (best-effort for malformed requests)
 *
 * This helper attempts to extract the 'id' field even when the JSON
 * is malformed. Used for constructing parse error responses with
 * the correct request ID.
 *
 * @param raw_json String containing the raw JSON request
 * @return Extracted ID (string, number, or null if not found)
 */
static RequestId extract_request_id(std::string_view raw_json);

// Usage in example server:
try {
    json parsed = json::parse(line);
    if (auto request = JsonRpcRequest::from_json(parsed)) {
        // Process valid request
    } else {
        // Validation failed - extract ID for error response
        RequestId id = JsonRpcRequest::extract_request_id(line);
        // Send error response with extracted ID
    }
} catch (const json::exception&) {
    // JSON is completely malformed - still try to extract ID
    RequestId id = JsonRpcRequest::extract_request_id(line);
    // Send parse error with extracted ID
}
```

### Pattern 3: mcp.json Configuration for Inspector

**What:** Configuration file for MCP Inspector CLI and other MCP clients.

**When to use:** All stdio-based MCP servers that want Inspector compatibility.

**Example:**
```json
{
  "mcpServers": {
    "mcpp-example": {
      "command": "./build/examples/inspector_server",
      "args": [],
      "env": {
        "MCPP_DEBUG": "1"
      }
    }
  }
}
```

**Configuration structure (from official Inspector docs):**
- `mcpServers`: Object containing named server entries
- `command`: Executable path (relative to mcp.json location)
- `args`: Optional array of command-line arguments
- `env`: Optional environment variables for the server

**Transport types supported:**
- `stdio` (default, shown above): Command-based server
- `sse`: Server-Sent Events URL
- `streamable-http`: HTTP endpoint URL

**Path handling:** Paths in `command` and `args` are resolved relative to the `mcp.json` file location. This means `./build/examples/inspector_server` from `examples/mcp.json` correctly resolves to the built executable.

### Pattern 4: Inspector CLI Usage

**What:** Standard commands for testing MCP servers with Inspector.

**When to use:** Manual verification of server functionality.

**UI mode (for interactive testing):**
```bash
# From project root (mcp.json in examples/ directory):
npx @modelcontextprotocol/inspector --config examples/mcp.json

# Or directly with command:
npx @modelcontextprotocol/inspector node build/examples/inspector_server
```

**CLI mode (for scripting/automation):**
```bash
# List available tools
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server --method tools/list

# Call a specific tool
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server \
    --method tools/call --tool-name calculate --tool-arg operation=add --tool-arg a=5 --tool-arg b=3

# List resources
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server --method resources/list

# List prompts
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server --method prompts/list
```

### Anti-Patterns to Avoid
- **Creating separate stdio_example.cpp**: Context.md explicitly says "Modify the existing inspector_server.cpp (do NOT create a separate example)"
- **Hardcoded absolute paths in mcp.json**: Use relative paths so the config works from any location
- **Skipping validation in examples**: All examples should demonstrate proper `from_json()` usage
- **Inconsistent error handling**: All examples should use the same pattern for parse errors

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| ID extraction from malformed JSON | Manual string search (like inspector_server.cpp lines 367-401) | `JsonRpcRequest::extract_request_id()` helper | Centralized, handles all ID formats (string, number, null), tested |
| Newline-delimited JSON output | `std::cout << json.dump() << "\n"` | `to_string_delimited()` (from Phase 8) | Consistent with library patterns, guarantees newline |
| Request validation | Custom field checks after `json::parse()` | `JsonRpcRequest::from_json()` | Validates full JSON-RPC 2.0 structure, type-safe |
| Debug output to stdout | `std::cout << "Debug: " << ...` | `MCPP_DEBUG_LOG()` (from Phase 8) | Guarantees stderr-only, prevents protocol pollution |

**Key insight:** Phase 8 delivered all the building blocks—this phase just wires them into the example server.

## Common Pitfalls

### Pitfall 1: Parse Error Responses Missing Request ID

**What goes wrong:** Parse error responses have `"id": null` even when the malformed request contained a valid ID field.

**Why it happens:** When `json::parse()` or `from_json()` fails, the request ID is lost unless extracted before validation.

**How to avoid:**
1. Add `JsonRpcRequest::extract_request_id()` helper for best-effort ID extraction
2. Call `extract_request_id()` before full validation or when validation fails
3. Use extracted ID in error response construction

**Example from inspector_server.cpp (lines 367-401) - fragile pattern to replace:**
```cpp
// CURRENT (fragile): Manual string search for "id" field
size_t id_pos = line.find("\"id\"");
if (id_pos != std::string::npos) {
    // ... 30+ lines of string manipulation
}
```

**Better approach (library helper):**
```cpp
RequestId id = JsonRpcRequest::extract_request_id(line);
```

### Pitfall 2: Relative Paths in mcp.json Not Resolving

**What goes wrong:** Inspector can't find the server executable because paths are resolved incorrectly.

**Why it happens:** mcp.json paths are relative to the config file's location, not the current working directory.

**How to avoid:**
1. Place mcp.json in `examples/` directory next to the example source
2. Use path `./build/examples/inspector_server` (relative to `examples/mcp.json`)
3. Or use `../../build/examples/inspector_server` if mcp.json is elsewhere
4. Test with `--config` flag: `npx @modelcontextprotocol/inspector --config examples/mcp.json`

### Pitfall 3: Inspector Connection Hangs

**What goes wrong:** Inspector appears to connect but shows no tools/resources/prompts.

**Why it happens:** Server isn't sending proper newline-delimited JSON, or debug output is polluting stdout.

**How to avoid:**
1. Use `to_string_delimited()` for all stdio output (from Phase 8)
2. Ensure all debug output uses `MCPP_DEBUG_LOG()` or `std::cerr`
3. Verify response format: `{"jsonrpc": "2.0", "result": {...}, "id": 1}\n`
4. Test with manual echo: `echo '{"jsonrpc":"2.0","method":"tools/list","id":1}' | ./build/examples/inspector_server`

### Pitfall 4: Mixed Validation Approaches Across Examples

**What goes wrong:** One example uses `from_json()`, another uses raw parsing—inconsistent patterns confuse users.

**Why it happens:** Example servers are updated piecemeal, not all at once.

**How to avoid:**
1. Update ALL examples (stdio, HTTP, SSE) in this phase
2. Use `from_json()` for ALL request parsing
3. Use `extract_request_id()` for ALL parse error handling
4. Use `to_string_delimited()` for ALL stdio output

### Pitfall 5: Testing Documentation Outdated

**What goes wrong:** TESTING.md references commands that don't work or Inspector options that changed.

**Why it happens:** Inspector evolves quickly; documentation becomes stale.

**How to avoid:**
1. Verify all commands in TESTING.md before committing
2. Include Inspector version requirement (Node.js ^22.7.5)
3. Document both UI and CLI modes
4. Include expected output examples
5. Note that Inspector is run via npx (no installation)

## Code Examples

Verified patterns from Phase 8 and official Inspector documentation:

### Existing Library API (Ready to Use)

```cpp
// Source: src/mcpp/core/json_rpc.h (Phase 8 deliverables)

// 1. Request validation (returns nullopt if invalid)
std::optional<JsonRpcRequest> JsonRpcRequest::from_json(const JsonValue& j);

// 2. Newline-delimited output (for stdio transport)
std::string JsonRpcResponse::to_string_delimited() const;
std::string JsonRpcRequest::to_string_delimited() const;
std::string JsonRpcNotification::to_string_delimited() const;

// 3. Debug logging to stderr (from util/logger.h)
MCPP_DEBUG_LOG("Request validation failed");
```

### Example Server Update Pattern

```cpp
// OLD (inspector_server.cpp lines 353-413):
while (std::getline(std::cin, line)) {
    try {
        json request = json::parse(line);
        std::optional<json> response = server.handle_request(request);
        if (response.has_value()) {
            std::cout << response->dump() << std::endl;  // Missing newline consistency
        }
    } catch (const json::exception& e) {
        // Fragile manual ID extraction (lines 367-401)
        json id = nullptr;
        size_t id_pos = line.find("\"id\"");
        // ... 30+ lines of string manipulation
        std::cout << error.dump() << std::endl;
    }
}

// NEW (pattern to implement):
while (std::getline(std::cin, line)) {
    try {
        json raw = json::parse(line);

        // Use Phase 8 validation
        if (auto parsed = JsonRpcRequest::from_json(raw)) {
            // Valid request - process with server
            std::optional<json> response = server.handle_request(raw);
            if (response.has_value()) {
                // Use Phase 8 delimiter helper
                std::cout << response->dump() << "\n" << std::flush;
            }
        } else {
            // Validation failed - extract ID and send error
            RequestId id = JsonRpcRequest::extract_request_id(line);
            json error = {
                {"jsonrpc", "2.0"},
                {"error", {{"code", -32700}, {"message", "Parse error"}}},
                {"id", id}
            };
            std::cout << error.dump() << "\n" << std::flush;
        }
    } catch (const json::exception& e) {
        // Completely malformed JSON - still try to extract ID
        RequestId id = JsonRpcRequest::extract_request_id(line);
        json error = {
            {"jsonrpc", "2.0"},
            {"error", {{"code", -32700}, {"message", "Parse error"}}},
            {"id", id}
        };
        std::cout << error.dump() << "\n" << std::flush;
    }
}
```

### mcp.json Configuration

```json
// Source: Designed for this phase
// Location: examples/mcp.json

{
  "mcpServers": {
    "mcpp-inspector-server": {
      "command": "./build/examples/inspector_server",
      "args": [],
      "env": {
        "MCPP_DEBUG": "1"
      }
    }
  }
}
```

**Usage:**
```bash
# From project root:
npx @modelcontextprotocol/inspector --config examples/mcp.json

# Inspector UI opens at http://localhost:6274
# Connects to inspector_server via stdio
```

### Inspector CLI Testing Commands

```bash
# Source: MCP Inspector documentation (verified)

# 1. List tools
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server \
    --method tools/list

# 2. Call a tool (calculator example)
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server \
    --method tools/call \
    --tool-name calculate \
    --tool-arg operation=add \
    --tool-arg a=42 \
    --tool-arg b=8

# 3. List resources
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server \
    --method resources/list

# 4. List prompts
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server \
    --method prompts/list

# 5. Get a prompt
npx @modelcontextprotocol/inspector --cli node build/examples/inspector_server \
    --method prompts/get \
    --tool-arg name=greeting \
    --tool-arg name=World
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual JSON parsing | `JsonRpcRequest::from_json()` validation | Phase 8 | Type-safe request validation |
| Fragile ID string extraction | Library `extract_request_id()` helper | This phase | Robust parse error responses |
| `std::cout << ... << std::endl` | `to_string_delimited()` helper | Phase 8 | Consistent newline delimiters |
| Debug output to stdout | `MCPP_DEBUG_LOG()` to stderr | Phase 8 | No protocol pollution |
| No Inspector configuration | Standard `mcp.json` format | This phase | Inspector CLI compatibility |

**Deprecated/outdated:**
- **Manual ID extraction code** (inspector_server.cpp lines 367-401): Replace with `JsonRpcRequest::extract_request_id()`
- **Mixed output patterns** (`dump()` + `std::endl`): Use `to_string_delimited()` consistently
- **No mcp.json**: Add standard configuration for Inspector compatibility

## Open Questions

1. **Should extract_request_id() return RequestId or std::optional<RequestId>?**
   - What we know: Need to extract ID for error responses; null is valid per JSON-RPC spec
   - What's unclear: Should missing ID be signaled as nullopt or return null RequestId?
   - Recommendation: Return `RequestId` with null default (simpler, matches existing pattern where null ID is valid)

2. **Should we add a separate parse error builder function?**
   - What we know: Error responses need specific structure (jsonrpc, error, id)
   - What's unclear: Should helper build entire error response, or just extract ID?
   - Recommendation: Just add `extract_request_id()` helper—example server can build error response. Keeps library minimal.

3. **Exact TESTING.md format**
   - What we know: Need documentation for manual testing workflow
   - What's unclear: How much detail, what sections to include?
   - Recommendation: Start with (1) Prerequisites, (2) Building, (3) UI Mode, (4) CLI Mode, (5) Expected Output. Add more based on Claude's discretion.

## Sources

### Primary (HIGH confidence)
- [MCP Inspector GitHub Repository](https://github.com/modelcontextprotocol/inspector) - Authoritative source for CLI usage, mcp.json format, configuration options
- [MCP Inspector Documentation](https://modelcontextprotocol.io/docs/tools/inspector) - Official usage patterns, CLI commands, configuration structure
- `src/mcpp/core/json_rpc.h` (Phase 8 implementation) - `JsonRpcRequest::from_json()`, `to_string_delimited()` methods
- `src/mcpp/util/logger.h` (Phase 8 implementation) - `MCPP_DEBUG_LOG` macro
- `examples/inspector_server.cpp` (current implementation) - Example of manual ID extraction to replace

### Secondary (MEDIUM confidence)
- [MCP Inspector Guide (mcpevals.io)](https://www.mcpevals.io/blog/mcp-inspector-guide) - Community tutorial on Inspector usage, verified against official docs
- Phase 8 RESEARCH.md - JSON-RPC validation patterns, stdio transport requirements

### Tertiary (LOW confidence)
- [How to use MCP Inspector (Medium)](https://medium.com/@laurentkubaski/how-to-use-mcp-inspector-2748cd33faeb) - Community tutorial, not verified against official docs
- Third-party MCP server examples - Used for pattern reference only, not copied

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - No new dependencies, Phase 8 deliverables verified
- Architecture: HIGH - Based on existing Phase 8 patterns, official Inspector documentation
- Pitfalls: MEDIUM - Inspector CLI is stable but evolving; mcp.json format is standardized but Edge cases may exist

**Research date:** 2026-02-01
**Valid until:** 2026-03-01 (30 days - Inspector is actively developed, but core patterns are stable)
