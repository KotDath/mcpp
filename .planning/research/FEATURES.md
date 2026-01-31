# Feature Research

**Domain:** MCP (Model Context Protocol) C++ Library
**Researched:** 2026-01-31
**Confidence:** HIGH

## Feature Landscape

### Table Stakes (Users Expect These)

Features users expect from an MCP library. Missing = product feels incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **JSON-RPC 2.0 Core** | MCP is built on JSON-RPC; basic request/response is foundational | HIGH | Must handle id mapping, batch requests (optional), error codes, notifications |
| **Initialization Handshake** | Protocol lifecycle requires initialize/initialized exchange | MEDIUM | Protocol version negotiation, capability exchange, client/server info |
| **Capability Declaration** | Both sides declare supported features for compatibility | MEDIUM | Structured capability objects with sub-capabilities (listChanged, subscribe, etc.) |
| **Tools (list/call)** | Core server primitive - executable functions for LLMs | HIGH | Tool discovery, JSON Schema input validation, structured/unstructured results |
| **Resources (list/read)** | Core server primitive - data sources for context | HIGH | URI-based resources, text/blob content, MIME type handling |
| **Prompts (list/get)** | Core server primitive - reusable templates | MEDIUM | Prompt discovery, argument completion, message templates |
| **stdio Transport** | Standard for local MCP servers | MEDIUM | Subprocess spawning, stdin/out messaging, newline-delimited JSON |
| **Progress Tokens** | Required for long-running operations | MEDIUM | Opaque token attachment, progress notification emission |
| **Cancellation** | Required for user-initiated request termination | MEDIUM | notifications/cancelled, race condition handling |
| **Logging** | Server-to-client log messages | LOW | Logging levels (debug/info/warn/error), structured messages |
| **Pagination** | Required for large lists (tools/resources/prompts) | MEDIUM | Cursor-based pagination, nextCursor handling |
| **List Changed Notifications** | Dynamic capability discovery | LOW | notifications/tools/list_changed, prompts/list_changed, resources/list_changed |
| **Resource Subscriptions** | Monitor individual resource changes | MEDIUM | resources/subscribe, resources/unsubscribe, notifications/resources/updated |
| **Content Types** | Rich content in tool results and resources | MEDIUM | Text, image, audio, resource links, embedded resources, annotations |
| **Error Handling** | Protocol-level and execution errors | HIGH | JSON-RPC errors, Tool execution errors (isError flag), detailed error data |
| **Roots (client)** | Client exposes filesystem boundaries to server | LOW | roots/list, roots/list_changed notification, file:// URIs |
| **Sampling (client)** | Server requests LLM completion through client | HIGH | sampling/createMessage, model preferences, tool use in sampling |
| **Elicitation (client)** | Server requests user input through client | HIGH | Form mode (JSON Schema), URL mode (secure out-of-band), response actions |
| **Streamable HTTP Transport** | Required for remote servers | VERY HIGH | HTTP POST/SSE, session management, resumability, protocol version header |

### Differentiators (Competitive Advantage)

Features that set the product apart. Not required, but valuable.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Tasks (Experimental)** | Durable execution wrapper for expensive computations | VERY HIGH | Task lifecycle, polling, status notifications, result retrieval, TTL management |
| **Completion (auto-completion)** | Argument completion for prompts/resources | MEDIUM | Complete API with reference values, improves DX |
| **Resource Templates** | Parameterized resources with URI templates | MEDIUM | URI template expansion, dynamic resource discovery |
| **Structured Output** | Tools return validated structured JSON | MEDIUM | outputSchema, JSON Schema validation, better type safety |
| **Tool Annotations** | Metadata about tool behavior (audience, priority, etc.) | LOW | Execution hints, destructive/read-only indicators |
| **Progress Reset on Notification** | Reset timeout clock on progress updates | LOW | Improves long-running operation reliability |
| **Experimental Capabilities** | Custom feature negotiation for extensions | MEDIUM | experimental field in capabilities, vendor-specific extensions |
| **Layered API Design** | Low-level core + high-level wrappers | MEDIUM | Ergonomic API without sacrificing control, differentiates from C# SDK |
| **Zero-Copy Abstractions** | Minimize memory copies for performance | HIGH | View types, borrowed data parsing, significant throughput benefit |
| **Header-Only Option** | For library users who prefer header-only | MEDIUM | Compile-time tradeoff, distribution convenience |
| **Custom Transport Support** | Pluggable transport architecture | HIGH | WebSocket, custom IPC, extension points for transport implementers |
| **MCP Inspector Integration** | Built-in testing and validation | MEDIUM | Test helpers, inspector connection utilities |
| **Tool Result Streaming** | Incremental results for long tool calls | HIGH | Streaming content blocks, progressive response rendering |

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem good but create problems.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **Synchronous Blocking API** | Simpler to use, easier to understand | Blocks event loop, poor performance, cannot handle progress/cancellation properly | Async/callback-based API with optional sync wrappers using futures |
| **Built-in HTTP Server** | Library users want drop-in server capability | Out of scope, bloats library, users have different server preferences | Users bring their own HTTP server; library provides Streamable HTTP client |
| **Custom JSON Schema Validation** | Want to avoid external dependency | Reinventing wheel, maintenance burden, edge cases | Use existing JSON Schema library (e.g., nlohmann/json with schema extension) |
| **Global State** | Simpler initialization, easier singleton pattern | Thread-unsafe, prevents multiple connections, testing issues | Per-connection state, explicit context passing |
| **Auto-Reconnection** | Transparent handling of network failures | Hides errors, makes behavior unpredictable, violates explicit is better than implicit | Expose reconnection primitives, let users decide retry policy |
| **Implicit Threading** | Easier async handling | Thread-safety complexity, hidden performance costs, integration difficulties | Explicit threading model, user controls thread pool |
| **Built-in LLM Integration** | "Make it easier to use sampling directly" | Out of scope, ties library to specific providers, bloat | Library provides MCP protocol only; users bring their own LLM client |
| **Blocking stdio reading** | Simpler subprocess spawning | Deadlocks on stderr, poor error handling | Non-blocking I/O, separate stderr handling thread |
| **JSON-C or json.h** | Lighter than nlohmann/json | C++ has move semantics, RAII; C libraries are error-prone | Use modern C++ JSON library (nlohmann/json is standard) |
| **Dynamic schema negotiation** | "Future-proof" capability discovery | Violates explicit contract, complex to implement correctly, rare use case | Stick to 2025-11-25 spec, version mismatch = disconnect |

## Feature Dependencies

```
[JSON-RPC Core]
    └──requires──> [UTF-8 Encoding]
    └──requires──> [Transport Abstraction]

[Initialization Handshake]
    └──requires──> [JSON-RPC Core]
    └──requires──> [Capability Declaration]

[Capability Declaration]
    └──enhances──> [Tools/Resources/Prompts] (declares which are supported)

[stdio Transport]
    └──implements──> [Transport Abstraction]

[Streamable HTTP Transport]
    └──implements──> [Transport Abstraction]
    └──requires──> [Session Management]
    └──requires──> [SSE Parsing]

[Tools - List/Call]
    └──requires──> [Initialization Handshake]
    └──requires──> [JSON Schema Validation] (for inputSchema)

[Resources - List/Read]
    └──requires──> [Initialization Handshake]
    └──requires──> [URI Parsing]

[Resource Subscriptions]
    └──requires──> [Resources - List/Read]
    └──requires──> [Resource Update Notifications]

[Prompts - List/Get]
    └──requires──> [Initialization Handshake]

[Progress Tokens]
    └──requires──> [Request Metadata] (_meta field)
    └──enables──> [Progress Notifications]

[Cancellation]
    └──requires──> [Request Tracking] (to find in-flight requests)

[Sampling (Client)]
    └──requires──> [Initialization Handshake]
    └──requires──> [Capability Negotiation] (sampling capability)
    └──requires──> [Tool Use Support] (for tools in sampling)

[Elicitation (Client)]
    └──requires──> [Initialization Handshake]
    └──requires──> [Capability Negotiation] (elicitation capability)
    └──requires──> [JSON Schema Validation] (for requestedSchema)

[Tasks]
    └──requires──> [Initialization Handshake]
    └──requires──> [Capability Negotiation] (tasks capability)
    └──requires──> [Task Lifecycle Management]
    └──enables──> [Task Polling] (tasks/get)
    └──enables──> [Task Result Retrieval] (tasks/result)
    └──enables──> [Task Cancellation] (tasks/cancel)

[Completion]
    └──requires──> [Resource/Prompt Templates]

[Resource Templates]
    └──requires──> [URI Template Expansion]
    └──enhances──> [Completion] (completion for template parameters)

[Structured Output]
    └──requires──> [JSON Schema Validation]
    └──enhances──> [Tools] (outputSchema)

[List Changed Notifications]
    └──requires──> [Notification Emission]
    └──enhances──> [Tools/Resources/Prompts]
```

## MVP Definition

### Launch With (v1)

Minimum viable product - what's needed to validate the concept and be useful.

- [ ] **JSON-RPC 2.0 Core** - Foundation of all communication
- [ ] **Initialization Handshake** - Protocol lifecycle, capability negotiation
- [ ] **stdio Transport** - Essential for local MCP servers
- [ ] **Tools (list/call)** - Core server primitive, most common use case
- [ ] **Resources (list/read)** - Core server primitive for context data
- [ ] **Prompts (list/get)** - Core server primitive for templates
- [ ] **Progress Tokens** - Required for long-running tools
- [ ] **Cancellation** - Required for user control
- [ ] **Error Handling** - Protocol and execution errors
- [ ] **Basic Content Types** - Text, image, resource content
- [ ] **Roots (client)** - Filesystem boundaries
- [ ] **Sampling (client)** - LLM sampling without tool use
- [ ] **Elicitation Form Mode** - Structured user input

### Add After Validation (v1.x)

Features to add once core is working and validated by real users.

- [ ] **Streamable HTTP Transport** - Remote server support, SSE parsing
- [ ] **Resource Subscriptions** - Change notifications for individual resources
- [ ] **Resource Templates** - Parameterized dynamic resources
- [ ] **Elicitation URL Mode** - Secure out-of-band interactions
- [ ] **Sampling with Tool Use** - Agentic behaviors with tool loops
- [ ] **Pagination** - For large lists (may defer if most use cases have small lists)
- [ ] **List Changed Notifications** - Dynamic capability updates
- [ ] **Tool Annotations** - Enhanced metadata
- [ ] **Structured Output** - outputSchema for tools

### Future Consideration (v2+)

Features to defer until product-market fit is established.

- [ ] **Tasks** - Experimental, complex, adds significant state management
- [ ] **Completion** - Nice to have, not critical for basic functionality
- [ ] **Tool Result Streaming** - Advanced feature, adds complexity
- [ ] **Custom Transport Support** - Extension point, can be added later
- [ ] **WebSocket Transport** - Alternative to SSE for bi-directional streaming
- [ ] **Experimental Capabilities** - For protocol extensions

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| JSON-RPC Core | HIGH | HIGH | P1 |
| Initialization Handshake | HIGH | MEDIUM | P1 |
| stdio Transport | HIGH | MEDIUM | P1 |
| Tools (list/call) | HIGH | HIGH | P1 |
| Resources (list/read) | HIGH | MEDIUM | P1 |
| Prompts (list/get) | MEDIUM | MEDIUM | P1 |
| Progress Tokens | MEDIUM | MEDIUM | P1 |
| Cancellation | MEDIUM | MEDIUM | P1 |
| Error Handling | HIGH | MEDIUM | P1 |
| Basic Content Types | MEDIUM | MEDIUM | P1 |
| Roots (client) | MEDIUM | LOW | P1 |
| Sampling (client, basic) | HIGH | HIGH | P1 |
| Elicitation Form Mode | MEDIUM | MEDIUM | P1 |
| Streamable HTTP Transport | HIGH | VERY HIGH | P2 |
| Resource Subscriptions | MEDIUM | MEDIUM | P2 |
| Resource Templates | MEDIUM | MEDIUM | P2 |
| Elicitation URL Mode | LOW | MEDIUM | P2 |
| Sampling with Tool Use | MEDIUM | HIGH | P2 |
| Pagination | LOW | MEDIUM | P2 |
| List Changed Notifications | LOW | LOW | P2 |
| Tool Annotations | LOW | LOW | P2 |
| Structured Output | MEDIUM | MEDIUM | P2 |
| Tasks | MEDIUM | VERY HIGH | P3 |
| Completion | LOW | MEDIUM | P3 |
| Tool Result Streaming | LOW | HIGH | P3 |
| Custom Transport Support | LOW | HIGH | P3 |
| WebSocket Transport | LOW | HIGH | P3 |
| Experimental Capabilities | LOW | MEDIUM | P3 |

**Priority key:**
- P1: Must have for launch (v1)
- P2: Should have, add when possible (v1.x)
- P3: Nice to have, future consideration (v2+)

## Competitor Feature Analysis

| Feature | Rust SDK | Python SDK | C# SDK | Java SDK | Our Approach (C++) |
|---------|-----------|------------|--------|---------|-------------------|
| Transport support | stdio, SSE, WebSocket | stdio, SSE | stdio, SSE | stdio, SSE | stdio, SSE (WebSocket v2+) |
| Async model | Async/await, Tokio | Async/await | Tasks, async/await | CompletableFuture, Project Loom | Callbacks + optional futures |
| JSON library | serde_json | orjson/json | System.Text.Json | Jackson/Gson | nlohmann/json |
| Thread safety | Yes (Arc/Mutex) | Limited (GIL) | Yes | Yes | Yes (planned from start) |
| Layered API | Some | Good | Limited | Limited | Two-tier: low-level core + high-level wrappers |
| Header-only option | No | No | No | No | Consider separate header-only build |
| Zero-copy parsing | Yes | Limited | No | No | Yes (string_views, borrowed data) |
| Tasks (experimental) | Yes | Not yet | Not yet | Not yet | Defer to v2 |

**Key differentiator:** Rust SDK is most complete but has compile times and learning curve. Python SDK has GIL issues. C# SDK tied to .NET. Our C++ library can offer: performance, zero-copy parsing, explicit memory management, no runtime GC, broad platform support.

## Sources

- HIGH confidence: MCP 2025-11-25 specification (official schema and documentation)
  - /thirdparty/modelcontextprotocol/schema/2025-11-25/schema.json
  - /thirdparty/modelcontextprotocol/docs/specification/2025-11-25/
  - https://modelcontextprotocol.io/specification/latest
- HIGH confidence: Architecture overview and concepts
  - /thirdparty/modelcontextprotocol/docs/docs/learn/architecture.mdx
  - /thirdparty/modelcontextprotocol/docs/docs/learn/server-concepts.mdx
  - /thirdparty/modelcontextprotocol/docs/docs/learn/client-concepts.mdx
- MEDIUM confidence: Rust SDK reference implementation
  - /thirdparty/rust-sdk/ (for patterns, not definitive for C++ design)

---
*Feature research for: MCP (Model Context Protocol) C++ Library*
*Researched: 2026-01-31*
