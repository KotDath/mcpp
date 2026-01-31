# Roadmap: mcpp

## Overview

A modern C++17 library for the Model Context Protocol (MCP) starts with core JSON-RPC protocol handling and builds through transports, server primitives, client capabilities, advanced features, and finally a polished high-level API with full testing coverage. Each phase delivers a coherent, verifiable capability that unblocks the next phase, culminating in a production-ready library supporting the complete MCP 2025-11-25 specification.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Protocol Foundation** - JSON-RPC core, MCP initialization, transport abstraction, async core API
- [x] **Phase 2: Core Server** - Tools, resources, prompts with stdio transport and progress support
- [ ] **Phase 3: Client Capabilities** - Roots, sampling, elicitation with cancellation and futures
- [ ] **Phase 4: Advanced Features & HTTP Transport** - SSE transport, streaming, subscriptions, completions
- [ ] **Phase 5: Content & Tasks** - Rich content types, annotations, pagination, experimental tasks
- [ ] **Phase 6: High-Level API** - Type-safe wrappers, thread safety, logging, utilities
- [ ] **Phase 7: Build & Validation** - CMake build, libraries, testing, Inspector integration

## Phase Details

### Phase 1: Protocol Foundation

**Goal**: A working JSON-RPC 2.0 core with MCP initialization handshake, transport abstraction interface, and callback-based async API foundation that all other layers build upon.

**Depends on**: Nothing (first phase)

**Requirements**: PROTO-01, PROTO-02, PROTO-03, PROTO-04, PROTO-05, TRAN-03, ASYNC-03, ASYNC-05, API-01

**Success Criteria** (what must be TRUE):
1. Library consumer can send a JSON-RPC request and receive a matching response with proper ID correlation
2. Library consumer can perform the MCP initialize/initialized handshake with protocol version negotiation
3. Library consumer can declare and parse structured capability objects with nested sub-capabilities
4. Library consumer can handle JSON-RPC errors and tool execution errors with proper error codes and isError flag
5. Library consumer can register a custom transport implementation against the transport abstraction interface

**Plans**: 6 plans in 3 waves

Plans:
- [x] 01-01-PLAN.md — JSON-RPC 2.0 core types (Request, Response, Notification, Error)
- [x] 01-02-PLAN.md — Transport abstraction interface
- [x] 01-03-PLAN.md — MCP protocol types (capabilities, initialize/initialized)
- [x] 01-04-PLAN.md — Request tracking (atomic ID generation, pending requests)
- [x] 01-05-PLAN.md — Async callbacks and timeout manager
- [x] 01-06-PLAN.md — Low-level callback-based MCP client API

### Phase 2: Core Server

**Goal**: A working MCP server that can list and call tools, list and read resources, and list and get prompts, communicating via stdio transport with progress token support.

**Depends on**: Phase 1 (Protocol Foundation)

**Requirements**: SRVR-01, SRVR-02, SRVR-03, SRVR-04, SRVR-05, SRVR-06, TRAN-01, ASYNC-01

**Success Criteria** (what must be TRUE):
1. Library consumer can register tools and respond to tools/list requests with discovery metadata
2. Library consumer can execute tool calls with JSON Schema input validation and return structured or unstructured results
3. Library consumer can register resources and respond to resources/list with URI-based discovery and MIME types
4. Library consumer can serve resource reads supporting text, blob, and embedded resource content
5. Library consumer can register prompts and respond to prompts/list and prompts/get with argument completion and message templates
6. Library consumer can communicate over stdio transport with a spawned subprocess using newline-delimited JSON
7. Library consumer can attach progress tokens to requests and emit progress notifications for long-running operations

**Plans**: 6 plans in 3 waves

Plans:
- [x] 02-01-PLAN.md — Tool registration and execution with JSON Schema validation
- [x] 02-02-PLAN.md — Resource registration and serving (text/blob content)
- [x] 02-03-PLAN.md — Prompt registration and retrieval with argument substitution
- [x] 02-04-PLAN.md — RequestContext for progress reporting
- [x] 02-05-PLAN.md — McpServer main class integrating all registries
- [x] 02-06-PLAN.md — StdioTransport with subprocess spawning

### Phase 3: Client Capabilities

**Goal**: A working MCP client that can advertise and use roots, sampling, and elicitation capabilities with full cancellation support and ergonomic future-based APIs.

**Depends on**: Phase 1 (Protocol Foundation)

**Requirements**: CLNT-01, CLNT-02, CLNT-03, CLNT-04, CLNT-05, ASYNC-02, ASYNC-04

**Success Criteria** (what must be TRUE):
1. Library consumer can list roots supporting file:// URIs and receive roots/list_changed notifications
2. Library consumer can create LLM completion requests via sampling/createMessage
3. Library consumer can perform agentic behaviors with tool loops via sampling with tool use
4. Library consumer can elicit structured user input via form mode with JSON Schema validation
5. Library consumer can elicit user input via URL mode for secure out-of-band interactions
6. Library consumer can cancel in-flight requests via notifications/cancelled with proper race condition handling
7. Library consumer can use std::future wrappers for ergonomic blocking API on top of async core

**Plans**: 8 plans (6 original + 2 gap closure)

Plans:
- [x] 03-01-PLAN.md — C++20 upgrade and cancellation support (std::stop_token, CancellationManager)
- [x] 03-02-PLAN.md — Roots management (file:// URIs, roots/list, list_changed notification)
- [x] 03-03-PLAN.md — Sampling support (sampling/createMessage for LLM completions)
- [x] 03-04-PLAN.md — Sampling with tool use (agentic loops with tool execution)
- [x] 03-05-PLAN.md — Elicitation support (form mode with JSON Schema, URL mode for out-of-band)
- [x] 03-06-PLAN.md — std::future wrapper API (ergonomic blocking API on async core)
- [ ] 03-07-PLAN.md — Fix CMakeLists.txt build configuration (gap closure)
- [ ] 03-08-PLAN.md — Add ClientCapabilities builder helper (gap closure)

### Phase 4: Advanced Features & HTTP Transport

**Goal**: Streamable HTTP (SSE) transport with non-blocking I/O, tool result streaming, resource templates, subscriptions, completions, tool annotations, and structured output.

**Depends on**: Phase 2 (Core Server - for server primitives to enhance)

**Requirements**: TRAN-02, TRAN-04, ADV-01, ADV-02, ADV-03, ADV-04, ADV-05, ADV-06

**Success Criteria** (what must be TRUE):
1. Library consumer can communicate via Streamable HTTP transport with HTTP POST/SSE session management and resumability
2. Library consumer can use all transports without blocking the event loop
3. Library consumer can receive incremental tool results for long-running operations via tool result streaming
4. Library consumer can define parameterized resources with URI template expansion via resource templates
5. Library consumer can subscribe to resources and receive resources/updated notifications
6. Library consumer can provide argument completion for prompts and resources with reference values
7. Library consumer can annotate tools with metadata (audience, priority, destructive/read-only indicators)
8. Library consumer can validate tool outputs against JSON Schema via structured output

**Plans**: TBD

Plans:
- [ ] 04-01: TBD
- [ ] 04-02: TBD
- [ ] 04-03: TBD

### Phase 5: Content & Tasks

**Goal**: Rich content type handling with annotations, pagination support, list changed notifications, and experimental task lifecycle management.

**Depends on**: Phase 2 (Core Server - for content integration)

**Requirements**: CONT-01, CONT-02, CONT-03, CONT-04, ADV-07

**Success Criteria** (what must be TRUE):
1. Library consumer can work with rich content types including text, image, audio, resource links, and embedded resources
2. Library consumer can attach metadata to content items via content annotations
3. Library consumer can paginate list results using cursor-based pagination with nextCursor handling
4. Library consumer can send and receive tools/prompts/resources/list_changed notifications
5. Library consumer can create and manage experimental tasks with polling, status notifications, result retrieval, and TTL management

**Plans**: TBD

Plans:
- [ ] 05-01: TBD
- [ ] 05-02: TBD

### Phase 6: High-Level API

**Goal**: Type-safe, RAII-based high-level API wrappers with full thread safety, logging support, and utility features for production use.

**Depends on**: Phase 3 (Client Capabilities), Phase 4 (Advanced Features), Phase 5 (Content & Tasks)

**Requirements**: API-02, API-03, UTIL-01, UTIL-02, UTIL-03

**Success Criteria** (what must be TRUE):
1. Library consumer can use high-level wrapper APIs with type-safe interfaces, RAII resource management, and modern C++ patterns
2. Library consumer can safely call any public API method from multiple threads without data races
3. Library consumer can configure and receive log messages at debug, info, warn, and error levels with structured message content
4. Library consumer can reset timeout clocks on progress updates via progress reset notification
5. Library consumer can negotiate custom experimental capabilities via the experimental capabilities field

**Plans**: TBD

Plans:
- [ ] 06-01: TBD
- [ ] 06-02: TBD

### Phase 7: Build & Validation

**Goal**: Production-ready CMake build system with static and shared libraries, MIT licensing, comprehensive test suite, and MCP Inspector integration validation.

**Depends on**: Phase 6 (High-Level API - complete API to test and package)

**Requirements**: BUILD-01, BUILD-02, BUILD-03, BUILD-04, TEST-01, TEST-02, TEST-03, TEST-04, TEST-05

**Success Criteria** (what must be TRUE):
1. Library consumer can build the project using CMake with C++17 standard on gcc-11/g++-11 or compatible
2. Library consumer can link against a static library target for static linking
3. Library consumer can link against a shared library target with proper versioning (SONAME)
4. All source files include MIT license headers for permissive distribution
5. Library consumer can run Google Test unit tests covering core functionality
6. Library consumer can validate the library against real MCP clients via MCP Inspector integration
7. Library consumer can run JSON-RPC compliance tests verifying protocol correctness
8. Library consumer can run thread safety tests verifying concurrent access patterns
9. Library consumer can run async lifetime tests verifying no dangling references

**Plans**: TBD

Plans:
- [ ] 07-01: TBD
- [ ] 07-02: TBD
- [ ] 07-03: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Protocol Foundation | 6/6 | Complete | 2026-01-31 |
| 2. Core Server | 6/6 | Complete | 2026-01-31 |
| 3. Client Capabilities | 6/8 | Gap Closure | - |
| 4. Advanced Features & HTTP Transport | 0/0 | Not started | - |
| 5. Content & Tasks | 0/0 | Not started | - |
| 6. High-Level API | 0/0 | Not started | - |
| 7. Build & Validation | 0/0 | Not started | - |
