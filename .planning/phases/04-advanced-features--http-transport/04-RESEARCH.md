# Phase 04: Advanced Features & HTTP Transport - Research

**Researched:** 2026-01-31
**Domain:** MCP advanced features (resource templates, subscriptions, completions, tool annotations, structured output, streaming) + Streamable HTTP transport (HTTP POST/SSE)
**Confidence:** MEDIUM

## Summary

This phase covers advanced MCP server features and the Streamable HTTP transport. The research identified:

1. **Streamable HTTP Transport**: MCP defines a new "Streamable HTTP" transport (2025-06-18 spec) that replaces the older "HTTP+SSE" transport. It uses a single endpoint supporting both POST (client->server) and GET (server->client via SSE), with session management via `Mcp-Session-Id` header.

2. **Resource Templates**: Use RFC 6570 URI template expansion for parameterized resources. C++ libraries exist ([uritemplate-cpp](https://github.com/returnzero23/uritemplate-cpp), Google APIs Client Library).

3. **Resource Subscriptions**: Subscribe/unsubscribe pattern with `notifications/resources/updated` notifications when monitored resources change.

4. **Completion**: Argument autocompletion for prompts and resources with reference values, similar to IDE autocomplete.

5. **Tool Annotations**: Metadata including `audience`, `priority`, `destructive` indicators for tools.

6. **Structured Output**: Tools can specify `outputSchema` (JSON Schema) for validating result content.

7. **Tool Result Streaming**: Incremental results for long-running tools via streaming mechanism.

**Primary recommendation**: Use user-provided HTTP server integration pattern rather than building HTTP server directly. For SSE, implement protocol-compliant streaming as a Transport subclass. For URI templates, consider inline RFC 6570 Level 1-2 implementation rather than external dependency. JSON Schema validation already available via nlohmann/json-schema-validator.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| nlohmann/json | 3.11.0+ | JSON parsing (already in use) | Already a dependency |
| nlohmann/json-schema-validator | latest | JSON Schema validation (outputSchema) | Already in gopher-mcp build deps |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| cpp-httplib | latest | HTTP server (optional, user brings their own) | If building examples/tests with HTTP |
| asio | standalone | Async I/O (optional) | For non-blocking I/O patterns |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Built-in HTTP server | User-provided HTTP server | Library doesn't own HTTP server lifecycle; users bring their own (per REQUIREMENTS.md out-of-scope) |
| uritemplate-cpp | Inline RFC 6570 impl | External dependency vs simple inline implementation (only Level 1-2 needed) |
| Full SSE library | Manual SSE implementation | SSE protocol is simple (text/event-stream formatting); external library unnecessary |

**Installation:**
```bash
# JSON schema validator already available via gopher-mcp build
# If needed for standalone:
find_package(nlohmann_json 3.11.0 REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
```

### For HTTP Server (User Provided)
Per REQUIREMENTS.md, built-in HTTP server is **out of scope**. Users bring their own HTTP server. The library provides:
- `HttpTransport` class implementing `Transport` interface
- User integrates with their HTTP server (e.g., nginx, Apache, custom server)

## Architecture Patterns

### Recommended Project Structure
```
src/mcpp/
├── transport/
│   ├── transport.h           # Base interface (existing)
│   ├── stdio_transport.h     # stdio transport (existing)
│   └── http_transport.h      # NEW: HTTP/SSE transport
├── server/
│   ├── tool_registry.h       # Enhanced with streaming, annotations, outputSchema
│   ├── resource_registry.h   # Enhanced with templates, subscriptions
│   ├── prompt_registry.h     # Enhanced with completion support
│   └── mcp_server.h          # Enhanced with new handlers
├── protocol/
│   ├── types.h               # Enhanced with new types
│   └── capabilities.h        # Enhanced with advanced capabilities
└── util/
    ├── uri_template.h        # NEW: RFC 6570 template expansion
    └── sse_formatter.h       # NEW: SSE event formatting
```

### Pattern 1: Streamable HTTP Transport
**What:** Transport implementation supporting MCP Streamable HTTP specification
**When to use:** When server needs HTTP/SSE transport (stdio alternative)

Key design decisions:
- **Single endpoint design**: POST for client messages, GET for SSE stream
- **Session management**: `Mcp-Session-Id` header for session tracking
- **SSE formatting**: Proper `text/event-stream` formatting with `data:`, `event:`, `id:` fields
- **Resumability**: `Last-Event-ID` header support for reconnection
- **User-provided HTTP server**: Library integrates with existing HTTP servers

Example interface:
```cpp
// User provides HTTP server integration
class HttpTransport : public Transport {
public:
    // Called by user's HTTP server when POST received
    void handle_post_request(const std::string& body, HttpResponse& response);

    // Called by user's HTTP server when GET received (SSE)
    void handle_get_request(HttpSseWriter& writer);

    // Send notification via SSE
    void send_notification(const nlohmann::json& notification);

    // Session management
    std::string create_session();
    bool validate_session(const std::string& session_id);
    void terminate_session(const std::string& session_id);
};
```

### Pattern 2: Resource Templates with URI Expansion
**What:** RFC 6570 URI template expansion for parameterized resources
**When to use:** When resources need parameters (e.g., `file://{path}`)

Inline implementation for Level 1-2 templates:
```cpp
// Simple RFC 6570 Level 1-2 template expansion
// Supports: {var}, {var*}, {?var*}
class UriTemplate {
public:
    static std::string expand(const std::string& template_str,
                             const nlohmann::json& params);

    // Example:
    // expand("file://{path}", {{"path": "/etc/config"}})
    //   -> "file:///etc/config"
    //
    // expand("http://example.com/api{?params*}", {{"params": {"a":1,"b":2}}})
    //   -> "http://example.com/api?a=1&b=2"
};
```

### Pattern 3: Resource Subscriptions
**What:** Subscribe/unsubscribe pattern with change notifications
**When to use:** When clients need real-time updates for monitored resources

```cpp
class ResourceRegistry {
    // Existing methods...

    // NEW: Subscription management
    bool subscribe(const std::string& uri, const std::string& subscriber_id);
    bool unsubscribe(const std::string& uri, const std::string& subscriber_id);

    // NEW: Notification dispatch
    void notify_updated(const std::string& uri);

    // NEW: Completion handler
    struct CompletionHandler {
        using CompletionCallback = std::function<std::vector<Completion>(
            const std::string& name,
            const std::string& argument_name,
            const nlohmann::json& current_value
        )>;
    };
};
```

### Pattern 4: Tool Annotations and Structured Output
**What:** Metadata and output schema for tools
**When to use:** For rich tool descriptions and result validation

```cpp
struct ToolRegistration {
    std::string name;
    std::string description;
    nlohmann::json input_schema;

    // NEW: Optional output schema
    std::optional<nlohmann::json> output_schema;
    std::unique_ptr<nlohmann::json_schema::json_validator> output_validator;

    // NEW: Annotations
    struct Annotations {
        bool destructive = false;           // Indicates destructive operation
        bool read_only = true;              // Indicates read-only operation
        std::string audience = "user";      // "user" | "assistant" | "system"
        int priority = 0;                   // Lower = higher priority
    } annotations;

    ToolHandler handler;
};
```

### Pattern 5: Tool Result Streaming
**What:** Incremental results for long-running tools
**When to use:** For tools that generate results over time

```cpp
class RequestContext {
    // Existing progress support...

    // NEW: Streaming support
    void send_stream_result(const nlohmann::json& partial_result);

    // Handler uses:
    // ctx.send_stream_result({{"content", {{{"type", "text"}, {"text", "partial"}}}}});
    // Final result marks completion
};
```

### Anti-Patterns to Avoid
- **Blocking event loop**: Don't use synchronous I/O in transport send/receive. Use async/non-blocking patterns.
- **Global HTTP server state**: Don't manage HTTP server lifecycle in library. User owns the server.
- **Complex URI template library**: Don't add heavy dependency for simple template expansion. Inline Level 1-2 is sufficient.
- **Building full SSE library**: SSE format is simple text formatting. Don't over-engineer.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| JSON Schema validation | Custom validator | nlohmann/json-schema-validator | Already in build deps; handles Draft 7; edge cases like $ref, pattern validation |
| HTTP protocol handling | Raw socket HTTP | User's HTTP server | Out of scope per requirements; users bring their own server |
| SSE parsing/generation | Custom SSE parser | Simple inline formatting | SSE is just `data: JSON\n\n` - no complex parsing needed |

**Key insight:** JSON Schema validation is the only "don't hand-roll" item that requires an external library. HTTP server and SSE are either user-provided or simple enough to inline.

## Common Pitfalls

### Pitfall 1: Blocking I/O in Transport
**What goes wrong:** Synchronous socket I/O blocks the event loop, preventing concurrent request handling.
**Why it happens:** Using blocking read/write on sockets instead of async/non-blocking patterns.
**How to avoid:** Use non-blocking file descriptors (O_NONBLOCK) or async I/O. The existing StdioTransport already has this pattern - follow it.
**Warning signs:** `read()`/`write()` calls without timeout, single-threaded handling of multiple connections.

### Pitfall 2: Session ID Security Weakness
**What goes wrong:** Predictable or insecure session IDs enable session hijacking.
**Why it happens:** Using simple counters or non-cryptographic random generation.
**How to avoid:** Use cryptographically secure UUID generation (RFC 4122) for session IDs. Only contain ASCII 0x21-0x7E.
**Warning signs:** Sequential session IDs, short session IDs (<16 bytes), non-random IDs.

### Pitfall 3: SSE Protocol Violations
**What goes wrong:** Clients fail to parse SSE events due to incorrect formatting.
**Why it happens:** Missing newlines, incorrect field order, missing `data:` prefix.
**How to avoid:** Follow SSE spec exactly:
```
data: {"json":"rpc"}
event: message
id: 123

```
**Warning signs:** Clients not receiving events, parse errors, connection drops.

### Pitfall 4: URI Template Injection
**What goes wrong:** User input in template parameters causes incorrect URIs.
**Why it happens:** Not escaping special characters in URI components.
**How to avoid:** Percent-encode reserved characters in path/query components. RFC 3986 encoding.
**Warning signs:** Spaces in URIs, special characters breaking parsing.

### Pitfall 5: Memory Leak in Subscriptions
**What goes wrong:** Subscriptions accumulate without cleanup when clients disconnect.
**Why it happens:** No automatic unsubscribe on connection close.
**How to avoid:** Track subscriptions per session/connection; cleanup on disconnect. Use weak references or RAII.
**Warning signs:** Memory growth over time, increasing subscription count.

### Pitfall 6: Output Schema Not Validated
**What goes wrong:** Tools claim to follow outputSchema but return invalid data.
**Why it happens:** Registering output_schema but not validating actual results.
**How to avoid:** Compile validator for output_schema, validate results before returning.
**Warning signs:** Client errors parsing tool results, schema validation errors in logs.

## Code Examples

Verified patterns from official sources:

### SSE Event Formatting
```cpp
// Source: MCP Transports Specification
// https://modelcontextprotocol.io/specification/2025-06-18/basic/transports

class SseFormatter {
public:
    // Format a JSON-RPC message as SSE event
    static std::string format_event(const nlohmann::json& message,
                                     const std::string& event_id = "") {
        std::ostringstream oss;
        oss << "data: " << message.dump() << "\n";
        if (!event_id.empty()) {
            oss << "id: " << event_id << "\n";
        }
        oss << "\n";  // Double newline ends event
        return oss.str();
    }

    // SSE response headers (user sets these)
    static const char* content_type() { return "text/event-stream"; }
    static const char* cache_control() { return "no-cache"; }
    static const char* connection() { return "keep-alive"; }
};
```

### Resource Template Handler
```cpp
// Source: MCP Resources Specification
// https://modelcontextprotocol.io/specification/2025-11-25/server/resources

class ResourceRegistry {
public:
    // Register a templated resource
    bool register_template(
        const std::string& uri_template,  // e.g., "file://{path}"
        const std::string& name,
        const std::optional<std::string>& description,
        const std::string& mime_type,
        TemplateResourceHandler handler  // Receives expanded URI
    );

    // List resources includes templates
    std::vector<nlohmann::json> list_resources() const;
};
```

### Tool with Annotations and Output Schema
```cpp
// Source: MCP Tools Specification (2025-06-18)
// https://modelcontextprotocol.io/specification/2025-06-18/server/tools

server.register_tool(
    "delete_file",
    "Delete a file (DESTRUCTIVE)",
    R"({
        "type": "object",
        "properties": {
            "path": {"type": "string"}
        },
        "required": ["path"]
    })"_json,
    R"({
        "type": "object",
        "properties": {
            "success": {"type": "boolean"},
            "message": {"type": "string"}
        },
        "required": ["success"]
    })"_json,
    ToolRegistration::Annotations{
        .destructive = true,
        .read_only = false,
        .audience = "user",
        .priority = 100  // Lower priority for dangerous ops
    },
    [](const std::string& name, const json& args, RequestContext& ctx) {
        // Tool handler
        bool success = delete_file(args["path"]);
        return json{
            {"content", {{{"type", "text"}, {"text", success ? "Deleted" : "Failed"}}}},
            {"isError", !success}
        };
    }
);
```

### Completion Handler
```cpp
// Source: MCP Completion Specification
// https://mcp.transdocs.org/specification/2025-06-18/server/utilities/completion

struct Completion {
    std::string value;           // The completion value
    std::optional<std::string> description;  // Optional description
};

class PromptRegistry {
public:
    // Register completion handler for a prompt
    void set_completion_handler(
        const std::string& prompt_name,
        std::function<std::vector<Completion>(
            const std::string& argument_name,
            const nlohmann::json& current_value
        )> handler
    );
};
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| HTTP+SSE (separate endpoints) | Streamable HTTP (single endpoint) | 2025-06-18 spec | Simplified transport; single endpoint for POST/GET |
| No output validation | outputSchema with JSON Schema validation | 2025-06-18 spec | Structured tool outputs guaranteed valid |
| Static resource URIs | Resource templates with RFC 6570 | 2025-11-25 spec | Parameterized resources possible |
| No tool metadata | Tool annotations (audience, priority, etc.) | 2025-06-18 spec | Rich tool discovery; UI/UX improvements |

**Deprecated/outdated:**
- **HTTP+SSE separate endpoint transport**: Replaced by Streamable HTTP with single endpoint
- **Session management via query params**: Now uses `Mcp-Session-Id` header

## Open Questions

Things that couldn't be fully resolved:

1. **Tool Result Streaming Format**
   - What we know: MCP supports incremental tool results for long-running operations
   - What's unclear: Exact wire format for streaming tool results (multiple `notifications/progress`? Custom event type?)
   - Recommendation: Implement progress-based streaming first; investigate official examples for specific format

2. **URI Template Complexity Needed**
   - What we know: RFC 6570 has 4 levels of complexity
   - What's unclear: What level do MCP servers typically use?
   - Recommendation: Implement Level 1-2 (simple variables, query params); add Level 3-4 if real-world need emerges

3. **Subscription Storage Strategy**
   - What we know: Need to track which client is subscribed to which resource
   - What's unclear: In-memory vs persistent storage; cleanup timing
   - Recommendation: In-memory per-session storage; cleanup on session termination or explicit unsubscribe

4. **HTTP Server Integration Pattern**
   - What we know: Users bring their own HTTP server (out of scope to build one)
   - What's unclear: What API should library expose for integration?
   - Recommendation: Provide `HttpTransport` class with `handle_post()` and `handle_get()` methods users call from their HTTP request handlers

## Sources

### Primary (HIGH confidence)
- [MCP Transports Specification (2025-11-25)](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports) - Streamable HTTP, SSE format, session management
- [MCP Tools Specification (2025-06-18)](https://modelcontextprotocol.io/specification/2025-06-18/server/tools) - Tool annotations, outputSchema
- [MCP Resources Specification (2025-11-25)](https://modelcontextprotocol.io/specification/2025-11-25/server/resources) - Resource templates
- [MCP Completion Specification (2025-06-18)](https://mcp.transdocs.org/specification/2025-06-18/server/utilities/completion) - Argument completion

### Secondary (MEDIUM confidence)
- [RFC 6570 - URI Template](https://datatracker.ietf.org/doc/html/rfc6570) - URI template standard
- [uritemplate-cpp GitHub](https://github.com/returnzero23/uritemplate-cpp) - C++ URI template implementation reference
- [Google APIs C++ Client - UriTemplate](https://google.github.io/google-api-cpp-client/latest/doxygen/classgoogleapis_1_1client_1_1UriTemplate.html) - Production URI template implementation
- [cpp-httplib SSE Documentation](https://zread.ai/yhirose/cpp-httplib/21-server-sent-events-sse) - SSE implementation reference
- [Stack Overflow: SSE C++ Implementation](https://stackoverflow.com/questions/51949072/server-side-event-c-implementation) - SSE implementation discussion

### Tertiary (LOW confidence)
- [Auth0: MCP Streamable HTTP](https://auth0.com/blog/mcp-streamable-http/) - Analysis of transport changes
- [Blog: MCP Resource Templates](https://www.linnify.com/resources/model-context-protocol-mcp-complete-2025-guide) - Resource template overview
- [Blog: MCP Completion](https://blog.fka.dev/blog/2025-06-11-diving-into-mcp-advanced-server-capabilities/) - Advanced features overview

## Metadata

**Confidence breakdown:**
- Standard stack: MEDIUM - nlohmann/json well-established; JSON schema validator in existing deps
- Architecture: MEDIUM - Transport pattern follows existing StdioTransport; new features have spec definitions
- Pitfalls: HIGH - Non-blocking I/O, SSE formatting, URI encoding are well-understood problems

**Research date:** 2026-01-31
**Valid until:** 2026-03-01 (60 days - MCP spec evolving, but core patterns stable)
