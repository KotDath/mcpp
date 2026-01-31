# Phase 1: Protocol Foundation - Context

**Gathered:** 2025-01-31
**Status:** Ready for planning

## Phase Boundary

JSON-RPC 2.0 core implementation with MCP initialization handshake, transport abstraction interface, and callback-based async API foundation. This phase delivers the infrastructure that all higher-level layers (server primitives, client capabilities, transports) build upon.

## Implementation Decisions

### Transport Interface

- **Bidirectional interface** — Single transport abstraction handles both sending and receiving messages
- **Transport handles framing** — Each transport implementation handles its own message boundaries:
  - stdio: newline-delimited JSON
  - SSE: Server-Sent Events protocol framing
  - JSON-RPC layer works with complete messages only
- **Explicit lifecycle** — `connect()`, `disconnect()`, `is_connected()` methods (not RAII-based)
- **Error reporting: both approaches** — Return codes for synchronous operations, error callbacks for async operations
- **Message format** — Based on rmcp codec pattern: encode to JSON + newline delimiter for stdio

### Async Callback Model

- **Storage: std::function** — Library owns callbacks via `std::function`, user moves them in
- **Thread safety: library guarantees** — Library always invokes callbacks on the same thread (simpler for users)
- **Lifetime: documented safe patterns** — Document reference capture risks; recommend value captures for safety
- **Callback ownership** — Library stores `std::function` copies; user's original can be discarded after registration

### Request Tracking

- **Library-managed request IDs** — Library generates request IDs internally using atomic counter (like rmcp)
- **Pending request storage** — HashMap stores pending requests with their response correlators
- **Automatic cleanup** — Remove pending requests when response arrives or timeout occurs
- **Request ID type** — `NumberOrString` for flexibility (numeric or string IDs)

### Timeout Handling

- **Both global and per-request** — Global default timeout with per-request override capability
- **Timeout manager** — Background task checks for expired requests and triggers timeout callbacks
- **Timeout error** — Distinguish timeout errors from connection/transport errors
- **Default timeout** — 5 minutes (300 seconds) like rmcp, but configurable

### Capability Structure

- **Follow MCP spec** — Capability objects match MCP 2025-11-25 schema exactly
- **Nested sub-capabilities** — Support structured capability objects with nested features
- **Version negotiation** — Protocol version in initialize/initialized handshake

### Claude's Discretion

- Exact timeout manager implementation (event loop vs separate thread)
- Request ID internal representation (atomic counter type)
- HashMap implementation details (concurrent map vs mutex-protected)
- Exact error type hierarchy (follow C++ conventions)

## Specific Ideas

- "Follow rmcp patterns for transport framing and request tracking"
- "Use explicit lifecycle methods, not RAII — users want control over connection timing"
- "Library should guarantee same-thread callback execution for thread safety"
- "Global 5-minute timeout with per-request override gives good defaults + flexibility"

## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 01-protocol-foundation*
*Context gathered: 2025-01-31*
