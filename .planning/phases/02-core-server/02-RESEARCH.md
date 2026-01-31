# Phase 2: Core Server - Research

**Researched:** 2026-01-31
**Domain:** MCP Server Implementation + JSON Schema Validation + stdio Transport + Progress Reporting
**Confidence:** HIGH

## Summary

This phase implements the MCP server layer with tools, resources, and prompts support, stdio transport with subprocess spawning, and progress token notification for long-running operations. The research focused on four key areas: (1) MCP 2025-11-25 server specification requirements, (2) JSON Schema validation approaches in C++, (3) stdio subprocess spawning patterns, and (4) progress notification mechanisms.

The research confirmed that the MCP 2025-11-25 schema is the authoritative source for all message types. Key findings include: tools require JSON Schema input validation with optional output schemas, resources support both text and binary content types with URI-based discovery, prompts support argument templating, progress tokens are passed via `_meta.progressToken` in requests and returned via `notifications/progress`, and stdio transport requires newline-delimited JSON framing.

**Primary recommendation:** Use the official MCP 2025-11-25 TypeScript schema as the source of truth for type definitions, implement handler registration pattern following gopher-mcp's method registry approach, use nlohmann/json-schema-validator for JSON Schema validation (it integrates directly with nlohmann/json), implement stdio transport with POSIX popen or custom fork/exec with pipe management, and use RequestContext pattern for progress reporting.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| **nlohmann/json** | 3.11+ | JSON serialization | De facto standard, header-only, excellent C++17 support |
| **nlohmann/json-schema-validator** | 2.3.0+ | JSON Schema validation | Official nlohmann addon, Draft-7 support, 100x faster than v1 |
| **MCP Schema** | 2025-11-25 | Protocol definition | Official TypeScript schema from modelcontextprotocol repo |
| **C++17** | gcc-11/g++-11 | Language standard | std::optional, std::variant, structured bindings required |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| **Google Test** | 1.11+ | Unit testing | For server component unit tests |
| **CMake** | 3.10+ | Build system | Required; FetchContent for dependencies |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| nlohmann/json-schema-validator | valijson | valijson is header-only but requires more setup; nlohmann integrates directly |
| nlohmann/json-schema-validator | custom validation | Hand-rolling is error-prone; JSON Schema Draft-7 has many edge cases |
| popen/posix_spawn | fork+exec manual | More control but significantly more complex; popen is sufficient for stdio |

**Installation:**
```cmake
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# nlohmann/json-schema-validator
FetchContent_Declare(
    json_schema_validator
    GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
    GIT_TAG v2.3.0
)
FetchContent_MakeAvailable(json_schema_validator)
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── mcpp/
│   ├── core/              # JSON-RPC 2.0 core (from Phase 1)
│   ├── protocol/          # MCP-specific protocol (from Phase 1)
│   ├── transport/         # Transport abstraction (from Phase 1)
│   ├── server/            # NEW: Server implementation
│   │   ├── mcp_server.h   # Main server class
│   │   ├── mcp_server.cpp
│   │   ├── tool_registry.h    # Tool registration and dispatch
│   │   ├── resource_registry.h # Resource registration
│   │   ├── prompt_registry.h   # Prompt registration
│   │   └── request_context.h   # RequestContext for progress
│   └── transport/
│       └── stdio_transport.h  # NEW: stdio transport implementation
```

### Pattern 1: Handler Registration (Tool/Resource/Prompt)
**What:** Method registration pattern where handlers are registered with metadata and dispatched by name
**When to use:** All server primitives (tools, resources, prompts)
**Example:**
```cpp
// Source: CONTEXT.md decisions + gopher-mcp patterns
namespace mcpp::server {

// Handler signature for tools
using ToolHandler = std::function<ToolResult(
    const std::string& name,
    const json& args,
    RequestContext& ctx
)>;

// Handler signature for resources
using ResourceHandler = std::function<ResourceContent(
    const std::string& uri
)>;

// Handler signature for prompts
using PromptHandler = std::function<std::vector<PromptMessage>(
    const std::string& name,
    const json& arguments
)>;

class McpServer {
public:
    // Register a tool with its schema
    void register_tool(
        const std::string& name,
        const std::string& description,
        const json& input_schema,  // JSON Schema for validation
        ToolHandler handler
    );

    // Register a resource
    void register_resource(
        const std::string& uri,
        const std::string& mime_type,
        ResourceHandler handler
    );

    // Register a prompt template
    void register_prompt(
        const std::string& name,
        const std::string& description,
        const json& arguments_schema,  // Prompt argument definitions
        PromptHandler handler
    );

    // Handle incoming tools/call request
    ToolResult call_tool(const std::string& name, const json& args, RequestContext& ctx);

private:
    struct ToolRegistration {
        std::string name;
        std::string description;
        json input_schema;
        ToolHandler handler;
    };

    std::unordered_map<std::string, ToolRegistration> tools_;
    // Similar maps for resources_ and prompts_
};

} // namespace mcpp::server
```

### Pattern 2: RequestContext for Progress Reporting
**What:** Context object passed to handlers that enables sending progress notifications
**When to use:** All handler invocations that may support progress
**Example:**
```cpp
// Source: MCP 2025-11-25 schema - progress via _meta.progressToken
namespace mcpp::server {

class RequestContext {
public:
    RequestContext(
        const std::string& request_id,
        Transport& transport
    );

    // Report progress for long-running operations
    void report_progress(
        const std::string& token,  // From request._meta.progressToken
        double progress,           // 0-100
        const std::string& message = ""
    );

    // Check if client requested progress
    bool has_progress_token() const;

    const std::string& request_id() const { return request_id_; }

private:
    std::string request_id_;
    Transport& transport_;
    std::optional<std::string> progress_token_;
};

} // namespace mcpp::server
```

### Pattern 3: JSON Schema Validation
**What:** Pre-validate tool arguments against registered schema before calling handler
**When to use:** All tools/call requests
**Example:**
```cpp
// Source: nlohmann/json-schema-validator documentation
#include <nlohmann/json-schema.hpp>

using nlohmann::json;
using nlohmann::json_schema::json_validator;

namespace mcpp::server {

class SchemaValidator {
public:
    // Compile schema once during registration
    void compile_schema(const json& schema);

    // Validate arguments before dispatch
    std::optional<std::string> validate(const json& arguments) const;

private:
    std::unique_ptr<json_validator> validator_;
};

// Tool dispatch with validation
ToolResult McpServer::call_tool(
    const std::string& name,
    const json& args,
    RequestContext& ctx
) {
    auto it = tools_.find(name);
    if (it == tools_.end()) {
        return ToolResult::error("Tool not found: " + name);
    }

    // Validate arguments against schema
    if (auto error = it->second.validator.validate(args)) {
        return ToolResult::error("Invalid arguments: " + *error);
    }

    // Call handler with validated arguments
    return it->second.handler(name, args, ctx);
}

} // namespace mcpp::server
```

### Pattern 4: Stdio Transport with Subprocess Spawning
**What:** Spawn subprocess and communicate via stdin/stdout with newline-delimited JSON
**When to use:** Client spawning an MCP server via stdio
**Example:**
```cpp
// Source: POSIX popen patterns + MCP stdio transport spec
namespace mcpp::transport {

class StdioTransport : public Transport {
public:
    // Spawn a subprocess for stdio communication
    static std::expected<StdioTransport, std::string> spawn(
        const std::string& command,
        const std::vector<std::string>& args
    );

    // Transport interface implementation
    bool send(std::string_view message) override;
    void set_message_callback(MessageCallback cb) override;

    ~StdioTransport() {
        // RAII cleanup: kill subprocess
        if (pid_ > 0) {
            ::kill(pid_, SIGTERM);
            waitpid(pid_, nullptr, 0);
        }
    }

private:
    StdioTransport(int pid, int read_fd, int write_fd)
        : pid_(pid), read_fd_(read_fd), write_fd_(write_fd) {}

    pid_t pid_;
    int read_fd_;   // Read from subprocess stdout
    int write_fd_;  // Write to subprocess stdin
    std::thread read_thread_;
};

} // namespace mcpp::transport
```

### Anti-Patterns to Avoid
- **Exception-based error handling in handlers:** Handlers should return ToolResult with isError flag, not throw exceptions
- **Synchronous blocking I/O in transport:** Use separate read thread for stdin to avoid blocking
- **Ignoring progress tokens:** Always check for and respect _meta.progressToken if present
- **Manual JSON parsing for schemas:** Use nlohmann/json-schema-validator; hand-rolled validation is incomplete
- **Fork without cleanup:** Subprocess must be killed on transport destruction

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| JSON Schema validation | Custom validator | nlohmann/json-schema-validator | Draft-7 has complex rules: references, dependencies, pattern validation |
| Subprocess spawning | Manual fork+exec+pipe | popen or posix_spawn | Edge cases in signal handling, pipe buffering, process lifecycle |
| Progress token tracking | Custom token management | Extract from request._meta | Spec defines exact location; follow it exactly |
| Request routing | Manual if/else chains | unordered_map dispatch | O(1) lookup, extensible, cleaner code |

**Key insight:** JSON Schema Draft-7 validation has many edge cases (pattern validation, references, dependencies, numeric ranges). The nlohmann validator is 100x faster than v1 because it compiles schemas to C++ objects. Hand-rolling this would be a significant undertaking.

## Common Pitfalls

### Pitfall 1: Tool Result Error Handling
**What goes wrong:** Tool execution errors are thrown as exceptions or returned as JSON-RPC errors
**Why it happens:** Confusion about where errors should be reported
**How to avoid:** Tool errors MUST return ToolResult with isError=true, not JSON-RPC errors. JSON-RPC errors are only for protocol-level issues.
**Warning signs:** LLM cannot see tool execution errors because they were sent as protocol errors

### Pitfall 2: Progress Token Location
**What goes wrong:** Progress token is not found because code looks in wrong location
**Why it happens:** Progress token is nested in _meta object, not at request root
**How to avoid:** Always check `request.params._meta.progressToken` (from RequestParams interface)
**Warning signs:** Progress notifications never reach client

### Pitfall 3: JSON Schema Validation Performance
**What goes wrong:** Schema is re-parsed on every validation call
**Why it happens:** Using old nlohmann/json-schema-validator v1 API or not compiling schema
**How to avoid:** Compile schema once during tool registration, reuse validator instance
**Warning signs:** Tool calls are slow, validation dominates CPU time

### Pitfall 4: Stdio Message Framing
**What goes wrong:** Messages are not properly delimited, causing parser failures
**Why it happens:** MCP stdio requires newline-delimited JSON, not raw JSON
**How to avoid:** Always append '\n' after JSON serialization when writing
**Warning signs:** Client/server cannot parse messages, hangs waiting for complete message

### Pitfall 5: Subprocess Zombie Processes
**What goes wrong:** Child processes become zombies after parent exits
**Why it happens:** Not properly waiting for child or not killing on cleanup
**How to avoid:** RAII pattern with SIGTERM + waitpid in destructor
**Warning signs:** `ps aux` shows defunct/zombie processes

### Pitfall 6: Resource Content Type Confusion
**What goes wrong:** Returning text content for binary resources or vice versa
**Why it happens:** TextResourceContents vs BlobResourceContents confusion
**How to avoid:** Use TextResourceContents.text for text, BlobResourceContents.blob (base64) for binary
**Warning signs:** Client displays binary as garbled text or text as base64 string

## Code Examples

Verified patterns from official sources:

### MCP Tool Schema Definition
```cpp
// Source: MCP 2025-11-25 schema.ts - Tool interface
namespace mcpp::protocol {

// Tool definition matching spec
struct Tool {
    std::string name;        // Required
    std::string description; // Optional
    json input_schema;       // Required - JSON Schema object

    // Optional execution properties
    struct ToolExecution {
        std::string task_support = "forbidden";  // "forbidden" | "optional" | "required"
    };
    std::optional<ToolExecution> execution;

    // Optional output schema for structured results
    std::optional<json> output_schema;
};

// Tool result matching spec
struct CallToolResult {
    std::vector<ContentBlock> content;  // Unstructured content
    std::optional<json> structured_content;  // Optional structured result
    bool is_error = false;  // Error flag (NOT a protocol error)
};

} // namespace mcpp::protocol
```

### Progress Notification
```cpp
// Source: MCP 2025-11-25 schema.ts - ProgressNotification
namespace mcpp::protocol {

struct ProgressNotificationParams {
    std::string progress_token;  // From request._meta.progressToken
    double progress;             // 0-100, should increase
    std::optional<double> total; // Optional total
    std::optional<std::string> message;  // Optional status message
};

// Build and send progress notification
void RequestContext::report_progress(
    const std::string& token,
    double progress,
    const std::string& message
) {
    json notification = {
        {"jsonrpc", "2.0"},
        {"method", "notifications/progress"},
        {"params", {
            {"progressToken", token},
            {"progress", progress}
        }}
    };

    if (!message.empty()) {
        notification["params"]["message"] = message;
    }

    std::string serialized = notification.dump() + "\n";
    transport_.send(serialized);
}

} // namespace mcpp::protocol
```

### Resource Read Handler
```cpp
// Source: MCP 2025-11-25 schema.ts - Resource types
namespace mcpp::server {

struct ResourceContent {
    std::string uri;
    std::optional<std::string> mime_type;

    bool is_text = true;
    std::string text;      // For text content
    std::string blob;      // For binary content (base64)
};

// Convert to MCP response format
json read_resource_to_json(const ResourceContent& content) {
    json result = {
        {"contents", json::array()}
    };

    json item = {{"uri", content.uri}};
    if (content.mime_type) {
        item["mimeType"] = *content.mime_type;
    }

    if (content.is_text) {
        item["type"] = "resource";
        item["text"] = content.text;
    } else {
        item["type"] = "resource";
        item["blob"] = content.blob;
    }

    result["contents"].push_back(item);
    return result;
}

} // namespace mcpp::server
```

### Prompt Handler with Argument Substitution
```cpp
// Source: MCP 2025-11-25 schema.ts - PromptMessage
namespace mcpp::server {

struct PromptMessage {
    std::string role;  // "user" or "assistant"
    ContentBlock content;
};

// Prompt handler returns array of messages
std::vector<PromptMessage> McpServer::get_prompt(
    const std::string& name,
    const json& arguments
) {
    auto it = prompts_.find(name);
    if (it == prompts_.end()) {
        throw std::runtime_error("Prompt not found: " + name);
    }

    // Substitute arguments into template
    // Handler returns completed message array
    return it->second.handler(name, arguments);
}

} // namespace mcpp::server
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| JSON Schema v4 | JSON Schema Draft-7 | ~2018+ | More powerful validation, recursive refs |
| Manual schema parsing | Compiled validators | nlohmann v2.0 (2020) | 100x validation speed improvement |
| Exception-based errors | isError flag pattern | MCP 2025 spec | LLM can see and self-correct from tool errors |
| No progress support | _meta.progressToken | MCP 2024+ | Long-running tools can provide updates |
| ad-hoc subprocess | popen/posix_spawn | POSIX standard | Reliable process lifecycle management |

**Deprecated/outdated:**
- **JSON Schema v4:** Draft-7 is current; newer drafts (2019-09, 2020-12) exist but Draft-7 is widely supported
- **Exception-based tool errors:** isError flag in result is the MCP standard; exceptions should be caught and converted
- **Custom transport framing:** Newline-delimited JSON is specified for stdio; don't invent new framing

## Open Questions

Things that couldn't be fully resolved:

1. **JSON Schema Version for MCP**
   - What we know: MCP spec uses JSON Schema but doesn't specify which draft
   - What's unclear: Whether Draft-7, 2019-09, or 2020-12 is required
   - Recommendation: Start with Draft-7 (widest support), nlohmann validator supports Draft-7; can extend if needed

2. **Stdio Buffer Size**
   - What we know: Messages are newline-delimited JSON
   - What's unclear: Optimal buffer size for reading stdin
   - Recommendation: Start with 4KB or 8KB buffer, use line-by-line reading with std::getline or custom buffering

3. **Progress Update Frequency**
   - What we know: Progress notifications can be sent at any time during operation
   - What's unclear: Recommended frequency to avoid spam
   - Recommendation: Update every 1-5% or every 100ms for long operations, whichever is less frequent

4. **Subprocess Signal Handling**
   - What we know: Need to clean up subprocess on transport destruction
   - What's unclear: Best signal to use (SIGTERM vs SIGKILL)
   - Recommendation: Send SIGTERM first, wait up to 1 second, then SIGKILL if still running

## Sources

### Primary (HIGH confidence)
- [MCP Schema 2025-11-25 (TypeScript)](https://raw.githubusercontent.com/modelcontextprotocol/specification/main/schema/2025-11-25/schema.ts) - Complete protocol definition, all message types, tool/resource/prompt interfaces, progress notification format
- [nlohmann/json-schema-validator GitHub](https://github.com/pboettch/json-schema-validator) - JSON Schema validation library, Draft-7 support, API documentation
- [nlohmann/json GitHub](https://github.com/nlohmann/json) - JSON library documentation, CMake integration
- [gopher-mcp builders.h](/home/kotdath/omp/personal/cpp/mcpp/thirdparty/gopher-mcp/include/mcp/builders.h) - Reference implementation for handler registration patterns

### Secondary (MEDIUM confidence)
- [Local MCP Development with C++ and Gemini CLI](https://xbill999.medium.com/local-mcp-development-with-c-and-gemini-cli-5c9def0a9752) - C++ MCP server implementation patterns
- [Inside an MCP Server: What Actually Happens](https://bhavyansh001.medium.com/inside-an-mcp-server-what-actually-happens-when-ai-calls-mcp-deepdive-04-98e8e9f11487) - Tool execution flow, validation patterns
- [3 MCP features you probably didn't know about](https://www.reddit.com/r/mcp/comments/1piy754/3_mcp_features_you_probably_didnt_know_about/) - Progress notification usage patterns
- [Subprocess Spawning Best Practices](https://chriswarrick.com/blog/2017/09/02/spawning-subprocesses-smartly-and-securely/) - fork/exec/popen patterns

### Tertiary (LOW confidence)
- [Example: communication with a subprocess via stdin/stdout](https://gist.github.com/konstantint/d49ab683b978b3d74172) - Code example for subprocess stdio (unverified)
- [How To Set Up a Model Context Protocol Server](https://thenewstack.io/how-to-set-up-a-model-context-protocol-server/) - General MCP server setup (language-agnostic)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - nlohmann/json is de facto standard; json-schema-validator is official addon; MCP schema is authoritative
- Architecture: HIGH - Based on official MCP schema and proven gopher-mcp patterns; CONTEXT.md decisions are locked
- Pitfalls: MEDIUM - Progress token location and error handling verified in spec; subprocess patterns based on general POSIX knowledge

**Research date:** 2026-01-31
**Valid until:** 2026-03-02 (30 days - MCP spec is stable but check for updates)
