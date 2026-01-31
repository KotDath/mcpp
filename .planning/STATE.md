# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2025-01-31)

**Core value:** Developers can build MCP clients and servers in C++ that are fast, correct, and support the complete protocol spec without wrestling with JSON-RPC details or transport plumbing
**Current focus:** Advanced Features & HTTP Transport (Phase 4)

## Current Position

Phase: 4 of 7 (Advanced Features & HTTP Transport)
Plan: 1 of 6 in current phase
Status: In progress
Last activity: 2026-01-31 — Completed 04-01: SSE formatter and RFC 6570 URI template expander utilities

Progress: [█████░░░░░] 17%

## Performance Metrics

**Velocity:**
- Total plans completed: 24
- Average duration: 4 min
- Total execution time: 1.6 hours

**By Phase:**

| Phase | Plans | Complete | Avg/Plan |
|-------|-------|----------|----------|
| 01-protocol-foundation | 6 | 6 | 6 min |
| 02-core-server | 6 | 6 | 2 min |
| 03-client-capabilities | 8 | 8 | 3 min |
| 04-advanced-features--http-transport | 6 | 1 | 2 min |

**Recent Trend:**
- Last 1 plan: 04-01 (SSE formatter, URI template expander)
- Trend: Started Phase 4 with HTTP transport foundation utilities

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

**From 03-02 (Roots Management):**
- file:// URI validation using uri.rfind("file://", 0) == 0 per MCP spec
- RootsManager with callback pattern for list_changed notifications
- roots/list handler registered in McpClient constructor
- RootsManager accessible via get_roots_manager() for user code

**From 03-03 (Basic Sampling):**
- ContentBlock as std::variant for extensibility (future ImageContent, AudioContent)
- SamplingHandler as std::function for flexible LLM provider integration
- Content parsing supports string shorthand for text content
- CreateMessageRequest with from_json parsing, max_tokens validation
- CreateMessageResult with to_json serialization
- SamplingClient with handler registration and JSON-RPC error returns
- sampling/createMessage handler registered in McpClient constructor

**From 03-01 (Cancellation Support):**
- std::stop_token over atomic<bool> for better composability with std::jthread
- Separate CancellationManager from RequestTracker for cleaner separation of concerns
- Idempotent unregister operations handle race conditions (cancel after completion)
- CancellationToken for polling cancellation state in long-running operations
- CancellationSource for requesting cancellation via cancel() method
- CancellationManager tracks pending requests by RequestId with mutex protection
- notifications/cancelled handler registered in McpClient constructor
- cancel_request() public method for user-initiated cancellation

**From 03-04 (Sampling with Tool Use):**
- Tool types (Tool, ToolChoice) added for agentic sampling loops
- ToolResultContent for structured tool return values with isError flag
- CreateMessageRequest extended with optional tools and tool_choice parameters
- CreateMessageResult extended with toolUseContent for multi-step tool loops
- SamplingClient::handle_create_message_with_tools for tool loop execution
- Maximum of 10 tool iterations to prevent infinite loops

**From 03-05 (Elicitation Support):**
- PrimitiveSchema restricted to top-level properties only per MCP spec (no nested objects)
- Form mode synchronous (immediate ElicitResult) vs URL mode asynchronous (notification completes)
- Variant-based request discrimination using std::variant<ElicitRequestForm, ElicitRequestURL>
- ElicitationClient with pending_url_requests_ map for URL mode correlation by elicitation_id
- ElicitationCapability added to protocol types with form and url flags
- elicitation/create request handler registered in McpClient constructor
- notifications/elicitation/complete notification handler registered in McpClient constructor

**From 03-06 (std::future Wrappers):**
- shared_ptr<promise> pattern ensures promise lifetime until lambda callback invoked
- FutureBuilder template with wrap() converts callback-style async to std::future
- with_timeout() prevents infinite blocking (30-second default)
- McpClientBlocking holds reference (not ownership) to McpClient for lifecycle control
- Exception conversion: JsonRpcError.message stored as runtime_error in promise
- McpClientBlockingError provides typed exceptions with JSON-RPC error codes
- Added ListRootsResult::from_json() for parsing JSON responses in blocking context

**From 03-07 (Build Configuration Gap Closure):**
- Header organization: client/* headers grouped together in MCPP_PUBLIC_HEADERS for logical subsystem organization
- client_blocking.h placement: at top level (not client/) since it's the main public blocking API
- CMakeLists.txt must be updated when new source files are added to prevent linker errors
- Files grouped by subsystem directory structure (client/, core/, async/, etc.)

**From 03-08 (ClientCapabilities Builder):**
- Nested Builder class (ClientCapabilities::Builder) for fluent API capability configuration
- with_* prefix for builder methods following common fluent API conventions
- Ref-qualified build() methods (build() && for rvalue, build() const & for lvalue) for optimal move semantics
- Convenience free function build_client_capabilities() for simple boolean-based configuration
- Constructors added to RootsCapability, SamplingCapability for easy initialization

**From 04-01 (HTTP Transport Foundation Utilities):**
- Header-only SseFormatter class with static methods for text/event-stream formatting
- Header-only UriTemplate class with RFC 6570 Level 1-2 template expansion
- Inline implementation instead of external SSE/URI template library dependencies
- Reserve-style path expansion (preserves /, :, @) for proper file:// URI handling
- mcpp::util namespace established for utility classes

### Pending Todos

[From .planning/todos/pending/ — ideas captured during sessions]

None yet.

### Blockers/Concerns

[Issues that affect future work]

**None currently.** Previous verification gap (transport nullptr dereference) was auto-fixed during Phase 2 execution (commit 20178a3).

## Session Continuity

Last session: 2026-01-31
Stopped at: Completed 04-01 (SSE formatter and URI template expander). Auto-fixed path encoding bug in URI template expansion.
Resume file: None
