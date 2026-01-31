# Phase 4: Advanced Features & HTTP Transport - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

## Phase Boundary

Building transport and feature extensions — streamable HTTP (SSE) transport, non-blocking I/O, tool result streaming, resource templates, subscriptions, completions, tool annotations, and structured output. This phase delivers infrastructure and capabilities that library users CALL and configure.

## Implementation Decisions

### SSE Transport Lifecycle

**Endpoint structure:**
- Single endpoint `/mcp` with method-based routing (following rust-sdk pattern)
  - `POST /mcp` — JSON-RPC requests with SSE stream response, session ID in header
  - `GET /mcp` — SSE stream reconnection with `Last-Event-ID` and `Session-ID` headers
  - `DELETE /mcp` — Session close
- Accept header validation: `text/event-stream` for GET, `application/json` + `text/event-stream` for POST

**Session management:**
- Server-generated session IDs (UUID or similar)
- Session ID returned in response header after first POST
- Client includes `Session-ID` header on reconnect (GET)
- `Last-Event-ID` header for resumable streams

**Reconnection:**
- Automatic reconnection with exponential backoff (following rust-sdk's `SseAutoReconnectStream`)
- Configurable retry policy: FixedInterval, ExponentialBackoff, NeverRetry
- Default: Exponential backoff with infinite max retries
- Server-directed retry via SSE `retry:` field (use max(server_retry, client_policy_retry))
- Reconnection state machine: Connected → Retrying → WaitingNextRetry → Connected/Terminated

**Keep-alive:**
- SSE heartbeat comments (`: ping\n\n`) every 15 seconds (following rust-sdk default)
- Priming event with `retry:` field (3 seconds default) for client reconnection guidance

**HTTP configuration:**
- Default keep-alive: 15 seconds
- Default retry interval: 3 seconds
- Connect timeout: 30 seconds
- Idle timeout: 120 seconds

### Subscription Model

**Routing:**
- Token-based routing (string tokens) — matches progress token pattern from Phase 3
- Tokens used as routing keys in HashMap/unordered_map

**API style:**
- Callback-based (`std::function`) for consistency with existing mcpp async patterns
- Stream-like iterator API (C++20 coroutines) noted as future enhancement

**Cleanup:**
- Hybrid approach: RAII + manual
  - Subscriber destructor auto-unsubscribes (RAII)
  - `unsubscribe()` method for explicit early cleanup

**Buffering:**
- Unbounded queue for notifications

**Channel size:**
- 16-item buffer for notification channels (following rust-sdk's ProgressDispatcher)

### Claude's Discretion

**SSE Transport:**
- Exact backpressure handling strategy
- Connection pooling for multiple SSE streams
- SSL/TLS configuration details for HTTPS+SSE

**Streaming Semantics:**
- Chunk granularity for tool result streaming
- Stream completion signaling mechanism
- Backpressure handling between producer and consumer

**Subscriptions:**
- Exact channel implementation (lock-free queue vs mutex-protected)
- Notification drop policy when queue is full (unbounded still needs limits)

**Extensibility Pattern:**
- How experimental capabilities integrate with existing API surface
- Capability negotiation protocol details

**Resource Templates & Completions:**
- URI template expansion syntax (RFC 6570 or simpler)
- Completion trigger conditions (on-change, on-demand)

**Tool Annotations & Structured Output:**
- Annotation metadata schema
- JSON Schema validation library choice

## Specific Ideas

- "Follow rust-sdk and gopher-mcp patterns where they exist — they've solved these problems"
- Single `/mcp` endpoint is cleaner than separate `/rpc` and `/events` endpoints
- Session IDs should be server-generated for security and simplicity
- Auto-reconnect should be opt-out (enabled by default) for better UX

## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 04-advanced-features-http-transport*
*Context gathered: 2026-01-31*
