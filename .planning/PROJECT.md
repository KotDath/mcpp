# MCP C++ Library (mcpp)

## What This Is

A modern C++17 library for building Model Context Protocol (MCP) clients and servers. Provides full MCP 2025-11-25 spec coverage with all transports (stdio, SSE, WebSocket), fast JSON-RPC handling, and minimal blocking operations. Distributed under MIT license for both static and shared library use.

## Core Value

Developers can build MCP clients and servers in C++ that are fast, correct, and support the complete protocol spec without wrestling with JSON-RPC details or transport plumbing.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Complete MCP 2025-11-25 spec implementation (tools, resources, prompts, logging, sampling, roots, progress, cancellations, streaming)
- [ ] All transports: stdio, SSE (Server-Sent Events), WebSocket
- [ ] Fast and correct JSON-RPC 2.0 handling
- [ ] Minimal blocking operations — async/non-blocking design
- [ ] Fully thread-safe API
- [ ] Layered API design: low-level core + high-level wrappers
- [ ] CMake build system, C++17 standard, gcc-11/g++-11 compatible
- [ ] Google Test + MCP Inspector integration for testing
- [ ] Linux primary, cross-platform support (macOS, Windows via Docker)
- [ ] MIT license
- [ ] Static and shared library distribution

### Out of Scope

- [C++20/23 features] — Must compile with C++17 for broader compatibility
- [Built-in HTTP server] — Library provides MCP over transports; users bring their HTTP server for SSE/WebSocket
- [Custom allocators] — Standard allocators only; keep it simple
- [Embedded targets] — Focus on desktop/server; embedded would require different tradeoffs

## Context

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
- Dependencies: Pragmatic approach (nlohmann/json, openssl, networking libs as needed)

**Known challenges:**
- Full MCP spec is large and complex (streaming, cancellations, progress tokens)
- Thread-safe async design in C++17 requires careful architecture
- Multiple transports require abstraction layer while maintaining performance
- JSON-RPC correctness is critical — any deviation breaks protocol compatibility

## Constraints

- **Language**: C++17 — Must compile with gcc-11/g++-11, no C++20+ features
- **License**: MIT — Permissive for both static and shared distribution
- **Performance**: Fast JSON-RPC handling, minimal blocking operations
- **Concurrency**: Fully thread-safe, async/non-blocking design
- **Platforms**: Linux primary, macOS/Windows via Docker validation
- **Transports**: stdio, SSE, WebSocket — all required in v1

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Layered API (low-level core + high-level wrappers) | Low-level for power users and spec updates; high-level for ergonomics | — Pending |
| Async model: callback core with future wrappers | Streaming needs callbacks; simple RPC works with futures | — Pending |
| Pragmatic dependencies | nlohmann/json, openssl, networking libs are proven and reliable | — Pending |
| Traditional library structure | Header-only would slow compiles; separate headers/sources scale better | — Pending |
| MCP 2025-11-25 spec | Latest standard with streaming and enhanced capabilities | — Pending |

---
*Last updated: 2025-01-31 after initialization*
