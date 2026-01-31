# Phase 6: High-Level API - Research

**Researched:** 2026-02-01
**Domain:** Modern C++ (C++20), Thread Safety, RAII, Structured Logging, Utility Patterns
**Confidence:** HIGH

## Summary

This phase focuses on building type-safe, RAII-based high-level wrapper APIs with full thread safety, logging support, and utility features for production use. The research examined rust-sdk and gopher-mcp reference implementations, modern C++ concurrency patterns, structured logging libraries, and industry best practices for thread-safe API design.

**Primary recommendation:** Follow the rust-sdk's Service/Peer pattern with C++20 std::stop_token for cancellation, std::shared_ptr + std::shared_mutex for shared state, and spdlog-based structured logging. Build RAII wrapper types on top of existing callback-based core from Phases 1-5.

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| **C++20 stdlib** | C++20 | Language features | std::stop_token, std::jthread, concepts, coroutines foundation |
| **spdlog** | 1.14+ | Structured logging | Fast, header-only, thread-safe, fmtlib-based formatting |
| **nlohmann/json** | 3.11+ | JSON (already in use) | Already in codebase, excellent ergonomics |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| **std::shared_mutex** | C++17 | Reader-writer lock | Multi-reader, single-writer shared state (Peer state) |
| **std::atomic** | C++11 | Lock-free state | ID provider, flags, metrics counters |
| **std::stop_token** | C++20 | Cancellation | Cooperative cancellation (already in use via cancellation.h) |
| **std::shared_ptr** | C++11 | Shared ownership | Arc equivalent for shared handler/peer state |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| spdlog | glog, Quill, custom | spdlog is faster than glog, simpler than Quill, widely adopted |
| std::shared_mutex | std::mutex only | shared_mutex allows concurrent reads |
| std::stop_token | atomic<bool> | stop_token is composable, standard, jthread-compatible |

**Installation:**
```bash
# Already in project for nlohmann/json
# spdlog should be added:
# CMakeLists.txt: find_package(spdlog REQUIRED) or FetchContent
```

## Architecture Patterns

### Recommended Project Structure

```
src/mcpp/
├── api/                    # High-level API wrappers
│   ├── service.h           # Service<Role> trait abstraction
│   ├── peer.h              # Peer<Role> for connection handling
│   ├── role.h              # RoleClient, RoleServer marker types
│   └── running_service.h   # RAII running service wrapper
├── util/                   # Utility features
│   ├── logger.h            # Structured logging wrapper
│   ├── error.h             # Unified error types
│   ├── retry.h             # Retry/backoff strategies
│   ├── metrics.h           # Statistics collection
│   └── pagination.h        # (already exists - extend)
└── thread/                 # Thread-safe primitives
    ├── atomic_id_provider.h
    └── lock_guard.h        # RAII lock helpers
```

### Pattern 1: Role-Based Type Safety (rust-sdk)

**What:** Marker types for compile-time role distinction

**When to use:** When APIs differ between client and server roles

**Example:**
```cpp
// Source: rust-sdk/src/service.rs (adapted to C++)
struct RoleClient {};
struct RoleServer {};

template<typename Role>
concept ServiceRole = std::same_as<Role, RoleClient> ||
                       std::same_as<Role, RoleServer>;

// Role-specific types are selected at compile time
template<ServiceRole Role>
class Peer {
    // Methods available only for specific roles
    void send_request(...) requires std::same_as<Role, RoleClient>;
};
```

### Pattern 2: Service Trait Abstraction (rust-sdk)

**What:** Polymorphic handler interface with role-based request/response types

**When to use:** For defining client and server handlers with type safety

**Example:**
```cpp
// Source: rust-sdk/src/service.rs (adapted to C++)
template<ServiceRole Role>
class Service {
public:
    virtual void handle_request(
        const typename Role::PeerReq& request,
        RequestContext<Role>& ctx
    ) = 0;

    virtual void handle_notification(
        const typename Role::PeerNot& notification,
        NotificationContext<Role>& ctx
    ) = 0;

    virtual typename Role::Info get_info() const = 0;
};
```

### Pattern 3: RAII Running Service (rust-sdk RunningService)

**What:** RAII wrapper that owns background task and ensures cleanup

**When to use:** For any service that spawns a background message loop

**Example:**
```cpp
// Source: rust-sdk/src/service.rs (lines 434-548)
template<ServiceRole Role, Service<Role> S>
class RunningService {
public:
    ~RunningService() {
        // DropGuard ensures cancellation on destruction
        if (handle_ && !cancellation_token_.is_cancelled()) {
            // Log warning if not explicitly closed
            logger_.log(Logger::Level::Debug,
                       "RunningService dropped without explicit close()");
        }
    }

    // Explicit close with timeout
    std::optional<QuitReason> close_with_timeout(
        std::chrono::milliseconds timeout
    );

    // Deref to Peer for convenient access
    Peer<Role>* operator->() { return &peer_; }

private:
    std::shared_ptr<S> service_;
    Peer<Role> peer_;
    std::optional<std::jthread> handle_;  // RAII join via jthread
    std::stop_token cancellation_token_;
};
```

### Pattern 4: Thread-Safe Peer State (rust-sdk + gopher-mcp)

**What:** Shared connection state protected by mutex with lock hierarchy

**When to use:** For state shared between message loop and API calls

**Example:**
```cpp
// Source: rust-sdk/src/service.rs (lines 312-347) + gopher-mcp context
template<ServiceRole Role>
class Peer {
public:
    // Thread-safe request sending
    std::future<typename Role::PeerResp> send_request(
        const typename Role::Req& request
    );

    // Thread-safe notification sending
    void send_notification(const typename Role::Not& notification);

    // Access peer info (protected by mutex)
    std::optional<typename Role::PeerInfo> peer_info() const {
        std::shared_lock lock(mutex_);
        return peer_info_;
    }

private:
    mutable std::shared_mutex mutex_;  // For peer_info_
    std::optional<typename Role::PeerInfo> peer_info_;

    // Channel for message passing to event loop
    mpsc::Sender<PeerMessage<Role>> tx_;
    std::shared_ptr<AtomicRequestIdProvider> id_provider_;
};
```

### Pattern 5: Structured Logging with Context (rust-sdk tracing + gopher-mcp)

**What:** Hierarchical logging with request-scoped context

**When to use:** For all production logging

**Example:**
```cpp
// Source: rust-sdk tracing + gopher-mcp LoggingFilter
class Logger {
public:
    enum class Level { Trace, Debug, Info, Warn, Error };

    // Log with contextual key-value pairs
    void log(Level level, std::string_view message,
             const std::map<std::string, std::string>& context = {});

    // Create a scoped span for request tracking
    class Span {
    public:
        Span(std::string_view name, const Context& parent);
        ~Span();  // Automatically logs duration

        void add_context(std::string_view key, std::string_view value);
    };

    // Runtime enable/disable
    void set_level(Level level);
    void enable_payload_logging(bool enable, size_t max_size = 1024);
};
```

### Anti-Patterns to Avoid

- **Blocking in event loop:** Never block the message loop with I/O or long computations
- **Lock inversion:** Always acquire locks in consistent order (chain locks before filter locks)
- **Callback after free:** Ensure RunningService keeps service alive via shared_ptr
- **Logging before handshake:** Handle log messages that arrive before initialize completes
- **Throwing from destructors:** Destructors should never throw (RAII principle)

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Structured logging | Custom printf | spdlog | Thread-safe, fast, formatted, multiple sinks |
| Thread-safe ID generation | mutex + counter | std::atomic<uint32_t> with fetch_add | Lock-free, faster, simpler |
| RAII thread join | Custom wrapper | std::jthread | Standard, composable with stop_token |
| Cancellation | atomic<bool> | std::stop_token | Composable, jthread integration |
| Request timeout tracking | Custom timer | std::stop_token + std::jthread | Standard, cooperative |

**Key insight:** C++20 provides most primitives needed. Use standard library components over custom implementations.

## Common Pitfalls

### Pitfall 1: Data Races on Shared State

**What goes wrong:** Multiple threads accessing peer_info_ or handler state without synchronization

**Why it happens:** Shared state accessed from both API calls and event loop

**How to avoid:**
- Use std::shared_mutex for multi-reader single-writer state
- Use std::atomic for simple flags and counters
- Document lock ordering requirements

**Warning signs:** Valgrind/helgrind errors, intermittent crashes, "impossible" state

### Pitfall 2: Deadlock from Lock Inversion

**What goes wrong:** Thread A holds lock X and waits for Y, Thread B holds Y and waits for X

**Why it happens:** Inconsistent lock acquisition order

**How to avoid:**
- Define lock hierarchy: always acquire chain locks before filter locks
- Use std::scoped_lock for multiple lock acquisition (ordered by address)
- Keep lock scopes minimal

**Warning signs:** Application hangs, gdb shows threads stuck in mutex lock

### Pitfall 3: Use-After-Free with Callbacks

**What goes wrong:** Callback captures pointer to destroyed object

**Why it happens:** RunningService destroyed while event loop still running

**How to avoid:**
- RunningService stores std::shared_ptr<S> not raw pointer
- Stop event loop before destroying service
- Use std::weak_ptr for breaking cycles

**Warning signs:** Segfaults, valgrind invalid reads, corrupted heap

### Pitfall 4: Logging Performance Degradation

**What goes wrong:** Excessive logging slows down message processing

**Why it happens:** Logging in tight loops, large payload logging

**How to avoid:**
- Log level filtering at call site (if constexpr for compile-time)
- Limit payload logging size (truncate at 1KB)
- Use async logging (spdlog async mode)

**Warning signs:** Latency spikes, CPU usage increases with log level

### Pitfall 5: Stop Token Not Propagated

**What goes wrong:** Cancellation requested but long-running operation doesn't stop

**Why it happens:** stop_token not passed to nested operations

**How to avoid:**
- Always pass stop_token through the call chain
- Check stop_requested() in loops
- Use std::jthread which automatically passes stop_token

**Warning signs:** Operations don't respect cancellation, slow shutdown

## Code Examples

### Role-Based Service Entry Points

```cpp
// Source: rust-sdk/src/service/server.rs (serve_server pattern)
template <Service<RoleServer> S>
RunningService<RoleServer, S> serve_server(
    std::shared_ptr<S> service,
    std::unique_ptr<Transport> transport
) {
    // Perform handshake
    auto [peer, server_info] = handshake(*transport, *service);

    // Start event loop with RAII wrapper
    return RunningService<RoleServer, S>(
        std::move(service),
        std::move(transport),
        std::move(peer)
    );
}

// Usage:
auto server = std::make_shared<MyServerHandler>();
auto running = serve_server(server, std::move(transport));
// running owns the event loop, cleans up on destruction
```

### Thread-Safe ID Provider

```cpp
// Source: rust-sdk/src/service.rs (AtomicU32Provider)
class AtomicRequestIdProvider {
public:
    uint32_t next_id() noexcept {
        return next_id_.fetch_add(1, std::memory_order_relaxed);
    }

private:
    std::atomic<uint32_t> next_id_{1};
};
```

### Pagination Helper

```cpp
// Source: rust-sdk/src/service/client.rs (list_all pattern)
template <typename T>
std::vector<T> list_all(auto&& list_fn) {
    std::vector<T> items;
    std::optional<std::string> cursor;

    do {
        auto page = list_fn(cursor);
        items.insert(items.end(),
                    std::make_move_iterator(page.items.begin()),
                    std::make_move_iterator(page.items.end()));
        cursor = page.next_cursor;
    } while (cursor);

    return items;
}

// Usage:
auto all_tools = list_all<Tool>([&](auto cursor) {
    return peer->list_tools(PaginatedRequestParams{cursor});
});
```

### Structured Logging with Spans

```cpp
// Source: rust-sdk tracing pattern adapted to C++ spdlog
class Logger {
public:
    class Span {
    public:
        Span(Logger& logger, std::string_view name,
             std::map<std::string, std::string> context = {})
            : logger_(logger), name_(name) {
            logger_.log(Level::Debug,
                       fmt::format("Enter: {}", name_), context);
            start_ = std::chrono::steady_clock::now();
        }

        ~Span() {
            auto elapsed = std::chrono::steady_clock::now() - start_;
            logger_.log(Level::Debug,
                       fmt::format("Exit: {} ({}ms)", name_,
                                  elapsed.count()),
                       context_);
        }

    private:
        Logger& logger_;
        std::string name_;
        std::chrono::steady_clock::time_point start_;
        std::map<std::string, std::string> context_;
    };

    void log(Level level, std::string_view msg,
             const std::map<std::string, std::string>& ctx = {});
};
```

### Retry with Exponential Backoff

```cpp
// Source: gopher-mcp/sdk/go/src/filters/retry.go
template<typename T>
class RetryPolicy {
public:
    virtual std::chrono::milliseconds next_delay(int attempt) const = 0;
    virtual bool should_retry(const std::exception& e) const = 0;
};

class ExponentialBackoff : public RetryPolicy<int> {
public:
    std::chrono::milliseconds next_delay(int attempt) const override {
        auto delay = initial_delay_ * std::pow(multiplier_, attempt - 1);
        delay = std::min(delay, max_delay_);
        return std::chrono::milliseconds(static_cast<long>(delay));
    }

private:
    double initial_delay_ = 1000;      // 1 second
    double multiplier_ = 2.0;
    double max_delay_ = 30000;         // 30 seconds
};

template<typename T>
Result<T> retry_with_backoff(
    std::function<Result<T>()> fn,
    const RetryPolicy<T>& policy,
    int max_attempts = 3
) {
    Result<T> last_error;
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        try {
            return fn();
        } catch (const std::exception& e) {
            last_error = Result<T>::error(e);
            if (attempt < max_attempts && policy.should_retry(e)) {
                auto delay = policy.next_delay(attempt);
                std::this_thread::sleep_for(delay);
            }
        }
    }
    return last_error;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual thread join | std::jthread RAII | C++20 | No more thread leaks |
| atomic<bool> cancellation | std::stop_token | C++20 | Composable cancellation |
| std::mutex everywhere | std::shared_mutex for reads | C++17 | Better read concurrency |
| printf-style logging | Structured logging (spdlog) | 2020s | Searchable, contextual logs |
| Custom retry | Backoff strategies | 2020s | Better resilience |

**Deprecated/outdated:**
- **std::thread** (prefer std::jthread for automatic join)
- **printf for logging** (no structured context, not thread-safe)
- **Manual RAII wrappers** (use standard library equivalents)
- **Lock-free patterns for simple state** (std::atomic is sufficient, no need for atomics library)

## Open Questions

### C++20 Coroutines Integration

**What we know:** rust-sdk uses Rust async/await. C++20 has coroutines.

**What's unclear:** Whether to use coroutines for this phase or stay with callback-based core.

**Recommendation:** Stay with callbacks for core (already implemented from Phase 1). Consider coroutines as a future enhancement in a separate phase. The high-level wrappers can provide coroutine adapters if needed.

### fmtlib vs std::format

**What we know:** spdlog uses fmtlib for formatting. C++23 has std::format.

**What's unclear:** Whether to wait for std::format or use fmtlib now.

**Recommendation:** Use fmtlib (comes with spdlog). It's mature, fast, and std::format isn't widely available yet. Can switch to std::format in future when C++23 adoption is widespread.

### Async Logging Strategy

**What we know:** spdlog supports async logging via thread pool.

**What's unclear:** Whether async logging is needed or if synchronous is sufficient.

**Recommendation:** Start with synchronous logging for simplicity. Add async logging if profiling shows log I/O is a bottleneck. Use spdlog's async logger when ready.

## Sources

### Primary (HIGH confidence)

- **rust-sdk source code** at `/home/kotdath/omp/personal/cpp/mcpp/thirdparty/rust-sdk/`
  - `/crates/rmcp/src/service.rs` - Service trait, Peer, RunningService patterns
  - `/crates/rmcp/src/service/server.rs` - ServerHandler, serve_server implementation
  - `/crates/rmcp/src/service/client.rs` - ClientHandler, serve_client implementation, pagination
  - `/crates/rmcp/src/handler/server.rs` - Handler trait with default implementations
  - `/crates/rmcp/src/error.rs` - Unified error types (ServiceError, McpError)

- **gopher-mcp source code** at `/home/kotdath/omp/personal/cpp/mcpp/thirdparty/gopher-mcp/`
  - `/sdk/go/src/filters/logging.go` - LoggingFilter with level control, payload limits
  - `/sdk/go/src/filters/retry.go` - Retry with exponential/linear backoff, jitter strategies
  - `/sdk/go/src/core/context.go` - ProcessingContext with thread-safe property map
  - `/sdk/go/src/integration/filtered_client.go` - FilteredMCPClient with mutex protection

- **Existing mcpp codebase** at `/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/`
  - `/client.h` - Callback-based client (foundation for high-level wrappers)
  - `/client_blocking.h` - Blocking wrapper pattern (RAII reference)
  - `/client/cancellation.h` - std::stop_token usage pattern
  - `/server/request_context.h` - RequestContext pattern
  - `/content/pagination.h` - PaginatedResult already implemented

### Secondary (MEDIUM confidence)

- **Modern C++ concurrency articles (web search verified)**
  - "Cooperative Interruption of a Thread in C++20" - std::stop_token patterns
  - "Mastering C++ Cooperative Cancellation" - Deep dive on stop_token/jthread
  - "RAII in Modern C++" - RAII best practices
  - "C++ Core Guidelines" - Thread safety guidelines

- **Logging research (web search verified)**
  - "Logging in C++: Lessons from Three Decades" - Best practices, pitfalls
  - spdlog GitHub issues - JSON logging patterns
  - "Top C++ Logging Libraries Compared" - spdlog vs alternatives

### Tertiary (LOW confidence)

- Generic C++ articles (marked for validation if referenced in planning)
- StackOverflow discussions (verify with official docs)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All libraries are standard, well-documented
- Architecture: HIGH - Based on working reference implementations (rust-sdk, gopher-mcp) directly analyzed
- Pitfalls: HIGH - Based on documented C++ concurrency issues and reference implementation patterns

**Research date:** 2026-02-01
**Valid until:** 30 days (C++ ecosystem is stable, rust-sdk patterns are established)
