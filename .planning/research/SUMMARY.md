# Project Research Summary

**Project:** mcpp — C++ MCP (Model Context Protocol) Library
**Domain:** C++ Networking / Protocol Library
**Researched:** 2025-01-31
**Confidence:** HIGH

## Executive Summary

mcpp is a C++17 library implementing the Model Context Protocol (MCP) — a JSON-RPC 2.0-based protocol for LLM server-client communication. Expert C++ protocol libraries prioritize modern async I/O (Asio), header-only dependencies where possible (nlohmann/json), and layered architecture (event loop → transport → filter chain → protocol → API). The recommended approach follows Envoy Proxy's threading model: single-threaded event loops with thread-safe posting to avoid lock contention, combined with a filter chain for protocol processing.

Key risks center on C++ async lifetime management: lambda reference captures in async contexts, `std::string_view` dangling references, and misuse of `std::shared_ptr` for thread safety. These pitfalls cause heisenbugs that are extremely difficult to debug. Mitigation requires establishing strict coding guidelines in Phase 1: async-only API design (no blocking methods), RAII-based lifetime management, and thread-local dispatcher patterns. Architecture should separate transport, protocol, and API layers to enable independent testing and future extensibility.

The MCP protocol requires specific features beyond basic JSON-RPC: initialization handshake, capability negotiation, progress tokens with cancellation, and three server primitives (tools, resources, prompts). Transports include stdio (subprocess), Streamable HTTP (SSE), and WebSocket. Competitive differentiation comes from zero-copy parsing, layered API design (callback + future-based), and header-only distribution option — all areas where C++ excels over higher-level languages.

## Key Findings

### Recommended Stack

Core technologies prioritize modern C++17 with header-only dependencies to minimize build complexity while maintaining performance.

**Core technologies:**
- **nlohmann/json 3.12.0** — JSON parsing/serialization; de facto standard, header-only, actively maintained (Apr 2025)
- **Asio 1.36.0 (standalone)** — Async networking I/O; modern C++ design, no Boost dependency, excellent coroutine support
- **llhttp 9.3.0** — HTTP/1.x parsing; Node.js's HTTP parser, production-hardened, security-patched
- **WebSocket++ 0.8.2** — WebSocket transport; header-only, integrates with Asio (despite 2020 last release)
- **OpenSSL 1.1+/3.0+** — TLS/SSL; required for HTTPS+WSS, industry standard
- **GoogleTest 1.17.0+** — Testing; requires C++17, aligns with project

**Stack variants:**
- **stdio-only:** nlohmann/json + GoogleTest (minimal dependencies)
- **stdio + SSE:** Add llhttp for HTTP parsing + OpenSSL
- **Full stack:** All above + WebSocket++ + Asio (recommended production config)

**Critical decision:** Build custom JSON-RPC layer using nlohmann/json rather than json-rpc-cxx (too restrictive, poor MCP integration). Build custom SSE parser using llhttp (no quality C++ SSE library exists).

### Expected Features

MCP is JSON-RPC 2.0 with extensions. Feature completeness determined by 2025-11-25 specification.

**Must have (table stakes):**
- **JSON-RPC 2.0 Core** — Foundation of all communication (request/response, error handling)
- **Initialization Handshake** — Protocol lifecycle, version negotiation, capability exchange
- **stdio Transport** — Standard for local MCP servers (subprocess communication)
- **Tools (list/call)** — Core server primitive (executable functions for LLMs)
- **Resources (list/read)** — Core server primitive (data sources for context)
- **Prompts (list/get)** — Core server primitive (reusable templates)
- **Progress Tokens** — Required for long-running operations (opaque token, progress notifications)
- **Cancellation** — Required for user control (notifications/cancelled, race handling)
- **Roots (client)** — Filesystem boundaries (roots/list, file:// URIs)
- **Sampling (client)** — LLM completion through client (sampling/createMessage)
- **Elicitation (client)** — User input through client (form mode for structured input)

**Should have (competitive):**
- **Streamable HTTP Transport** — Remote server support, HTTP POST/SSE, session management
- **Resource Subscriptions** — Monitor individual resource changes (resources/subscribe)
- **Structured Output** — Tool result validation (outputSchema, JSON Schema validation)
- **Layered API Design** — Low-level core + high-level wrappers (differentiates from C# SDK)
- **Zero-Copy Abstractions** — Minimize memory copies for performance (view types, borrowed data)

**Defer (v2+):**
- **Tasks (Experimental)** — Durable execution wrapper, adds significant state management complexity
- **Tool Result Streaming** — Advanced feature, can add after core validation
- **WebSocket Transport** — Alternative to SSE, can add after HTTP transport working
- **Completion** — Argument completion, nice to have but not critical

### Architecture Approach

Layered architecture inspired by Envoy Proxy: event loop foundation → transport abstraction → filter chain → JSON-RPC protocol → high-level API.

**Major components:**
1. **Event Loop/Dispatcher** — Async I/O multiplexing (epoll/kqueue/IOCP), timer scheduling, callback execution; thread-local pattern avoids locks
2. **Transport Abstraction** — Byte-level I/O, transport-specific framing (stdio/SSE/WebSocket); polymorphic implementations
3. **Filter Chain** — Protocol processing pipeline (JSON-RPC codec → HTTP/SSE framing → message framing); pipes-and-filters pattern
4. **JSON-RPC Layer** — Request/response validation, ID generation/tracking, message serialization; state machine for protocol lifecycle
5. **McpClient/McpServer** — User-facing facades; coordinate internal components (connection manager, protocol handler, request tracker)

**Key patterns:**
- **Single-threaded event loops** with thread-safe posting → no locks on data path
- **Filter chain** → extensible protocol processing, clear separation of concerns
- **Layered API** → callback-based core + future-based convenience wrappers
- **Facade pattern** → simple user API hiding internal complexity

### Critical Pitfalls

C++ async code has specific pitfalls that cause production disasters. Prevention through architecture and coding guidelines.

1. **Lambda Reference Capture in Async Callbacks** — Capturing `[&]` in lambdas passed to async operations causes dangling references when callback executes after scope exits. **Prevention:** Always capture by value `[=]` or use `std::shared_ptr` for ownership. Establish static analysis rules in Phase 1.

2. **std::string_view Dangling References** — `std::string_view` is non-owning; using as member variable or return type creates dangling references. **Prevention:** Use `std::string` for owned data, `string_view` only as function parameter. Audit all string_view usage in Phase 2.

3. **std::shared_ptr Thread Safety Misconception** — `shared_ptr` makes reference count atomic, not the pointed-to object. Concurrent access without synchronization is data race. **Prevention:** Use mutex for shared mutable objects, or thread-local dispatcher pattern where each thread owns its data.

4. **Blocking the Event Loop** — User callbacks performing blocking I/O or computation stalls entire event loop. **Prevention:** Async-only API design (no blocking `callTool()`, only `callToolAsync()`). Offload heavy work to worker threads with explicit posting back to event loop.

5. **Lock Contention on Hot Path** — Protecting request maps/connection state with mutexes causes contention at scale. **Prevention:** Thread-local dispatcher pattern (each thread owns its connections), cross-thread communication via lock-free `post()` only.

6. **JSON-RPC Request ID Mishandling** — Request ID collisions, improper tracking, or timeout cleanup cause lost responses or memory leaks. **Prevention:** Connection-scoped IDs with timeout cleanup, explicit state machine for request lifecycle (Pending → Canceling → Complete/Failed).

7. **stdio Transport Deadlocks** — Blocking I/O on stdio causes deadlocks when buffers fill. **Prevention:** Non-blocking I/O with event loop integration, separate thread for stderr handling.

8. **MCP Progress/Cancellation Race Conditions** — Progress notifications arriving after completion, cancellation racing with in-progress operations. **Prevention:** Explicit state machine with validated state transitions, terminal state checking before setting promises.

## Implications for Roadmap

Based on combined research, suggested phase structure:

### Phase 1: Foundation — Event Loop & Core Types

**Rationale:** Event loop is the foundation everything else depends on. Core types (Result, async utilities, thread-safe posting) establish patterns used throughout. Critical pitfalls (lambda capture, lock contention, blocking event loop) must be prevented through architecture here.

**Delivers:**
- Event loop abstraction (Dispatcher) with platform-specific I/O (epoll/kqueue/IOCP)
- Thread-safe posting mechanism for cross-thread communication
- Core types: Result<T>, async utilities, error handling
- Transport interface definition (no implementations yet)

**Addresses:**
- Stack: Event loop platform factory
- Features: Foundation for all async operations
- Architecture: Event loop + platform I/O layer
- Pitfalls: Blocking event loop (#4), Lock contention (#5), Wrong memory ordering (#6)

**Avoids:**
- Mixing sync/async APIs later
- Lock-based threading model
- Global state for configuration

### Phase 2: Protocol Layer — JSON-RPC & MCP Types

**Rationale:** Protocol layer defines all MCP types and JSON-RPC mechanics. This is where request tracking, progress/cancellation state machines, and message validation happen. Must be solid before adding transports.

**Delivers:**
- JSON-RPC message types (Request, Response, Notification)
- JSON-RPC request ID tracking with timeout cleanup
- MCP schema types (Tool, Resource, Prompt, etc.)
- Protocol state machine (initialization, capability negotiation)
- Progress/cancellation state machine with validated transitions
- Message validation and error handling

**Uses:**
- Stack: nlohmann/json for serialization
- Architecture: JSON-RPC layer responsibility
- Filter interfaces (ReadFilter/WriteFilter)

**Implements:**
- Features: JSON-RPC Core, Initialization Handshake, Capability Declaration, Progress Tokens, Cancellation, Error Handling

**Avoids:**
- Pitfall #2 (string_view dangling) — use std::string for owned data
- Pitfall #6 (request ID mishandling) — connection-scoped IDs with cleanup
- Pitfall #10 (progress/cancellation races) — explicit state machine

### Phase 3: Transport Implementations

**Rationale:** With protocol layer complete, implement transports. Start with stdio (simplest, required for MCP), then add SSE (most complex due to HTTP parsing). WebSocket can be v1.x.

**Delivers:**
- stdio transport implementation (non-blocking, event loop integrated)
- SSE transport implementation (HTTP parsing via llhttp, custom SSE parser)
- Transport registration/factory mechanism
- Session management for SSE (session ID, resumption)

**Uses:**
- Stack: llhttp for HTTP parsing, OpenSSL for TLS
- Architecture: Transport layer, filter chain foundation
- Features: stdio Transport, Streamable HTTP Transport (v1.x)

**Avoids:**
- Pitfall #9 (stdio deadlocks) — non-blocking I/O, stderr handling
- Pitfall #14 (stdio newline handling) — explicit newline framing
- Pitfall #13 (ABI fragility) — Pimpl if needed for public transport classes

### Phase 4: Filter Chain Implementation

**Rationale:** Filters process protocol layers. Implement filters that connect transports to JSON-RPC layer. This completes the data pipeline.

**Delivers:**
- Filter chain manager with ReadFilter/WriteFilter interfaces
- JSON-RPC codec filter (serialization/deserialization)
- Framing filter (length-prefix or newline-delimited)
- HTTP/SSE codec filter (for SSE transport)
- Filter registration and ordering

**Uses:**
- Architecture: Filter chain pattern
- Stack: nlohmann/json in JSON-RPC filter, llhttp in HTTP filter
- Features: All transport-specific framing

**Avoids:**
- Pitfall #11 (wrong JSON library) — design abstraction to allow simdjson later
- Pitfall #16 (message length overflow) — validate before allocation
- Security issues (message size validation)

### Phase 5: High-Level API — Client & Server

**Rationale:** With all infrastructure complete, build user-facing API. This is the facade that coordinates all components. Provide both callback-based (low-level) and future-based (convenience) APIs.

**Delivers:**
- McpClient facade (connection lifecycle, request tracking)
- McpServer facade (listener management, session handling)
- Tool/Resource/Prompt wrapper APIs
- Builder API for server configuration
- Future-based convenience methods
- Integration testing with MCP Inspector

**Uses:**
- Architecture: Facade pattern, layered API design
- Features: Tools (list/call), Resources (list/read), Prompts (list/get), Roots (client), Sampling (client), Elicitation (client)

**Avoids:**
- Pitfall #7 (coroutine lifetime) — if using coroutines, establish strict guidelines
- Pitfall #13 (header-only ABI fragility) — Pimpl or document limitations
- Pitfall #15 (future not retrieved) — explicit fire-and-forget methods

### Phase 6: Validation & Documentation

**Rationale:** Complete feature set needs testing against real MCP clients/servers. Documentation for library users.

**Delivers:**
- Integration tests with MCP Inspector
- Example server implementation
- Example client implementation
- API documentation
- Performance benchmarks

**Uses:**
- Stack: GoogleTest for testing, MCP Inspector for validation
- Features: All MVP features

**Avoids:**
- "Looks done but isn't" issues (checklist from PITFALLS.md)

### Phase 7: Advanced Features (v1.x+)

**Rationale:** After MVP validation, add competitive differentiators and advanced transport options.

**Delivers:**
- WebSocket transport
- Resource Subscriptions
- Structured Output (outputSchema)
- Resource Templates
- Sampling with Tool Use

**Uses:**
- Stack: WebSocket++ for WebSocket
- Features: Advanced features from FEATURES.md

### Phase Ordering Rationale

- **Foundation first:** Event loop and core types must exist before anything async can work
- **Protocol before transport:** JSON-RPC layer is transport-agnostic; define it before implementing transports
- **stdio before SSE:** stdio is simpler (no HTTP parsing), validates protocol layer
- **Filters after transports:** Filters depend on transport interface, but implement before high-level API
- **API last:** Client/Server facades coordinate all components; they can't be built until infrastructure exists
- **Validation before advanced:** Prove MVP works before adding complexity

This ordering follows the architecture build order (ARCHITECTURE.md § Build Order) and ensures pitfalls are addressed in the appropriate phase (PITFALLS.md § Pitfall-to-Phase Mapping).

### Research Flags

**Phases likely needing deeper research during planning:**
- **Phase 3 (Transport Implementations):** SSE parsing has no quality C++ library; custom parser implementation needs research on SSE edge cases and reconnection semantics
- **Phase 5 (High-Level API):** If using coroutines for ergonomics, need research on C++20 coroutine lifetime best practices — pitfalls are severe and patterns still evolving (CppCon/ACCU 2025 guidance)
- **Phase 7 (WebSocket Transport):** WebSocket integration with Asio has patterns but less documented than SSE; may need `/gsd:research-phase`

**Phases with standard patterns (skip research-phase):**
- **Phase 1 (Foundation):** Event loop patterns well-documented (Envoy, Asio docs, gopher-mcp reference)
- **Phase 2 (Protocol Layer):** JSON-RPC 2.0 is stable spec, MCP 2025-11-25 spec is clear
- **Phase 4 (Filter Chain):** Pipes-and-filters pattern well-established (Envoy filters)
- **Phase 6 (Validation):** Standard testing patterns, MCP Inspector provides test harness

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Verified with official sources (nlohmann/json releases, Asio docs, WebSocket++ repo). Alternatives well-researched with clear rationale. Minor gap: SSE library search confirmed no C++ options, but custom parser approach is standard. |
| Features | HIGH | MCP 2025-11-25 specification is authoritative source (local copy in /thirdparty/). Feature dependencies clearly defined. Priority matrix based on specification requirements and competitor analysis. |
| Architecture | HIGH | Based on proven patterns from Envoy Proxy (official blog), gopher-mcp reference implementation (local code), rmcp Rust SDK (local code). Build order logically derived from dependencies. |
| Pitfalls | HIGH | All critical pitfalls sourced from authoritative sources (StackOverflow consensus, C++ Core Guidelines, PVS-Studio). Specific to C++ async/networking code. MCP-specific pitfalls from spec and GitHub discussions. |

**Overall confidence:** HIGH

Research is comprehensive with authoritative sources. Stack choices are well-documented with version-specific rationale. Architecture follows proven patterns from production systems (Envoy). Pitfalls are specific to C++ async code with prevention strategies. Minor gaps (SSE parser details, coroutine ergonomics) can be addressed during implementation without architectural risk.

### Gaps to Address

- **SSE parser implementation:** No quality C++ SSE library exists; building custom parser using llhttp. Need to validate SSE reconnection semantics and Last-Event-ID handling during Phase 3 implementation. **Mitigation:** Reference gopher-mcp's sse_codec_filter.cc implementation and MDN SSE specification.

- **Coroutine ergonomics (optional):** If using C++20 coroutines in Phase 5 for user-facing API, need research on lifetime management patterns. CppCon/ACCU 2025 guidance is that patterns are still evolving. **Mitigation:** Start with callback-based core API, add coroutines as optional wrapper later (can be v1.x feature).

- **JSON library performance:** nlohmann/json is recommended for convenience, but may become bottleneck at high throughput (>10K messages/second). **Mitigation:** Design JSON abstraction layer to allow swapping to simdjson for hot paths. Benchmark during Phase 2 to verify if switch needed.

- **MCP spec versioning:** MCP is evolving quickly (2025-11-25 spec). Need to handle version negotiation gracefully. **Mitigation:** Always send protocol version in initialize, validate peer compatibility, document version support matrix.

## Sources

### Primary (HIGH confidence)

**Official MCP Specification:**
- /thirdparty/modelcontextprotocol/schema/2025-11-25/schema.json — Official schema
- /thirdparty/modelcontextprotocol/docs/specification/2025-11-25/ — Complete specification
- /thirdparty/modelcontextprotocol/docs/docs/learn/architecture.mdx — Architecture overview
- https://modelcontextprotocol.io/specification/latest — Living specification

**Official Library Documentation:**
- [nlohmann/json Releases](https://github.com/nlohmann/json/releases) — v3.12.0 (Apr 2025)
- [WebSocket++ Repository](https://github.com/zaphoyd/websocketpp) — v0.8.2 documentation
- [llhttp Releases](https://github.com/nodejs/llhttp/releases) — v9.3.0 (May 2025)
- [Asio C++ Library](https://think-async.com/) — Official site, v1.36.0
- [GoogleTest Repository](https://github.com/google/googletest) — v1.17.0+ C++17 requirement

**Reference Implementations:**
- /thirdparty/gopher-mcp/ — Complete C++ MCP implementation (filter chain architecture, event loop abstraction)
- /thirdparty/rust-sdk/crates/rmcp/ — Modern MCP implementation (service trait, transport interface)

**Architecture References:**
- [Envoy Threading Model (Official Blog)](https://blog.envoyproxy.io/envoy-threading-model-a8d44b922310) — "No locks on data path" architecture
- [Envoy Life of a Request](https://www.envoyproxy.io/docs/envoy/latest/intro/life_of_a_request) — Filter chain architecture

### Secondary (MEDIUM confidence)

**Community Research:**
- [Asio vs libevent performance comparison (2024)](https://juejin.cn/post/7314485430385295400) — Benchmark: Asio ~800K QPS vs libevent ~500K QPS
- [C++ networking library comparison (2025)](https://blog.serdardogruyol.com/cpp-networking-library-comparison) — Comprehensive benchmark
- [json-rpc-cxx GitHub](https://github.com/jsonrpcx/json-rpc-cxx) — Evaluated and rejected (too restrictive)
- Reddit: "Asio in 2024/2025" — Community sentiment on Asio vs Boost

**C++ Best Practices:**
- C++ Core Guidelines I.27 — Pimpl idiom for stable ABI
- PVS-Studio Blog: "C++ programmer's guide to undefined behavior" (Jan 2025)
- Chromium C++ Style Guide — Coroutines as safer alternative for async code

**MCP Community:**
- GitHub Discussion #491: "Asynchronous operations in MCP"
- Dev.to: "Complete MCP JSON-RPC Reference Guide"

### Tertiary (LOW confidence)

**Areas needing validation:**
- SSE library search — No C++-specific libraries found; custom parser recommendation untested but aligned with community practice
- Some alternative libraries mentioned in community discussions (Glaze, rapidjson alternatives) but not verified for this use case
- Coroutine lifetime patterns — CppCon/ACCU 2025 talks highlight this as active area without settled best practices

---
*Research completed: 2025-01-31*
*Ready for roadmap: yes*
