# Architecture Research

**Domain:** C++ Networking / Protocol Library (MCP - Model Context Protocol)
**Researched:** 2025-01-31
**Confidence:** HIGH

## Standard Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        High-Level API Layer                                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │ McpClient    │  │ McpServer    │  │ Tool/Resource │  │ Builder API  │   │
│  │ (user API)   │  │ (user API)   │  │ Wrappers     │  │ (convenience)│   │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘   │
│         │                 │                 │                 │             │
├─────────┼─────────────────┼─────────────────┼─────────────────┼─────────────┤
│         │         ┌───────▼────────┐       │                 │             │
│         │         │ JSON-RPC Layer │       │                 │             │
│         │         │ - Request/Resp  │       │                 │             │
│         │         │ - Notification │       │                 │             │
│         │         │ - Validation   │       │                 │             │
│         │         └───────┬────────┘       │                 │             │
│         │                 │                 │                 │             │
├─────────┼─────────────────┼─────────────────┼─────────────────┼─────────────┤
│         │         ┌───────▼──────────────────────────────────▼─────────┐   │
│         │         │              Filter Chain                          │   │
│         │         │  ┌────────────┐ ┌────────────┐ ┌────────────┐     │   │
│         │         │  │ JSON-RPC   │ │ HTTP/SSE   │ │ Framing    │     │   │
│         │         │  │ Filter     │ │ Codec      │ │ Filter     │     │   │
│         │         │  └────────────┘ └────────────┘ └────────────┘     │   │
│         │         └──────────────────────┬──────────────────────────────┘   │
├─────────┼────────────────────────────────┼─────────────────────────────────┤
│         │         ┌───────────────────────▼──────────────────────────────┐ │
│         │         │            Transport Abstraction Layer               │ │
│         │         │  ┌────────────┐ ┌────────────┐ ┌────────────┐      │ │
│         │         │  │ Stdio      │ │ SSE (HTTP) │ │ WebSocket  │      │ │
│         │         │  │ Transport  │ │ Transport  │ │ Transport  │      │ │
│         │         │  └────────────┘ └────────────┘ └────────────┘      │ │
│         │         └───────────────────────┬──────────────────────────────┘ │
├─────────┼────────────────────────────────┼─────────────────────────────────┤
│         │         ┌───────────────────────▼──────────────────────────────┐ │
│         │         │           Event Loop / Dispatcher                    │ │
│         │         │  - File events (read/write)                         │ │
│         │         │  - Timers                                          │ │
│         │         │  - Deferred deletion                               │ │
│         │         │  - Thread-safe posting                             │ │
│         │         └───────────────────────┬──────────────────────────────┘ │
├─────────┼────────────────────────────────┼─────────────────────────────────┤
│         │         ┌───────────────────────▼──────────────────────────────┐ │
│         │         │           Platform I/O Layer                         │ │
│         │         │  Linux: epoll │ macOS: kqueue │ Windows: IOCP       │ │
│         │         └──────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation | Communicates With |
|-----------|----------------|------------------------|-------------------|
| **McpClient/McpServer** | User-facing API, connection lifecycle, capability negotiation | Facade pattern, holds references to all subsystems | JSON-RPC layer, Transport, User callbacks |
| **JSON-RPC Layer** | Request/response validation, ID generation, message serialization | State machine for protocol lifecycle, nlohmann/json | Filter chain, Client/Server API |
| **Filter Chain** | Protocol processing pipeline - decode/encode/transform | ReadFilter/WriteFilter interfaces with FilterManager | Transport layer, JSON-RPC layer |
| **Transport Abstraction** | Byte-level I/O, transport-specific framing (SSE/stdio/WS) | TransportSocket interface with polymorphic implementations | Event loop, Filter chain |
| **Event Loop/Dispatcher** | Async I/O multiplexing, timer scheduling, callback execution | libevent/epoll/kqueue wrapper with FileEvent/Timer abstractions | All layers (via callbacks) |
| **Platform I/O** | OS-specific system calls for network/file I/O | Syscall wrappers (epoll_wait/kevent/WSAPoll) | Event loop only |

## Recommended Project Structure

```
mcpp/
├── include/mcpp/
│   ├── mcpp.h                 # Main public header
│   ├── client/
│   │   ├── client.h           # McpClient facade
│   │   └── types.h            # Client-specific types
│   ├── server/
│   │   ├── server.h           # McpServer facade
│   │   └── types.h            # Server-specific types
│   ├── core/
│   │   ├── types.h            # Common JSON-RPC types
│   │   ├── result.h           # Result/Error types
│   │   └── async.h            # Future/callback utilities
│   ├── protocol/
│   │   ├── json_rpc.h         # JSON-RPC message types
│   │   ├── protocol_handler.h # Protocol state machine
│   │   └── validation.h       # Message validation
│   ├── transport/
│   │   ├── transport.h        # Transport interface
│   │   ├── stdio_transport.h  # stdio implementation
│   │   ├── sse_transport.h    # SSE implementation
│   │   └── ws_transport.h     # WebSocket implementation
│   ├── filter/
│   │   ├── filter.h           # Filter interfaces
│   │   ├── filter_chain.h     # Filter chain manager
│   │   ├── json_rpc_filter.h  # JSON-RPC codec filter
│   │   └── framing_filter.h   # Message framing filter
│   └── event/
│       ├── dispatcher.h       # Event loop interface
│       ├── timer.h            # Timer abstraction
│       ├── file_event.h       # File descriptor events
│       └── platform_factory.h # Platform-specific factory
├── src/
│   ├── client/
│   ├── server/
│   ├── core/
│   ├── protocol/
│   ├── transport/
│   ├── filter/
│   └── event/
└── tests/
    ├── unit/
    └── integration/
```

### Structure Rationale

- **client/**: Client-specific functionality (connection initiation, request tracking)
- **server/**: Server-specific functionality (listener management, session handling)
- **core/**: Shared types, error handling, async utilities used by both client and server
- **protocol/**: MCP protocol logic independent of transport (JSON-RPC layer)
- **transport/**: Transport implementations are swappable and isolated
- **filter/**: Filter chain provides extensibility for protocol processing
- **event/**: Event loop is the foundation - isolated for potential alternative implementations

## Architectural Patterns

### Pattern 1: Filter Chain (Pipes and Filters)

**What:** A chain of filters process data in sequence, each responsible for a specific transformation. Inspired by Envoy's network filter architecture.

**When to use:** Processing layered protocols where multiple transformations occur (framing → decoding → routing).

**Trade-offs:**
- **Pros:** Extensible, testable, clear separation of concerns, easy to add new protocol features
- **Cons:** Indirection can hurt performance, state management across filters is complex

**Example:**
```cpp
// Filter interface - each filter processes data in both directions
class ReadFilter {
public:
    virtual FilterStatus onData(Buffer& data, bool end_stream) = 0;
    virtual void initializeReadFilterCallbacks(ReadFilterCallbacks& callbacks) = 0;
};

class WriteFilter {
public:
    virtual FilterStatus onWrite(Buffer& data, bool end_stream) = 0;
    virtual void initializeWriteFilterCallbacks(WriteFilterCallbacks& callbacks) = 0;
};

// Filter chain processes data through multiple stages
// Read path: Raw bytes → HTTP decode → SSE decode → JSON-RPC parse → Application
// Write path: Application → JSON-RPC serialize → SSE encode → HTTP encode → Raw bytes
```

### Pattern 2: Single-Threaded Event Loop with Thread-Safe Posting

**What:** Each worker thread runs its own event loop. Cross-thread communication happens via thread-safe message posting. Avoids locks on hot paths.

**When to use:** High-performance networking where lock contention would be a bottleneck.

**Trade-offs:**
- **Pros:** Lock-free data path, excellent scalability, predictable latency
- **Cons:** Cannot directly share state between threads, all access must be posted

**Example:**
```cpp
class Dispatcher {
public:
    // Thread-safe: can be called from any thread
    void post(std::function<void()> callback) {
        {
            std::lock_guard<std::mutex> lock(post_mutex_);
            post_queue_.push(std::move(callback));
        }
        // Wake up the event loop
        wakeup();
    }

    // Only call from dispatcher thread
    void run() {
        while (running_) {
            process_events();
            process_posted_callbacks();
        }
    }

private:
    std::mutex post_mutex_;
    std::queue<std::function<void()>> post_queue_;
};
```

### Pattern 3: Layered API Design

**What:** Provide both low-level (callback-based) and high-level (future-based) APIs. Low-level for streaming/complex cases; high-level for simple RPC calls.

**When to use:** Library needs to support both simple use cases and complex scenarios like streaming responses.

**Trade-offs:**
- **Pros:** Ergonomic for common cases, flexible for advanced users
- **Cons:** More API surface to maintain, potential confusion about which to use

**Example:**
```cpp
// High-level: Future-based API for simple RPC
std::future<CallToolResult> callTool(const std::string& name, const json& args);

// Low-level: Callback-based API for streaming
void callToolStreaming(
    const std::string& name,
    const json& args,
    std::function<void(const json&)> on_chunk,
    std::function<void(const Error&)> on_error
);
```

### Pattern 4: Facade with Internal Component Coordination

**What:** McpClient/McpServer act as facades coordinating internal components (connection manager, protocol handler, request tracker). Simplifies user API while keeping internal flexibility.

**When to use:** Complex subsystems that need to present a simple interface.

**Trade-offs:**
- **Pros:** Clean user API, internal implementation can evolve independently
- **Cons:** Facade class becomes large, risk of God object

**Example:**
```cpp
class McpClient {
public:
    // Simple user API
    std::future<ListToolsResult> listTools();

private:
    // Internal components
    std::unique_ptr<ConnectionManager> connection_manager_;
    std::unique_ptr<ProtocolHandler> protocol_handler_;
    std::unique_ptr<RequestTracker> request_tracker_;
    std::unique_ptr<CircuitBreaker> circuit_breaker_;
};
```

## Data Flow

### Request Flow (Client)

```
[User Code]
    │ callTool("calculator", {"expression": "2+2"})
    ▼
[McpClient]
    │ Generate request ID
    │ Create RequestContext with promise
    │ Route to protocol layer
    ▼
[JSON-RPC Layer]
    │ Validate request
    │ Serialize to JSON
    │ Add to pending requests map
    ▼
[Filter Chain: Write]
    │ JsonRpcFilter: Add JSON-RPC envelope
    │ FramingFilter: Add length prefix
    │ SSE/HTTP Filter: Add HTTP/SSE framing
    ▼
[Transport]
    │ Write to socket/stdio
    ▼
[Network] ──────────────────────────────────────────────→ [Server]
                                                                          │
[Client] ←─────────────────────────────────────────────────┤
    ▼                                                                   │
[Transport]                                                          [Server]
    │ Read from socket/stdio                                          │
    ▼                                                                   │
[Filter Chain: Read]                                            [Server]
    │ SSE/HTTP Filter: Remove HTTP/SSE framing                      │
    │ FramingFilter: Extract message by length                      │
    │ JsonRpcFilter: Parse JSON, validate JSON-RPC                  │
    ▼                                                                   │
[JSON-RPC Layer]                                                 [Server]
    │ Match response ID to pending request                        │
    │ Set promise value                                             │
    ▼                                                                   │
[McpClient]                                                      [Server]
    │ Future resolves                                                │
    ▼                                                                   │
[User Code] ─── return result ────────────────────────────────────┘
```

### Notification Flow (Server to Client)

```
[User Code]
    │ sendNotification("progress", {"token": "abc", "value": 50})
    ▼
[McpServer]
    │ Serialize notification
    ▼
[JSON-RPC Layer]
    │ Create JSON-RPC notification (no ID)
    ▼
[Filter Chain: Write]
    │ Same as request path
    ▼
[Transport]
    │ Write to socket
    ▼
[Network] ──────────────────────────────────────────────→ [Client]
    │
[Client] receives notification
    │ No request matching (it's a notification)
    ▼
[Protocol Callbacks]
    │ onNotification() called
    ▼
[User Callback]
    │ User-provided notification handler
```

### Thread-Safe Data Flow

```
[Thread A: User]          [Thread B: Event Loop]         [Thread C: Application]
│ callTool()              │                               │
│   creates promise       │                               │
│   posts to dispatcher ───┼──> post_queue_               │
│   returns future        │                               │
│                         │ process_posted_callbacks()    │
│                         │   → generateRequest()         │
│                         │   → serialize                 │
│                         │   → write to transport        │
│                         │                               │
│                         │ ← on_read_ready()             │
│                         │   → filter chain             │
│                         │   → parse response            │
│                         │   → find pending request      │
│                         │   → set promise value         │
│                         │                               │
│ future.get() ◄───────────────────────────────────────────┘
│   (blocks until promise set)
```

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|--------------------------|
| 0-1 connections | Single dispatcher thread is sufficient. No connection pooling needed. |
| 1-100 connections | Single dispatcher still fine. Consider connection pool for HTTP transports. |
| 100-10,000 connections | Multiple worker threads with connection distribution. Enable flow control/backpressure. |
| 10,000+ connections | Multi-dispatcher with lock-free data structures between threads. Consider SO_REUSEPORT for socket sharding. |

### Scaling Priorities

1. **First bottleneck:** Event loop CPU saturation at high connection count. **Fix:** Add worker threads with epoll-per-thread or SO_REUSEPORT.

2. **Second bottleneck:** Memory allocation from frequent buffer creation. **Fix:** Buffer pooling, arena allocators for per-connection data.

3. **Third bottleneck:** JSON parsing overhead. **Fix:** Consider SIMD JSON parsers (simdjson) for hot paths.

## Anti-Patterns

### Anti-Pattern 1: Locking on Data Path

**What people do:** Protect every shared data structure with mutexes, including the request map and connection state.

**Why it's wrong:** Locks on the hot path cause contention. At high QPS, threads spend more time contending than doing useful work.

**Do this instead:** Use thread-local dispatcher pattern. Each thread owns its connections. Cross-thread communication via lock-free queues (post()). The only shared data is immutable config.

### Anti-Pattern 2: Mixing Sync and Async APIs

**What people do:** Provide both blocking callTool() and async callToolAsync() where the blocking version just waits on the async.

**Why it's wrong:** Users reach for the blocking API "for simplicity," then accidentally block their event loop thread. Deadlocks and poor performance follow.

**Do this instead:** Provide only async API. If users want blocking, they can wrap the future:
```cpp
auto result = client.callTool(...).get(); // User's choice, not library's
```

### Anti-Pattern 3: Callback Hell Without RAII

**What people do:** Raw callback pointers and manual lifetime management. Callbacks outlive their objects, use-after-free.

**Why it's wrong:** C++ without RAII is C. Manual lifetime management in async context is extremely error-prone.

**Do this instead:** Use std::shared_ptr for callback targets, or capture by value in lambdas. Deferred deletion pattern for objects that must outlive callbacks.

### Anti-Pattern 4: Tight Coupling to Transport

**What people do:** JSON-RPC layer knows about HTTP headers. SSE filter assumes TCP socket.

**Why it's wrong:** Cannot add new transports (WebSocket, Unix socket) without modifying core logic.

**Do this instead:** Transport is a byte stream abstraction. Filter chain handles all protocol-specific framing. JSON-RPC layer only sees complete messages.

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| libevent | Optional backend for Dispatcher | Can use epoll/kqueue directly if preferred |
| nlohmann/json | JSON encoding/decoding | Header-only, widely used, reliable |
| OpenSSL | TLS support for WebSocket | Optional, only needed for wss:// |
| spdlog | Logging | Optional, can use custom logger |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| User API ↔ Core | Direct calls (same thread) | User API posts to dispatcher if needed |
| Core ↔ Filter Chain | Direct method calls | Always on dispatcher thread |
| Filter Chain ↔ Transport | Direct method calls | Always on dispatcher thread |
| Transport ↔ Event Loop | Callback registration | File events registered with dispatcher |
| Cross-thread | Dispatcher::post() | Thread-safe, any thread can post |

## Thread Safety Strategy

### Core Principles

1. **Dispatcher thread affinity:** Each connection belongs to one dispatcher thread. All I/O for that connection happens on that thread.

2. **Thread-safe posting:** The `post()` method is the only cross-thread communication mechanism. It's lock-free or uses a single mutex.

3. **Immutable shared config:** Configuration is read-only after initialization. No locks needed to read config.

4. **Atomic flags only:** Use `std::atomic<bool>` for shutdown flags and similar simple state.

### Thread Safety by Layer

| Layer | Thread Safety | Notes |
|-------|---------------|-------|
| User API | Thread-safe | Posts to dispatcher, returns future |
| JSON-RPC Layer | Dispatcher-thread only | Never called directly from user threads |
| Filter Chain | Dispatcher-thread only | Filters execute on dispatcher thread |
| Transport | Dispatcher-thread only | I/O initiated from dispatcher thread |
| Event Loop | Thread-safe internally | post() from any thread, events dispatched on own thread |

### Request Tracking Thread Safety

```cpp
// Called from user thread (thread-safe)
std::future<Response> McpClient::sendRequest(const Request& request) {
    auto promise = std::make_shared<std::promise<Response>>();
    auto future = promise->get_future();

    // Post to dispatcher thread
    dispatcher_->post([this, request, promise]() {
        // Now on dispatcher thread - safe to access pending_requests_
        auto id = generateRequestId();
        pending_requests_[id] = RequestContext{request, promise};
        sendToTransport(request, id);
    });

    return future;
}

// Called from dispatcher thread (not thread-safe, must be on dispatcher)
void McpClient::handleResponse(const Response& response) {
    assert(dispatcher_->isThreadSafe());
    auto it = pending_requests_.find(response.id);
    if (it != pending_requests_.end()) {
        it->second.promise.set_value(response);
        pending_requests_.erase(it);
    }
}
```

## Build Order (Dependencies)

```
Phase 1: Foundation (no dependencies between these)
├── event/          # Event loop - must exist first
├── core/           # Types, Result, Async utilities
└── transport/      # Transport interface (no implementations yet)

Phase 2: Protocol Layer (depends on Phase 1)
├── protocol/       # JSON-RPC types, validation
└── filter/         # Filter interfaces and chain

Phase 3: Transport Implementations (depends on Phase 1+2)
├── transport/stdio_transport.cpp
├── transport/sse_transport.cpp
└── transport/ws_transport.cpp

Phase 4: Filter Implementations (depends on Phase 2+3)
├── filter/json_rpc_filter.cpp
├── filter/framing_filter.cpp
└── filter/sse_filter.cpp

Phase 5: High-Level API (depends on Phase 1-4)
├── client/client.cpp
└── server/server.cpp
```

### Why This Order?

1. **Event loop first:** Everything else needs async scheduling.
2. **Core types early:** Shared types used everywhere.
3. **Transport interface before implementation:** Define contract before implementing.
4. **Filter chain before filters:** Define the pattern before concrete filters.
5. **Client/Server last:** They coordinate all other components.

## Sources

- **Envoy Proxy Architecture:**
  - [Envoy Threading Model (Official Blog)](https://blog.envoyproxy.io/envoy-threading-model-a8d44b922310)
  - [Life of a Request (Official Documentation)](https://www.envoyproxy.io/docs/envoy/latest/intro/life_of_a_request)
  - [Envoy Event and Network Filter Framework Analysis](https://blog.mygraphql.com/zh/posts/cloud/envoy/event-driven-network-filter/)

- **gopher-mcp Reference Implementation (HIGH confidence):**
  - `/thirdparty/gopher-mcp/include/mcp/` - Complete C++ MCP implementation
  - Filter chain architecture directly inspired by Envoy
  - Event loop abstraction for epoll/kqueue/IOCP

- **rmcp Rust SDK (HIGH confidence):**
  - `/thirdparty/rust-sdk/crates/rmcp/` - Modern MCP implementation
  - Service trait abstraction and transport interface design
  - Async patterns with cancellation support

- **Async C++ Patterns (MEDIUM confidence):**
  - [Thread safe asynchronous code - Fuchsia](https://fuchsia.dev/fuchsia-src/development/languages/c-cpp/thread-safe-async) (March 2025)
  - [Asynchronous Callback API Tutorial | C++](https://grpc.io/docs/languages/cpp/callback/) (June 2024)
  - [Pipes-and-Filters Architecture Pattern](https://www.modernescpp.com/index.php/pipes-and-filters/) (April 2023)

- **JSON-RPC C++ Implementation (MEDIUM confidence):**
  - [Building a Modern C++23 JSON-RPC 2.0 Library](https://medium.com/@pooriayousefi/building-a-modern-c-23-json-rpc-2-0-library-b62c4826769d)
  - [RPC framework with Muduo](https://cloud.tencent.com/developer/article/2517637)

---
*Architecture research for: C++ MCP Library (mcpp)*
*Researched: 2025-01-31*
