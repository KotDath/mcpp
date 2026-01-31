# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2025-01-31)

**Core value:** Developers can build MCP clients and servers in C++ that are fast, correct, and support the complete protocol spec without wrestling with JSON-RPC details or transport plumbing
**Current focus:** Core Server (Phase 2)

## Current Position

Phase: 3 of 7 (Client Capabilities)
Plan: 0 of TBD in current phase
Status: Ready to plan
Last activity: 2026-01-31 — Phase 2 complete: Core Server with 6/6 plans executed, 7/7 success criteria verified (1 auto-fixed bug: transport nullptr dereference)

Progress: [████████░░] 28%

## Performance Metrics

**Velocity:**
- Total plans completed: 12
- Average duration: 5 min
- Total execution time: 1.0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01-protocol-foundation | 6 | 6 | 6 min |
| 02-core-server | 6 | 2 min | 2 min |

**Recent Trend:**
- Last 6 plans: 02-01, 02-02, 02-03, 02-04, 02-05, 02-06
- Trend: Phase 2 complete, ready for Phase 3

*Updated after each plan completion*

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

**From 01-01 (JSON-RPC core types):**
- Request ID as std::variant<int64_t, std::string> for full JSON-RPC spec compliance
- Return std::optional for fallible parsing instead of throwing exceptions
- Separate header/implementation for testability and future optimization
- Static factory methods on JsonRpcError for consistent error creation

**From 01-02 (Transport abstraction):**
- Non-copyable, non-movable base class prevents accidental slicing
- Library owns std::function copies - user can discard originals after registration
- std::string_view parameters for zero-copy message passing
- Explicit lifecycle (not RAII) for transport connections

**From 01-03 (MCP protocol types):**
- std::optional for all optional capability fields to enable extensibility
- CapabilitySet as nlohmann::json alias for experimental capabilities
- Exact match protocol version validation (simplest approach for MVP)
- Empty structs for notifications with no parameters provide type safety

**From 01-04 (Request tracking):**
- Library-managed request IDs using std::atomic - user code never chooses IDs
- Mutex-protected unordered_map for pending requests (simple before optimizing)
- Callbacks stored by value (std::function) to avoid lifetime issues
- Timestamp tracking for future timeout support

**From 01-05 (Async callbacks and timeout manager):**
- std::function-based callback type aliases for all async operations
- steady_clock for timeout tracking (immune to system time changes)
- Callbacks invoked after mutex release to prevent re-entrancy deadlock
- TimeoutManager returns expired IDs for caller cleanup coordination

**From 01-06 (McpClient):**
- Client owns transport via unique_ptr for clear lifecycle management
- Request IDs generated automatically - user code never chooses IDs
- All async operations use std::function callbacks stored by value
- Initialize method automatically sends initialized notification on success

**From 02-01 (Tool Registry):**
- Tool registry with handler-based pattern for dynamic execution
- std::function<ToolResult(const JsonValue&)> for tool callbacks
- URI-agnostic storage, user decides naming scheme
- Non-copyable, non-movable registry classes

**From 02-02 (Resource Registry):**
- Resource content with is_text flag for text vs blob field selection
- Base64 encoding is caller's responsibility for binary content
- URI scheme agnostic - user decides which schemes to support
- MIME type at registration with optional per-read override
- MCP ReadResourceResult format with contents array

**From 02-03 (Prompt Registry):**
- Handler receives raw arguments JSON for flexible template substitution
- PromptRegistry returns MCP GetPromptResult format directly from get_prompt()
- Argument substitution is handler's responsibility (not enforced by registry)

**From 02-04 (RequestContext):**
- Progress values as 0-100 percentage (universally understood, maps cleanly to JSON)
- Optional message parameter in progress notifications
- No-op behavior when no progress token is set (graceful degradation)
- Progress value clamping to 0-100 range for safety
- Transport held by reference (not owned) - server manages lifecycle

**From 02-05 (McpServer):**
- Registries held by value for clear ownership and lifecycle management
- Transport stored as optional non-owning pointer (set after construction)
- RequestContext created per tool call for progress notification support
- JSON-RPC method-based routing to typed handlers
- Progress token extracted from request._meta.progressToken

**From 02-06 (StdioTransport):**
- popen() for subprocess spawning (simpler than fork/exec but no direct PID)
- Newline delimiter appended to all outgoing messages per MCP spec
- Line buffering in read_loop handles partial reads and multiple messages
- Non-blocking pipe with fcntl O_NONBLOCK for better read behavior
- Callbacks stored by value to avoid lifetime issues

### Pending Todos

[From .planning/todos/pending/ — ideas captured during sessions]

None yet.

### Blockers/Concerns

[Issues that affect future work]

**None currently.** Previous verification gap (transport nullptr dereference) was auto-fixed during Phase 2 execution (commit 20178a3).

## Session Continuity

Last session: 2026-01-31
Stopped at: Completed Phase 2 - Core Server with all registries (tools, resources, prompts), RequestContext, McpServer, and StdioTransport. One orchestrator auto-fix applied (transport nullptr dereference). Verification passed 7/7 must-haves.
Resume file: None
