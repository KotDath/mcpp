# MCP C++ Library (mcpp)

## What This Is

A modern C++17 library for building Model Context Protocol (MCP) clients and servers. Provides full MCP 2025-11-25 spec coverage with all transports (stdio, SSE), fast JSON-RPC handling, and minimal blocking operations. Distributed under MIT license for both static and shared library use.

**Current State:** v1.1 in development — fixing critical JSON-RPC parse error bug, adding MCP Inspector integration, improving test coverage

## Current Milestone: v1.1 Stability & Inspector Integration

**Goal:** Fix critical JSON-RPC parse error bug and enable proper testing via MCP Inspector CLI mode.

**Target features:**
- Fix JSON-RPC parse error (-32700) affecting all tool calls
- Build example server compatible with `npx @modelcontextprotocol/inspector`
- Create automated CLI testing scripts for CI/CD
- Add comprehensive edge case and integration tests

## Core Value

Developers can build MCP clients and servers in C++ that are fast, correct, and support the complete protocol spec without wrestling with JSON-RPC details or transport plumbing.

## Requirements

### Validated

- ✓ Complete MCP 2025-11-25 spec implementation — v1.0
- ✓ All transports: stdio, SSE (Server-Sent Events) — v1.0
- ✓ Fast and correct JSON-RPC 2.0 handling — v1.0
- ✓ Minimal blocking operations — async/non-blocking design — v1.0
- ✓ Fully thread-safe API — v1.0
- ✓ Layered API design: low-level core + high-level wrappers — v1.0
- ✓ CMake build system, C++17 standard, gcc-11/g++-11 compatible — v1.0
- ✓ Google Test + MCP Inspector integration for testing — v1.0
- ✓ Linux primary, cross-platform support (documented patterns for macOS, Windows) — v1.0
- ✓ MIT license — v1.0
- ✓ Static and shared library distribution — v1.0

### Active

- [ ] Fix JSON-RPC parse error (-32700) on tool calls — v1.1
- [ ] MCP Inspector example server for CLI/UI testing — v1.1
- [ ] Automated CLI testing scripts for CI/CD — v1.1
- [ ] Improved test coverage (edge cases, integration tests) — v1.1

### Out of Scope

- [WebSocket transport] — Not in MCP 2025-11-25 spec; can be added in future release
- [Built-in HTTP server] — Library provides MCP over transports; users bring their HTTP server for SSE
- [Custom allocators] — Standard allocators only; keep it simple
- [Embedded targets] — Focus on desktop/server; embedded would require different tradeoffs

## Context

<details>
<summary>v1.0 Development Context (archived)</summary>

**Reference implementations provided:**
- `thirdparty/gopher-mcp/` — C++ MCP library with outdated protocol, useful for structure and dependencies
- `thirdparty/rust-sdk/` — Rust SDK showing modern MCP patterns
- `thirdparty/fastmcp/` — Python implementation for protocol understanding
- `thirdparty/modelcontextprotocol/` — Official spec (docs/ and schema/), targeting 2025-11-25 version
- `thirdparty/inspector/` — MCP testing utility for integration validation

**Technical environment:**
- Build system: CMake
- Language: C++17 (gcc-11/g++-11)
- Testing: Google Test + MCP Inspector
- Dependencies: nlohmann/json, openssl, networking libs as needed

**Known challenges addressed:**
- Full MCP spec is large and complex (streaming, cancellations, progress tokens) — resolved through phased implementation
- Thread-safe async design in C++17 requires careful architecture — resolved with std::stop_token and callback patterns
- Multiple transports require abstraction layer while maintaining performance — resolved with Transport interface
- JSON-RPC correctness is critical — resolved with comprehensive compliance tests
</details>

<details>
<summary>v1.0 Architecture Decisions</summary>

**Layered API Architecture:**
- Low-level callback-based API (Phases 1-5): Direct MCP protocol access with std::function callbacks
- High-level RAII-based API (Phase 6): Service<Role>, Peer<Role>, RunningService<Role> for modern C++ patterns
- Both patterns coexist — users choose based on their use case

**Async Model:**
- Core: std::function callbacks for streaming support
- Optional: std::future wrappers for simple RPC calls
- Cancellation: std::stop_token (C++20 feature with compiler support)

**Transport Design:**
- Abstract Transport interface for pluggable implementations
- StdioTransport for subprocess communication
- HttpTransport with user-provided HTTP server integration
- Non-blocking I/O throughout

</details>

**Current codebase state:**
- LOC: 15,511 lines of C++ (headers/sources)
- Test coverage: 184 tests (100% pass rate)
- Build targets: Static library (libmcpp.a) and shared library (libmcpp.so.0.1.0)
- Sanitizer testing: AddressSanitizer/LeakSanitizer and ThreadSanitizer documented in README

**Known issues:**
- **CRITICAL**: JSON-RPC parse error (-32700) on all tool calls — needs investigation and fix
- CMake 3.31 with GCC 15 has object file directory creation issue (workaround documented)
- Optional nlohmann/json-schema-validator dependency (conditionally compiled via MCPP_HAS_JSON_SCHEMA)
- NullTransport not exported in public headers (internal use only)

## Constraints

- **Language**: C++17 — Must compile with gcc-11/g++-11, no C++20+ features except std::stop_token
- **License**: MIT — Permissive for both static and shared distribution
- **Performance**: Fast JSON-RPC handling, minimal blocking operations
- **Concurrency**: Fully thread-safe, async/non-blocking design
- **Platforms**: Linux primary, macOS/Windows via Docker validation
- **Transports**: stdio, SSE — both required in v1

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Layered API (low-level core + high-level wrappers) | Low-level for power users and spec updates; high-level for ergonomics | ✓ Both patterns working, users have choice |
| Async model: callback core with future wrappers | Streaming needs callbacks; simple RPC works with futures | ✓ Streaming and simple calls both supported |
| Pragmatic dependencies | nlohmann/json, openssl, networking libs are proven and reliable | ✓ Stable dependency chain |
| Traditional library structure | Header-only would slow compiles; separate headers/sources scale better | ✓ 15K LOC builds quickly |
| MCP 2025-11-25 spec | Latest standard with streaming and enhanced capabilities | ✓ Full spec coverage achieved |
| std::stop_token for cancellation | C++20 feature for better composability with std::jthread | ✓ Cancellation working cleanly |
| User-provided HTTP server integration | Library provides MCP protocol, not HTTP infrastructure | ✓ Template adapter pattern working |
| Optional json-schema dependency | Not all users need output validation | ✓ Conditional compilation working |

## Future Milestones

Potential areas for v1.2+:
- WebSocket transport (if added to MCP spec)
- Zero-copy types for performance optimization
- Header-only build option for easier distribution
- Native macOS build support
- Native Windows build support (MSVC)
- Additional examples and tutorials

---
*Last updated: 2026-02-01 after starting v1.1 milestone*
