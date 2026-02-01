# Research Summary: mcpp v1.1

**Project:** mcpp — C++ MCP Library
**Milestone:** v1.1 — Stability & Inspector Integration
**Researched:** 2026-02-01
**Confidence:** HIGH

## Executive Summary

mcpp v1.1 is a stabilization milestone focused on fixing a critical JSON-RPC parse error bug (-32700) and enabling MCP Inspector CLI integration. The research reveals that the parse error is likely caused by stdio protocol violations (missing newline delimiters, stdout pollution, or incomplete flushes) in the example server, not the core library architecture. The fix requires adding proper JSON-RPC request validation (`JsonRpcRequest::from_json()`) and ensuring strict stdio protocol compliance (newline-delimited JSON, stderr-only logging, explicit flushing).

For Inspector integration, no new C++ dependencies are required—the Inspector communicates via existing stdio transport. The recommended approach uses Bats-core for automated CLI testing, enhanced logging for debugging, and an example server with proper `mcp.json` configuration. Key risks include stdout pollution corrupting the JSON-RPC stream and timing issues in automated tests, both mitigable through strict protocol compliance and proper test synchronization.

## Key Findings

### Stack

**No new C++ dependencies required for v1.1.** Inspector integration uses existing stdio transport. For testing infrastructure, Bats-core (1.11.0+) is recommended as a TAP-compliant bash testing framework, along with jq for JSON response validation.

**Core technologies remain unchanged from v1.0:**
- nlohmann/json 3.12.0 — De facto standard JSON library, header-only, C++17 optimized
- Asio 1.36.0 — Async networking I/O (standalone, no Boost dependency)
- GoogleTest 1.17.0+ — Unit testing framework (now requires C++17)

**v1.1 additions:**
- MCP Inspector (via npx) — Official MCP testing tool with CLI mode for automation
- Bats-core — Bash testing framework for stdio server validation
- jq — JSON parsing for test assertions

**Enhanced logging strategy** leverages existing `Logger` class with `set_level()` and `enable_payload_logging()` for parse error debugging.

### Features

**v1.1 scope is narrow and focused:** bug fix + testing infrastructure, not new protocol features.

**Must have for v1.1:**
- Fix JSON-RPC parse error (-32700) on all tool calls
- Example server compatible with MCP Inspector CLI
- Automated CLI testing scripts using Bats-core
- Enhanced logging for parse error debugging

**Out of scope for v1.1:**
- New MCP protocol features (deferred to v1.2+)
- WebSocket transport (not in 2025-11-25 spec)
- Built-in HTTP server (users bring their own)

**The fix location:** The parse error handling in `inspector_server.cpp` is ad-hoc. Proper fix requires adding `JsonRpcRequest::from_json()` to mirror existing `JsonRpcResponse::from_json()`, moving validation from example code into reusable library component.

### Architecture

**Inspector integration is layered on existing architecture** without structural changes. The architecture follows a layered design:

- **Transport Layer** (stdio/HTTP) — message framing
- **JSON-RPC Layer** — request/response parsing, validation
- **MCP Server/Client Facades** — MCP protocol specifics, registries
- **Example Server** — application-level code

**Inspector integration touches primarily the application layer and JSON-RPC validation:**
- Inspector spawns server as subprocess, communicates via stdin/stdout
- `inspector_server.cpp` uses manual stdio instead of `StdioTransport` (intentional for simplicity)
- Bug fix requires adding `JsonRpcRequest::from_json()` for proper request parsing
- No transport changes needed—stdio already works

**Modified components:**
- `inspector_server.cpp` — Replace manual JSON parsing with `JsonRpcRequest::from_json()`
- `core/json_rpc.h/cpp` — Add request validation to mirror existing response parsing
- `McpServer::handle_request()` — Add try-catch for proper error responses

**Build order:** Bug fix foundation (library layer) → Example server update → Automated testing → Documentation.

### Pitfalls

**Critical pitfalls to address:**

1. **Missing Newline Delimiters** — nlohmann/json's `dump()` doesn't add trailing newline. MCP spec requires newline-delimited JSON. Without explicit newline, receivers' `fgets()`/`getline()` never see delimiter, causing timeouts. **Prevention:** Always use `std::cout << response.dump() << '\n' << std::flush`.

2. **Stdout Pollution** — Any non-JSON output to stdout corrupts the MCP stdio protocol. Debug output, progress messages, or even `std::cout << "Starting..."` cause parse errors. **Prevention:** Use `std::cerr` for all non-protocol output; configure logging to stderr or file.

3. **Incomplete Flush** — When JSON output isn't flushed completely, receiving side reads partial JSON. **Prevention:** Always call `std::cout.flush()` after sending; consider `setvbuf(stdout, NULL, _IONBF, 0)` for unbuffered stdout.

4. **Process Lifecycle Mismatch** — C++ servers that exit immediately or don't handle shutdown gracefully cause Inspector to report connection failures. **Prevention:** Main loop must keep reading stdin until EOF; handle SIGPIPE/SIGTERM gracefully.

5. **Flaky CLI Tests** — Race conditions between process startup, buffer readiness, and first message send cause intermittent failures. **Prevention:** Use synchronization (poll for readiness or explicit ready marker); proper cleanup traps for background processes.

**Common mistakes:**
- Using `std::cout` for logging in stdio servers (corrupts protocol)
- Forgetting newline after `json.dump()`
- Not flushing stdout after sending
- Ignoring process lifecycle in tests
- Hardcoding paths in test scripts

## Implications for Roadmap

Based on research, v1.1 should be structured as a **4-phase milestone** focusing on bug fixes and testing infrastructure:

### Phase 1: Bug Fix Foundation (Library Layer)

**Rationale:** Fix the JSON-RPC parsing layer first, as this affects all request handling regardless of transport. This is pure library work with no external dependencies.

**Delivers:**
- `JsonRpcRequest::from_json()` for proper request parsing
- Updated `McpServer::handle_request()` with try-catch for proper error responses
- Unit tests for JSON-RPC request validation

**Addresses:**
- Critical parse error (-32700) on tool calls
- Ad-hoc error handling in example server

**Avoids:**
- Stdout pollution pitfalls (by moving validation to library layer)
- Missing newline delimiters (proper response helper)

**Research flag:** Skip research—well-understood JSON-RPC 2.0 specification, existing `JsonRpcResponse::from_json()` provides pattern.

### Phase 2: Inspector Server Integration

**Rationale:** Update the example server to use improved parsing layer and verify Inspector compatibility. This depends on Phase 1's library changes.

**Delivers:**
- Refactored `inspector_server.cpp` using `JsonRpcRequest::from_json()`
- Simplified error handling (no ad-hoc ID extraction)
- Proper error response builder
- `mcp.json` configuration for Inspector/Cursor/Claude Code

**Uses:**
- MCP Inspector (via npx)
- Existing `McpServer` facade
- Phase 1's `JsonRpcRequest::from_json()`

**Implements:**
- Application-layer stdio handling
- Debug mode with enhanced logging
- Proper stdout/stderr separation

**Research flag:** Skip research—Inspector CLI usage is well-documented, existing example server provides working pattern.

### Phase 3: Automated CLI Testing

**Rationale:** Add automated tests to prevent regression. This depends on Phase 2's working example server.

**Delivers:**
- Bats-core test suite for Inspector CLI testing
- Test helpers for server spawning and cleanup
- CMake integration for `cli-tests` target
- Tests covering: tools/list, tools/call, resources/list, prompts/list, error handling

**Uses:**
- Bats-core 1.11.0+
- jq for JSON response validation
- MCP Inspector CLI mode
- Phase 2's `inspector_server` binary

**Avoids:**
- Flaky test pitfalls (synchronization, cleanup)
- Environment pollution (proper process management)

**Research flag:** Skip research—Bats-core patterns are standard, existing test structure provides guidance.

### Phase 4: Documentation & Examples

**Rationale:** Document how to use Inspector for testing so users can validate their own servers.

**Delivers:**
- Updated README with Inspector integration instructions
- Testing guide for running automated tests
- Debug mode documentation
- Parse error troubleshooting guide

**Uses:**
- Phase 2's working example
- Phase 3's test suite as reference

**Research flag:** Skip research—documentation follows implementation patterns.

### Phase Ordering Rationale

The 4-phase order follows dependency chain: library fixes → example server → tests → documentation. Each phase depends on the previous being complete. Grouping by layer (library → application → testing → docs) ensures clean separation of concerns and allows parallel work where possible (e.g., documentation can start once example server works).

**How this avoids pitfalls:**
- Phase 1 addresses stdout pollution and newline delimiter pitfalls at library layer
- Phase 2 addresses process lifecycle and encoding mismatches in example server
- Phase 3 addresses flaky test pitfalls with proper synchronization and cleanup
- Phase 4 documents pitfalls for users

### Research Flags

**All phases can skip `/gsd:research-phase`:**
- **Phase 1:** Well-understood JSON-RPC 2.0 specification, existing code provides pattern
- **Phase 2:** Inspector CLI is documented tool, existing example server demonstrates integration
- **Phase 3:** Bats-core is standard testing framework with clear patterns
- **Phase 4:** Documentation follows implementation

**No phase needs deeper research**—v1.1 is a focused stabilization milestone with clear technical path.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All technologies verified with official sources; no new C++ dependencies required |
| Features | HIGH | v1.1 scope is narrow (bug fix + testing); well-defined from issue analysis |
| Architecture | HIGH | Based on code analysis of existing mcpp v1.0 codebase; Inspector integration is additive |
| Pitfalls | MEDIUM | Stdio pitfalls verified from multiple sources; C++/Node.js integration issues inferred from common patterns |

**Overall confidence:** HIGH

**Gaps to Address:**

1. **Exact parse error cause** — Research identifies multiple potential causes (newline, pollution, flush); actual root cause needs runtime debugging with Inspector during Phase 1. **Mitigation:** Enhanced logging will reveal which issue is occurring.

2. **Bats-core portability** — Research warns about bash vs POSIX shell differences. **Mitigation:** Specify `#!/usr/bin/env bash` shebang and use shellcheck during Phase 3.

3. **Inspector CLI edge cases** — Research assumes standard Inspector behavior; actual CLI may have quirks. **Mitigation:** Manual testing in Phase 2 will reveal any issues before test automation.

## Sources

### Primary (HIGH confidence)

**Official MCP Specification & Tools:**
- MCP 2025-11-25 specification — `/thirdparty/modelcontextprotocol/specification/2025-11-25/`
- [MCP Inspector GitHub](https://github.com/modelcontextprotocol/inspector) — Official Inspector CLI documentation
- [MCP Inspector Docs](https://modelcontextprotocol.io/docs/tools/inspector) — Official usage guide
- [MCP Transports Specification](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports) — Official stdio requirements

**Core Dependencies:**
- [nlohmann/json Releases](https://github.com/nlohmann/json/releases) — v3.12.0 (Apr 2025)
- [Asio C++ Library](https://think-async.com/) — Official site, v1.36.0
- [GoogleTest Repository](https://github.com/google/googletest) — v1.17.0+ requires C++17

**Testing:**
- [Bats-core GitHub](https://github.com/bats-core/bats-core) — Bash Automated Testing System
- [Bats-core Documentation](https://bats-core.readthedocs.io/) — Official docs

**Code Analysis:**
- `examples/inspector_server.cpp` — Lines 352-414 (main request/response loop)
- `src/mcpp/server/mcp_server.cpp` — Lines 95-180 (`handle_request()` implementation)
- `src/mcpp/core/json_rpc.h` — JSON-RPC type definitions
- `src/mcpp/core/json_rpc.cpp` — Response parsing implementation

### Secondary (MEDIUM confidence)

**JSON-RPC Protocol:**
- [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification) — Error code definitions (-32700, -32600)

**Community Knowledge:**
- [Understanding MCP Through Raw STDIO](https://foojay.io/today/understanding-mcp-through-raw-stdio-communication/) — Newline-delimited JSON requirement
- [MCP Debugging Best Practices](https://www.mcpevals.io/blog/debugging-mcp-servers-tips-and-best-practices) — Parse error causes
- [StackOverflow: MCP parse errors](https://stackoverflow.com/questions/79550897/mcp-server-always-get-initialization-error) — Common stdio issues
- [How to Test MCP Servers](https://codely.com/en/blog/how-to-test-mcp-servers) — E2E testing approaches

**nlohmann/json:**
- [nlohmann/json dump() documentation](https://json.nlohmann.me/api/basic_json/dump/) — Confirms no automatic newline

**Reference Implementations:**
- [fastmcpp C++ port](https://github.com/0xeb/fastmcpp) — C++ MCP implementation reference

### Tertiary (LOW confidence — requires validation)

**C++/Node.js Integration:**
- [Local MCP Development with C++ and Gemini CLI](https://xbill999.medium.com/local-mcp-development-with-c-and-gemini-cli-5c9def0a9752) — C++ stdio server patterns (single source)

**Issue Reports (inferred patterns):**
- [MCP server stdio mode corrupted by stdout log messages](https://github.com/ruvnet/claude-flow/issues/835) — Critical issue about stdout pollution
- [MCP Inspector Fails to Connect with stdio Transport](https://github.com/modelcontextprotocol/inspector/issues/106) — Connection troubleshooting
- [STDIO Connection Failure with MCP Inspector and Cline](https://github.com/awslabs/mcp/issues/1647) — Timeout error patterns
- [Error -32000, Connection Closed, Timeout Solutions](https://mcp.harishgarg.com/learn/mcp-server-troubleshooting-guide-2025) — MCP error codes and solutions

---
*Research completed: 2026-02-01*
*Ready for roadmap: yes*
