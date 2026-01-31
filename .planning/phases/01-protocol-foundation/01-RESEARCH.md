# Phase 1: Protocol Foundation - Research

**Researched:** 2026-01-31
**Domain:** JSON-RPC 2.0 + MCP Protocol + C++17 async patterns
**Confidence:** HIGH

## Summary

This phase implements the foundational protocol layer: JSON-RPC 2.0 message handling, MCP initialization handshake, transport abstraction interface, and callback-based async API. The research focused on three key areas: (1) MCP protocol specification and current implementations, (2) C++ JSON-RPC patterns and libraries, and (3) C++ async callback lifetime management.

**Primary recommendation:** Use nlohmann/json 3.11+ for JSON handling, follow the MCP 2025-11-25 schema exactly, implement library-managed atomic request IDs, and use std::function with documented lifetime patterns for callbacks. The Rust SDK (rmcp) provides excellent reference patterns for request tracking and timeout management.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| **nlohmann/json** | 3.11+ | JSON serialization/deserialization | De facto standard for modern C++; header-only; excellent C++17 support |
| **MCP Spec** | 2025-11-25 | Protocol definition | Official schema from modelcontextprotocol repo |
| **C++17** | gcc-11/g++-11 | Language standard | Required features: std::optional, std::variant, structured bindings |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| **Google Test** | 1.11+ | Unit testing | For protocol layer unit tests |
| **CMake** | 3.10+ | Build system | Required; FetchContent for dependencies |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| nlohmann/json | jsoncpp | More verbose API; less modern; nlohmann has better C++17 integration |
| nlohmann/json | RapidJSON | Faster but less safe; header-only but more complex API |
| nlohmann/json | simdjson | Faster parsing but not focused on serialization/writing |

**Installation:**
```cmake
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── mcpp/
│   ├── core/              # JSON-RPC 2.0 core
│   │   ├── json_rpc.h     # Request/Response/Notification types
│   │   ├── json_rpc.cpp
│   │   ├── request_id.h   # Request ID generation (atomic)
│   │   └── error.h        # JSON-RPC error codes
│   ├── protocol/          # MCP-specific protocol
│   │   ├── types.h        # MCP message types (from schema)
│   │   ├── initialize.h   # initialize/initialized handshake
│   │   └── capabilities.h # Capability declarations
│   ├── transport/         # Transport abstraction
│   │   └── transport.h    # Transport interface
│   └── async/             # Async callback infrastructure
│       ├── callbacks.h    # std::function callback types
│       └── timeout.h      # Timeout manager
```

### Pattern 1: JSON-RPC Message Types
**What:** Use nlohmann::json with strongly-typed wrappers for JSON-RPC messages
**When to use:** All JSON-RPC communication
**Example:**
```cpp
// Source: MCP 2025-11-25 schema + nlohmann/json documentation
namespace mcpp::core {

using RequestId = std::variant<int64_t, std::string>;
using JsonValue = nlohmann::json;

struct JsonRpcRequest {
    std::string jsonrpc = "2.0";
    RequestId id;
    std::string method;
    JsonValue params;  // nullptr if no params

    JsonValue to_json() const {
        return {
            {"jsonrpc", jsonrpc},
            {"id", std::visit([](auto&& v) -> JsonValue { return v; }, id)},
            {"method", method},
            {"params", params}
        };
    }
};

struct JsonRpcResponse {
    std::string jsonrpc = "2.0";
    RequestId id;
    std::optional<JsonValue> result;
    std::optional<JsonRpcError> error;

    static std::optional<JsonRpcResponse> from_json(const JsonValue& j);
};

struct JsonRpcNotification {
    std::string jsonrpc = "2.0";
    std::string method;
    JsonValue params;
};

} // namespace mcpp::core
```

### Pattern 2: MCP Initialization Handshake
**What:** Client sends initialize request, server responds with capabilities, client sends initialized notification
**When to use:** Connection startup
**Example:**
```cpp
// Source: MCP 2025-11-25 schema specification
namespace mcpp::protocol {

struct InitializeRequestParams {
    std::string protocolVersion;
    ClientCapabilities capabilities;
    Implementation clientInfo;
    std::optional<RequestMeta> _meta;
};

struct InitializeResult {
    std::string protocolVersion;
    ServerCapabilities capabilities;
    Implementation serverInfo;
    std::optional<std::string> instructions;
};

// Handshake flow (client-side):
// 1. Send initialize request with client capabilities
// 2. Receive initialize response with server capabilities
// 3. Verify protocol version compatibility
// 4. Send initialized notification
// 5. Connection is now ready for normal operation

} // namespace mcpp::protocol
```

### Pattern 3: Transport Abstraction Interface
**What:** Abstract interface for sending/receiving complete JSON-RPC messages
**When to use:** Pluggable transport implementations (stdio, SSE, WebSocket)
**Example:**
```cpp
// Source: Based on rmcp transport design + CONTEXT.md decisions
namespace mcpp::transport {

class Transport {
public:
    virtual ~Transport() = default;

    // Explicit lifecycle (not RAII)
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;

    // Send a complete JSON-RPC message (already framed)
    virtual bool send(std::string_view message) = 0;

    // Set callback for receiving messages
    using MessageCallback = std::function<void(std::string_view)>;
    virtual void set_message_callback(MessageCallback cb) = 0;

    // Error callback for async errors
    using ErrorCallback = std::function<void(std::string_view)>;
    virtual void set_error_callback(ErrorCallback cb) = 0;
};

} // namespace mcpp::transport
```

### Pattern 4: Library-Managed Request Tracking
**What:** Library generates request IDs via atomic counter, tracks pending requests
**When to use:** All outgoing requests that expect responses
**Example:**
```cpp
// Source: rmcp AtomicU32RequestIdProvider pattern
namespace mcpp::core {

class RequestTracker {
public:
    RequestId next_id() {
        return RequestId{counter_.fetch_add(1, std::memory_order_seq_cst)};
    }

    void register_pending(RequestId id, ResponseCallback callback) {
        std::lock_guard lock(mutex_);
        pending_[id] = std::move(callback);
    }

    std::optional<ResponseCallback> complete(RequestId id) {
        std::lock_guard lock(mutex_);
        auto it = pending_.find(id);
        if (it == pending_.end()) return std::nullopt;
        auto cb = std::move(it->second);
        pending_.erase(it);
        return cb;
    }

private:
    std::atomic<uint64_t> counter_{0};
    std::unordered_map<RequestId, ResponseCallback> pending_;
    std::mutex mutex_;
};

} // namespace mcpp::core
```

### Anti-Patterns to Avoid
- **RAII-based connection lifecycle:** Users need explicit control over when connect() happens. Use explicit methods instead.
- **User-managed request IDs:** Leads to collisions and tracking complexity. Library should manage IDs.
- **Cross-thread callback invocation:** Makes user code complex. Library guarantees same-thread invocation.
- **Blocking I/O in callback handlers:** Can deadlock the event loop. Document non-blocking requirements.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| JSON serialization | Custom JSON writer | nlohmann/json | Edge cases in escaping, unicode, numbers; proven library |
| JSON parsing | Custom parser | nlohmann/json | Error handling, type coercion, security |
| Atomic ID generation | mutex + counter | std::atomic | Lock-free, faster, simpler |
| Async callback storage | raw pointers | std::function | Exception safety, type erasure, move semantics |
| String building | string concatenation | std::stringstream or fmt | Performance, safety |

**Key insight:** JSON handling in particular is notorious for edge cases (unicode escapes, control characters, nested structures). Even "simple" JSON code has bugs that nlohmann/json has already solved.

## Common Pitfalls

### Pitfall 1: Dangling Reference Captures in Callbacks
**What goes wrong:** Callback captures `this` pointer or references that become invalid before callback fires.
**Why it happens:** std::function doesn't extend lifetime of captured objects.
**How to avoid:** Document value capture recommendations. Consider accepting `std::shared_ptr` for callback owners.
**Warning signs:** Use-after-free crashes, seemingly random callback failures.

### Pitfall 2: Request ID Type Mismatch
**What goes wrong:** Using only int or only string for request IDs, but peer uses the other.
**Why it happens:** JSON-RPC 2.0 spec allows both, but many implementations pick one.
**How to avoid:** Use std::variant<int64_t, std::string> for RequestId type.
**Warning signs:** "Request not found" errors when responses arrive.

### Pitfall 3: Protocol Version Negotiation Failure
**What goes wrong:** Server and client have incompatible protocol versions, but code doesn't check.
**Why it happens:** initialize response includes protocolVersion that must be verified.
**How to avoid:** Always check server's protocolVersion against supported range in initialize response handler.
**Warning signs:** "Method not found" errors for standard MCP methods.

### Pitfall 4: Timeout Implementation Race Conditions
**What goes wrong:** Request completes just as timeout fires, causing double-cleanup.
**Why it happens:** Timeout thread and response completion race to remove from pending map.
**How to avoid:** Use atomic try-remove pattern or lock both timeout and completion paths.
**Warning signs:** Occasional crashes in callback invocation.

### Pitfall 5: JSON-RPC Error Response Without ID
**What goes wrong:** Error response has null id field, but code assumes id is always present.
**Why it happens:** JSON-RPC spec allows error responses to have null id when request couldn't be parsed.
**How to avoid:** Handle optional<RequestId> in error responses.
**Warning signs:** Crashes when sending malformed requests.

## Code Examples

Verified patterns from official sources:

### JSON-RPC Error Handling
```cpp
// Source: MCP 2025-11-25 schema (official spec)
namespace mcpp::core {

// Standard JSON-RPC error codes (from spec)
constexpr int PARSE_ERROR = -32700;
constexpr int INVALID_REQUEST = -32600;
constexpr int METHOD_NOT_FOUND = -32601;
constexpr int INVALID_PARAMS = -32602;
constexpr int INTERNAL_ERROR = -32603;

struct JsonRpcError {
    int code;
    std::string message;
    std::optional<JsonValue> data;

    JsonValue to_json() const {
        JsonValue j = {{"code", code}, {"message", message}};
        if (data) j["data"] = *data;
        return j;
    }
};

// Tool execution error with isError flag (MCP-specific)
struct ToolExecutionError {
    bool isError = true;
    std::string content;  // Error description
};

} // namespace mcpp::core
```

### Capability Declaration Pattern
```cpp
// Source: MCP 2025-11-25 schema
namespace mcpp::protocol {

struct ServerCapabilities {
    std::optional<CapabilitySet> experimental;
    std::optional<LoggingCapability> logging;
    std::optional<PromptCapability> prompts;
    std::optional<ResourceCapability> resources;
    std::optional<ToolCapability> tools;
    // ... other capabilities
};

struct ToolCapability {
    std::optional<bool> listChanged;  // Optional: supports list_changed notification
};

// Nested sub-capabilities
struct ResourceCapability {
    bool subscribe = false;           // Can subscribe to resource updates
    std::optional<bool> listChanged;  // Supports list_changed notification
};

} // namespace mcpp::protocol
```

### Callback Registration Pattern
```cpp
// Source: Based on rmcp callback patterns + CONTEXT.md async model
namespace mcpp::async {

using ResponseCallback = std::function<void(const JsonValue& result)>;
using ErrorCallback = std::function<void(const JsonRpcError& error)>;

// Library stores std::function copies
class McpClient {
public:
    // User moves std::function in, library makes copy
    void call_tool(
        std::string_view tool_name,
        const JsonValue& arguments,
        ResponseCallback on_success,
        ErrorCallback on_error
    ) {
        // Store callbacks in pending requests
        RequestId id = request_tracker_.next_id();
        request_tracker_.register_pending(id, {
            .on_success = std::move(on_success),
            .on_error = std::move(on_error)
        });
        // Send request...
    }

private:
    RequestTracker request_tracker_;
};

// Usage example (user code):
// client.call_tool("calculator", {{"a", 1}, {"b", 2}},
//     [](const JsonValue& result) {
//         // Success handler - capture by value for safety
//         std::cout << "Result: " << result << std::endl;
//     },
//     [](const JsonRpcError& error) {
//         // Error handler
//         std::cerr << "Error: " << error.message << std::endl;
//     }
// );

} // namespace mcpp::async
```

### Timeout Manager Pattern
```cpp
// Source: rmcp timeout pattern + CONTEXT.md requirements
namespace mcpp::async {

class TimeoutManager {
public:
    TimeoutManager(std::chrono::milliseconds default_timeout)
        : default_timeout_(default_timeout) {}

    void set_timeout(RequestId id,
                     std::chrono::milliseconds timeout,
                     std::function<void()> on_timeout) {
        std::lock_guard lock(mutex_);
        deadlines_[id] = {
            .deadline = Clock::now() + timeout,
            .callback = std::move(on_timeout)
        };
    }

    // Called from background thread
    void check_timeouts() {
        auto now = Clock::now();
        std::vector<RequestId> expired;

        {
            std::lock_guard lock(mutex_);
            for (auto& [id, entry] : deadlines_) {
                if (now >= entry.deadline) {
                    expired.push_back(id);
                }
            }
        }

        for (RequestId id : expired) {
            std::function<void()> cb;
            {
                std::lock_guard lock(mutex_);
                auto it = deadlines_.find(id);
                if (it != deadlines_.end()) {
                    cb = std::move(it->second.callback);
                    deadlines_.erase(it);
                }
            }
            if (cb) cb();
        }
    }

    void cancel(RequestId id) {
        std::lock_guard lock(mutex_);
        deadlines_.erase(id);
    }

private:
    using Clock = std::chrono::steady_clock;

    struct TimeoutEntry {
        Clock::time_point deadline;
        std::function<void()> callback;
    };

    std::chrono::milliseconds default_timeout_;
    std::unordered_map<RequestId, TimeoutEntry> deadlines_;
    std::mutex mutex_;
};

} // namespace mcpp::async
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Custom JSON parsing | nlohmann/json | ~2016+ | Header-only, type-safe, modern C++ |
| Manual request IDs | Library-managed atomic IDs | MCP spec 2024+ | Simpler user code, no collisions |
| RAII connections | Explicit lifecycle | MCP tools practice | Better control over connection timing |
| C++11 std::bind | C++17 lambdas + std::function | C++17 | Cleaner syntax, better capture |

**Deprecated/outdated:**
- **C++14-style JSON libraries**: jsoncpp, rapidjson (less safe, more verbose)
- **Raw function pointers**: Use std::function for type erasure and exception safety
- **Manual mutex-only request tracking**: Use atomic for ID generation

## Open Questions

Things that couldn't be fully resolved:

1. **Timeout Manager Implementation Strategy**
   - What we know: Need background checking of expired requests
   - What's unclear: Whether to use dedicated thread vs integrate with event loop
   - Recommendation: Start with simple background thread using std::thread + sleep loop; can optimize to event loop integration later if needed

2. **HashMap Thread Safety**
   - What we know: Pending requests need concurrent access (timeout thread + I/O thread)
   - What's unclear: Whether std::unordered_map + mutex is sufficient or need concurrent map
   - Recommendation: Start with mutex-protected unordered_map; profile before optimizing

3. **Error Type Hierarchy**
   - What we know: Need JSON-RPC errors, MCP errors, transport errors
   - What's unclear: Exact inheritance vs composition structure
   - Recommendation: Use std::variant for error categories rather than deep inheritance

## Sources

### Primary (HIGH confidence)
- [MCP Schema 2025-11-25](https://github.com/modelcontextprotocol/specification/blob/main/schema/2025-11-25/schema.ts) - Complete protocol definition, all message types, error codes, capability structure
- [nlohmann/json GitHub](https://github.com/nlohmann/json) - JSON library documentation, API reference, examples
- [nlohmann/json CMake Integration](https://json.nlohmann.me/integration/cmake/) - Official CMake/FetchContent integration guide
- [rmcp (Rust SDK) - service.rs](https://github.com/modelcontextprotocol/rust-sdk/blob/main/crates/rmcp/src/service.rs) - Reference implementation for request tracking, timeout handling, ID generation
- [gopher-mcp README](https://github.com/MCP/backends) - C++ reference for architecture patterns (note: uses older MCP version)

### Secondary (MEDIUM confidence)
- [Building a Modern C++23 JSON-RPC 2.0 Library](https://medium.com/@pooriayousefi/building-a-modern-c-23-json-rpc-2-0-library-b62c4826769d) - Architecture patterns for JSON-RPC in modern C++
- [jsonrpc++](https://github.com/badaix/jsonrpcpp) - Transport-agnostic JSON-RPC design patterns
- [C++ member function callback and object lifetime](https://stackoverflow.com/questions/59504381/c-member-function-callback-and-object-lifetime) - Callback lifetime discussion
- [How to properly handle the life span of a callback](https://www.reddit.com/r/cpp_questions/comments/ajckh6/how_to_properly_handle_the_life_span_of_a_callback/) - Community best practices

### Tertiary (LOW confidence)
- [libjson-rpc-cpp](https://github.com/cinemast/libjson-rpc-cpp) - Older C++ JSON-RPC framework; patterns useful but code outdated
- [jsonrpc-cpp-lib](https://github.com/hankhsu1996/jsonrpc-cpp-lib) - Modern JSON-RPC but less mature than nlohmann-based approaches

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - nlohmann/json is de facto standard; MCP 2025-11-25 is official spec
- Architecture: HIGH - Based on official MCP schema and proven rmcp patterns
- Pitfalls: MEDIUM - Callback lifetime issues are well-documented; timeout race conditions based on general concurrent programming knowledge

**Research date:** 2026-01-31
**Valid until:** 2026-03-02 (30 days - MCP spec is stable but check for updates)
