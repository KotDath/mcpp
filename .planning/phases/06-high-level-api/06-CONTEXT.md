# Phase 6: High-Level API - Context

**Gathered:** 2026-02-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Type-safe, RAII-based high-level wrapper API with full thread safety, logging support, and utility features for production use. This phase delivers ergonomic developer-facing APIs on top of the low-level callback-based async core from previous phases.

**Scope anchor:** Developer API surface, concurrency model, logging integration, helper utilities — NOT new protocol features (those belong in earlier phases).

</domain>

<decisions>
## Implementation Decisions

### API Surface Design
Based on rust-sdk and gopher-mcp patterns:

- **Role-based typing**: `RoleClient` and `RoleServer` marker types for compile-time role distinction
- **Generic Service pattern**: `Service<Role>` trait abstraction for protocol handling
- **Entry points**: `serve_client()`, `serve_server()` with cancellation token variants
- **Handler traits**: Separate `ClientHandler` and `ServerHandler` with default implementations
- **Wrapper types**: Support smart pointer wrappers (shared_ptr, unique_ptr) for handlers
- **Layered architecture**: Clear separation between transport, protocol, and business logic layers
- **Interface-based design**: Extensive use of abstract interfaces for polymorphism and testability

### Thread Safety Model
Based on rust-sdk Arc/Mutex and gopher-mcp patterns:

- **Async-first**: Non-blocking operations throughout with std::future/std::async
- **Shared state**: `std::shared_ptr` for Arc equivalent, `std::shared_mutex` for RWMutex equivalent
- **Atomic ID provider**: `AtomicRequestIdProvider` using `std::atomic<uint32_t>` for request IDs
- **Peer state**: `Peer<Role>` encapsulating shared connection state with mutex protection
- **Cancellation tokens**: `std::stop_token` over atomic<bool> for better composability
- **Lock hierarchy**: Strict ordering — chain locks before filter locks (gopher-mcp pattern)
- **State flags**: `std::atomic<bool>` or `std::atomic<Value>` for state management
- **Context propagation**: Request-scoped context with thread-safe property map

### Logging Integration
Based on rust-sdk tracing and gopher-mcp LoggingFilter:

- **Structured logging**: Logger class with Level enum (Trace, Debug, Info, Warn, Error)
- **Context-aware logging**: Log spans with contextual data (method, request_id, transport_type)
- **Configurable sinks**: Pluggable log sink interface (console, file, custom)
- **Payload control**: Optional payload logging with size limits to avoid log spam
- **Error context logging**: Errors include contextual information (transport context, request ID)
- **Pre-handshake handling**: Graceful handling of log messages before MCP handshake
- **Runtime enable/disable**: Logging can be toggled without restart

### Utility Features
Based on both SDKs' helper patterns:

- **Error hierarchy**: Unified error types (ServiceError, TransportError) with context preservation
- **Timeout management**: Default 5-minute timeout (300s) with per-operation override capability
- **Capability negotiation**: Protocol version negotiation, feature-gated functionality
- **Pagination helpers**: `list_all<T>()` template for automatic cursor-based pagination
- **Statistics collection**: Built-in metrics tracking with atomic updates (bytes processed, error count, timing)
- **Retry logic**: Multiple backoff strategies (exponential, linear, jitter) with configurable conditions
- **Resource management**: RAII pattern for proper cleanup and resource lifetime management

### Claude's Discretion
- Exact mutex placement and lock granularity decisions
- Specific logging sink implementations
- Choice between std::format vs fmtlib for log formatting
- Exact timer implementation for timeout tracking
- Pool allocation strategies (object pools) if performance indicates need

</decisions>

<specifics>
## Specific Ideas

**Design references:**
- rust-sdk at /home/kotdath/omp/personal/cpp/mcpp/thirdparty/rust-sdk/ — follow Role types, Service trait, Arc patterns
- gopher-mcp at /home/kotdath/omp/personal/cpp/mcpp/thirdparty/gopher-mcp/ — follow filter chain patterns, context embedding

**Key patterns to mirror:**
```cpp
// Role-based typing (rust-sdk pattern)
struct RoleClient {};
struct RoleServer {};

// Thread-safe ID provider
class AtomicRequestIdProvider {
    std::atomic<uint32_t> next_id_{1};
public:
    uint32_t next() { return next_id_.fetch_add(1, std::memory_order_relaxed); }
};

// Structured logging (rust-sdk tracing pattern)
class Logger {
public:
    enum class Level { Trace, Debug, Info, Warn, Error };
    void log(Level level, const std::string& message,
            const std::map<std::string, std::string>& context = {});
};

// Pagination helper (rust-sdk pattern)
template<typename T>
std::vector<T> list_all(std::function<Result<Page<T>>(std::optional<std::string>)> list_fn);
```

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope. User explicitly requested following rust-sdk/gopher-mcp patterns.

</deferred>

---

*Phase: 06-high-level-api*
*Context gathered: 2026-02-01*
