# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2025-01-31)

**Core value:** Developers can build MCP clients and servers in C++ that are fast, correct, and support the complete protocol spec without wrestling with JSON-RPC details or transport plumbing
**Current focus:** High-Level API (Phase 6)

## Current Position

Phase: 6 of 7 (High-Level API)
Plan: 4 of TBD in current phase
Status: In progress
Last activity: 2026-02-01 — Completed 06-04 (Pagination Helpers and Unified Error Hierarchy)

Progress: [█████████░] 78%

## Performance Metrics

**Velocity:**
- Total plans completed: 36
- Average duration: 3 min
- Total execution time: 1.9 hours

**By Phase:**

| Phase | Plans | Complete | Avg/Plan |
|-------|-------|----------|----------|
| 01-protocol-foundation | 6 | 6 | 6 min |
| 02-core-server | 6 | 6 | 2 min |
| 03-client-capabilities | 8 | 8 | 3 min |
| 04-advanced-features--http-transport | 6 | 6 | 2 min |
| 05-content---tasks | 4 | 4 | 3 min |
| 06-high-level-api | 4 | 4 | 3 min |

**Recent Trend:**
- Last 5 plans: 05-04, 06-01, 06-02, 06-03, 06-04
- Trend: Phase 6 progressing with service foundation, message passing, logging, and utility helpers.

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

**From 04-02 (Streamable HTTP Transport):**
- HttpTransport class implementing Transport interface for POST/SSE pattern
- Single endpoint design: POST for client->server, GET for server->client (SSE)
- Session management via Mcp-Session-Id header with UUID v4 secure session IDs
- User-provided HTTP server integration (HttpResponseAdapter, HttpSseWriterAdapter)
- Non-blocking I/O: send() buffers, handle_post returns immediately
- 30-minute session timeout with automatic cleanup
- SseFormatter used for SSE event formatting (format_event, content_type, etc.)
- Template-based adapter pattern supports any HTTP server (cpp-httplib, drogon, oat++)

**From 04-03 (Tool Metadata and Output Validation):**
- ToolAnnotations struct with destructive, read_only, audience, priority fields
- Output schema validation using nlohmann/json_schema_validator
- Backward compatible API via overloaded register_tool method
- Extended tool discovery includes annotations and outputSchema fields
- Output validation failures return CallToolResult with isError=true (not JSON-RPC error)

**From 04-04 (Resource Templates and Subscriptions):**
- TemplateResourceHandler type receiving expanded URI and extracted parameters
- TemplateResourceRegistration struct with URI template pattern and parameter names
- register_template() for parameterized resource registration
- Regex-based template matching for URI-to-template resolution
- Subscription tracking via unordered_map<uri, vector<Subscription>>
- subscribe/unsubscribe methods for resource change monitoring
- notify_updated() to send notifications/resources/updated via transport
- Non-owning transport pointer following RequestContext pattern
- Resource list includes templates with template field per MCP spec

**From 04-05 (Argument Completion):**
- Completion struct with value and optional description for autocompletion suggestions
- CompletionHandler function type taking argument_name, current_value, and optional reference
- Completion handlers stored in unordered_map keyed by prompt/resource name
- Empty completion array returned when no handler registered (graceful degradation)
- Parameter validation returns INVALID_PARAMS error for missing required fields
- McpServer routes prompts/complete and resources/complete requests to registries
- Shared Completion struct defined in both registries for module boundary clarity

**From 04-06 (Tool Result Streaming):**
- send_stream_result method for incremental tool results via RequestContext
- is_streaming/set_streaming methods for streaming mode control
- SSE formatting via SseFormatter for HTTP transport compatibility
- Graceful no-op when no progress token available (degradation)
- Progress notifications work independently of streaming mode
- Compatible with both stdio and HTTP transports

**From 05-01 (Rich Content Type Support):**
- ContentBlock variant extended with 7 content types (TextContent, ImageContent, AudioContent, ResourceLink, EmbeddedResource, ToolUseContent, ToolResultContent)
- Annotations struct with audience (vector<string>), priority (double 0-1), last_modified (ISO 8601 string)
- Base64 encoding is caller's responsibility - data field stores pre-encoded base64 strings
- Content conversion implemented in sampling.cpp to avoid nlohmann::to_json naming conflicts
- Forward declaration for server::ResourceContent used in content.h to avoid circular dependency

**From 05-02 (Cursor-Based Pagination):**
- PaginatedResult<T> template with items, nextCursor, total, and has_more() method
- Offset-based cursor encoding: Simple string representation of integer offset
- PAGE_SIZE = 50: Server-determined page size for consistent pagination
- Helper functions for JSON conversion shared between list_*() and list_*_paginated()
- Header-only template implementation for PaginatedResult
- list_*_paginated() methods added to ToolRegistry, ResourceRegistry, and PromptRegistry
- Original list_*() methods remain unchanged for backward compatibility

**From 05-03 (List Changed Notifications):**
- NotifyCallback pattern following RootsManager implementation (std::function<void()>)
- Client capabilities stored from initialize for notification gating
- Registry callbacks check experimental capabilities for listChanged before sending
- Transport availability checked before sending notifications
- All three registries (tools, resources, prompts) support list_changed notifications
- Automatic notification on successful registration via notify_changed() call

**From 06-01 (High-Level API Foundation):**
- Role marker types (RoleClient, RoleServer) with IS_CLIENT constexpr bool for compile-time role distinction
- ServiceRole concept using std::same_as for simple role validation (C++20 concepts over SFINAE)
- memory_order_relaxed for atomic ID generation - strict ordering not required for uniqueness
- std::shared_mutex for peer_info access - concurrent reads with exclusive writes
- Non-copyable/non-movable types for ID provider and Peer - prevents accidental ID collisions
- Stub send_request/send_notification return std::future - establishes async pattern for 06-02
- RoleTypes trait for role-specific type aliases (PeerReq, PeerResp, PeerNot, Info, PeerInfo)
- Service trait as pure virtual interface for polymorphic handlers
- Experimental capabilities field in ClientInfo/ServerInfo using std::map<std::string, nlohmann::json> (UTIL-03)

**From 06-02 (RunningService & Message Passing):**
- Message passing channel with std::variant<NotificationMessage, RequestMessage> for type-safe dispatch
- shared_ptr<promise> pattern ensures promise lifetime until callback is invoked
- std::condition_variable_any for blocking wait with std::stop_token support
- std::jthread for automatic background thread cleanup on destruction (RAII)
- steady_clock for timeout tracking (immune to system time changes)
- 5-minute default timeout with progress notifications resetting the clock (UTIL-02)
- Message queue with mutex + condition_variable for thread-safe mpsc communication
- Future-based async API with std::future return types for request/response pattern
- QuitReason enum for service termination cause tracking

**From 06-03 (Structured Logging & Request Context):**
- Logger class with structured logging and level filtering (Trace/Debug/Info/Warn/Error)
- Logger::Span nested class for automatic request duration tracking with RAII
- Thread-safe singleton with std::call_once initialization
- spdlog backend with automatic stderr fallback when spdlog not available
- RequestContext template with thread-safe property access via std::shared_mutex
- NotificationContext template for notification metadata tracking
- Runtime log level changes and payload logging toggle without restart

**From 06-04 (Pagination Helpers and Unified Error Hierarchy):**
- list_all<T> template function for automatic cursor pagination across all pages
- PaginatedRequest struct with cursor and optional limit parameters
- ServiceError base class inheriting std::runtime_error for catch compatibility
- TransportError for transport-layer failures (connection lost, timeout)
- ProtocolError for protocol violations (invalid JSON, unexpected message type)
- RequestError for request-specific errors (invalid params, method not found)
- Context preservation via std::map<std::string, std::string> for debugging metadata
- Header-only template for pagination; separate .cpp for error type implementations

### Pending Todos

[From .planning/todos/pending/ — ideas captured during sessions]

None yet.

### Blockers/Concerns

[Issues that affect future work]

**Pre-existing json-schema dependency:** The code has a hard dependency on nlohmann/json-schema-validator which is not available. Made the include optional with __has_include, but full resolution requires ToolRegistration refactoring to make validators optional. This is outside the scope of Phase 6.

## Session Continuity

Last session: 2026-02-01
Stopped at: Completed 06-04 (Pagination Helpers and Unified Error Hierarchy)
Resume file: None
- list_all<T> template function for automatic cursor pagination
- ServiceError hierarchy with TransportError, ProtocolError, RequestError
- Context preservation via std::map<std::string, std::string> for debugging
- All error types inherit std::runtime_error for catch block compatibility
