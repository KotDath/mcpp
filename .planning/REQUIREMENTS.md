# Requirements: mcpp

**Defined:** 2025-01-31
**Core Value:** Developers can build MCP clients and servers in C++ that are fast, correct, and support the complete protocol spec without wrestling with JSON-RPC details or transport plumbing

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Protocol Foundation

- [ ] **PROTO-01**: JSON-RPC 2.0 Core implementation (request/response, id mapping, error codes, notifications)
- [ ] **PROTO-02**: Initialization handshake (initialize/initialized exchange, protocol version negotiation)
- [ ] **PROTO-03**: Capability declaration (structured capability objects with sub-capabilities)
- [ ] **PROTO-04**: Error handling (JSON-RPC errors, tool execution errors with isError flag)
- [ ] **PROTO-05**: Request tracking with timeout cleanup

### Core Server Primitives

- [ ] **SRVR-01**: Tools list (tool discovery)
- [ ] **SRVR-02**: Tools call (JSON Schema input validation, structured/unstructured results)
- [ ] **SRVR-03**: Resources list (URI-based discovery, MIME type handling)
- [ ] **SRVR-04**: Resources read (text/blob content, embedded resources)
- [ ] **SRVR-05**: Prompts list (prompt discovery)
- [ ] **SRVR-06**: Prompts get (argument completion, message templates)

### Transports

- [ ] **TRAN-01**: stdio transport (subprocess spawning, stdin/out messaging, newline-delimited JSON)
- [ ] **TRAN-02**: Streamable HTTP transport (HTTP POST/SSE, session management, resumability)
- [ ] **TRAN-03**: Transport abstraction interface (pluggable transport design)
- [ ] **TRAN-04**: Non-blocking I/O for all transports (avoid event loop blocking)

### Async & Lifecycle

- [ ] **ASYNC-01**: Progress tokens (opaque token attachment, progress notification emission)
- [ ] **ASYNC-02**: Cancellation support (notifications/cancelled, race condition handling)
- [ ] **ASYNC-03**: Callback-based core API for async operations
- [ ] **ASYNC-04**: std::future wrappers for ergonomic blocking API
- [ ] **ASYNC-05**: Proper async lifetime management (no dangling references)

### Client Capabilities

- [ ] **CLNT-01**: Roots support (roots/list, roots/list_changed notification, file:// URIs)
- [ ] **CLNT-02**: Sampling createMessage (basic LLM completion requests)
- [ ] **CLNT-03**: Sampling with tool use (agentic behaviors with tool loops)
- [ ] **CLNT-04**: Elicitation form mode (structured user input with JSON Schema)
- [ ] **CLNT-05**: Elicitation URL mode (secure out-of-band interactions)

### Content & Metadata

- [ ] **CONT-01**: Rich content types (text, image, audio, resource links, embedded resources)
- [ ] **CONT-02**: Content annotations (metadata on content items)
- [ ] **CONT-03**: Pagination (cursor-based pagination, nextCursor handling)
- [ ] **CONT-04**: List changed notifications (tools/prompts/resources/list_changed)

### Advanced Features

- [ ] **ADV-01**: Tool result streaming (incremental results for long-running tools)
- [ ] **ADV-02**: Resource templates (parameterized resources with URI template expansion)
- [ ] **ADV-03**: Resource subscriptions (resources/subscribe, resources/unsubscribe, notifications/resources/updated)
- [ ] **ADV-04**: Completion (argument completion for prompts/resources with reference values)
- [ ] **ADV-05**: Tool annotations (metadata: audience, priority, destructive/read-only indicators)
- [ ] **ADV-06**: Structured output (outputSchema for tools, JSON Schema validation)
- [ ] **ADV-07**: Tasks experimental (task lifecycle, polling, status notifications, result retrieval, TTL management)

### API Design

- [ ] **API-01**: Low-level core API (callback-based, close to wire protocol, maximum control)
- [ ] **API-02**: High-level wrappers (type-safe, RAII, modern C++ patterns, hides RPC details)
- [ ] **API-03**: Thread-safe API (all public methods safe to call from any thread)

### Utility Features

- [ ] **UTIL-01**: Logging support (debug/info/warn/error levels, structured messages)
- [ ] **UTIL-02**: Progress reset on notification (reset timeout clock on progress updates)
- [ ] **UTIL-03**: Experimental capabilities field (custom feature negotiation for extensions)

### Build & Distribution

- [ ] **BUILD-01**: CMake build system (C++17 standard, gcc-11/g++-11 compatible)
- [ ] **BUILD-02**: Static library build target
- [ ] **BUILD-03**: Shared library build target with proper versioning (SONAME)
- [ ] **BUILD-04**: MIT license headers in all source files

### Testing & Validation

- [ ] **TEST-01**: Google Test integration (unit tests for core functionality)
- [ ] **TEST-02**: MCP Inspector integration (integration validation against real MCP clients)
- [ ] **TEST-03**: JSON-RPC compliance tests (verify protocol correctness)
- [ ] **TEST-04**: Thread safety tests (verify concurrent access patterns)
- [ ] **TEST-05**: Async lifetime tests (verify no dangling references)

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Additional Transports

- **TRAN-V2-01**: WebSocket transport (alternative to SSE for bi-directional streaming)

### API Enhancements

- **API-V2-01**: Zero-copy types (string_views, borrowed data for performance)
- **API-V2-02**: Header-only build option (for users who prefer header-only distribution)

### Platform Support

- **PLAT-V2-01**: Native macOS build support
- **PLAT-V2-02**: Native Windows build support (MSVC)

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| WebSocket transport | Not defined in MCP 2025-11-25 spec; only stdio and Streamable HTTP are standard |
| Built-in HTTP server | Out of scope; users bring their own HTTP server for SSE/WebSocket |
| Custom JSON Schema validator | Use existing library (nlohmann/json with schema extension) |
| C++20/23 features | Must compile with C++17 for broader compatibility |
| Custom allocators | Standard allocators only; keep it simple |
| Embedded targets | Focus on desktop/server; embedded requires different tradeoffs |
| Global state | Thread-unsafe, prevents multiple connections; use per-connection state |
| Synchronous blocking API as primary | Blocks event loop; async-first with optional sync wrappers |
| Auto-reconnection | Hides errors, makes behavior unpredictable; expose reconnection primitives |
| Built-in LLM integration | Out of scope; library provides MCP protocol only, users bring LLM client |
| Tasks defer to v2 | User wants Tasks in v1 despite experimental status |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| PROTO-01 | Phase 1 | Pending |
| PROTO-02 | Phase 1 | Pending |
| PROTO-03 | Phase 1 | Pending |
| PROTO-04 | Phase 1 | Pending |
| PROTO-05 | Phase 1 | Pending |
| TRAN-03 | Phase 1 | Pending |
| ASYNC-03 | Phase 1 | Pending |
| ASYNC-05 | Phase 1 | Pending |
| API-01 | Phase 1 | Pending |
| SRVR-01 | Phase 2 | Pending |
| SRVR-02 | Phase 2 | Pending |
| SRVR-03 | Phase 2 | Pending |
| SRVR-04 | Phase 2 | Pending |
| SRVR-05 | Phase 2 | Pending |
| SRVR-06 | Phase 2 | Pending |
| TRAN-01 | Phase 2 | Pending |
| ASYNC-01 | Phase 2 | Pending |
| CLNT-01 | Phase 3 | Pending |
| CLNT-02 | Phase 3 | Pending |
| CLNT-03 | Phase 3 | Pending |
| CLNT-04 | Phase 3 | Pending |
| CLNT-05 | Phase 3 | Pending |
| ASYNC-02 | Phase 3 | Pending |
| ASYNC-04 | Phase 3 | Pending |
| TRAN-02 | Phase 4 | Pending |
| TRAN-04 | Phase 4 | Pending |
| ADV-01 | Phase 4 | Pending |
| ADV-02 | Phase 4 | Pending |
| ADV-03 | Phase 4 | Pending |
| ADV-04 | Phase 4 | Pending |
| ADV-05 | Phase 4 | Pending |
| ADV-06 | Phase 4 | Pending |
| CONT-01 | Phase 5 | Pending |
| CONT-02 | Phase 5 | Pending |
| CONT-03 | Phase 5 | Pending |
| CONT-04 | Phase 5 | Pending |
| ADV-07 | Phase 5 | Pending |
| API-02 | Phase 6 | Pending |
| API-03 | Phase 6 | Pending |
| UTIL-01 | Phase 6 | Pending |
| UTIL-02 | Phase 6 | Pending |
| UTIL-03 | Phase 6 | Pending |
| BUILD-01 | Phase 7 | Pending |
| BUILD-02 | Phase 7 | Pending |
| BUILD-03 | Phase 7 | Pending |
| BUILD-04 | Phase 7 | Pending |
| TEST-01 | Phase 7 | Pending |
| TEST-02 | Phase 7 | Pending |
| TEST-03 | Phase 7 | Pending |
| TEST-04 | Phase 7 | Pending |
| TEST-05 | Phase 7 | Pending |

**Coverage:**
- v1 requirements: 52 total
- Mapped to phases: 52
- Unmapped: 0 ✓

---
*Requirements defined: 2025-01-31*
*Last updated: 2025-01-31 after initial definition*
