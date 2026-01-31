# Phase 3: Client Capabilities - Research

**Researched:** 2026-01-31
**Domain:** MCP Client Capabilities (Roots, Sampling, Elicitation) + C++ Async/Cancellation
**Confidence:** HIGH

## Summary

This phase implements the MCP client-side capabilities: roots management (roots/list, roots/list_changed notification), sampling (sampling/createMessage for LLM completions), elicitation (interactive user input via forms and URLs), and comprehensive cancellation support. The research focused on four key areas: (1) MCP 2025-11-25 client capability specifications, (2) C++ cancellation patterns (std::stop_token vs atomic<bool>), (3) std::future-based wrapper APIs for ergonomic blocking, and (4) reference implementations from gopher-mcp and rmcp (Rust SDK).

The research confirmed that the MCP 2025-11-25 schema defines client capabilities as optional features advertised during initialization. Key findings include: roots must be file:// URIs only, sampling supports both basic messages and tool use with agentic loops, elicitation has two modes (form with JSON Schema validation, and URL for out-of-band interactions), cancellation uses notifications/cancelled with race condition handling, and std::future/promise provides an ergonomic blocking API wrapper around callback-based async operations.

**Primary recommendation:** Use std::stop_token (C++20) for cancellation instead of atomic<bool> (better composability, works with std::jthread), implement roots as file:// URI only with list_changed notification, support sampling with createMessage request including tool use (tools, toolChoice parameters), implement elicitation with both form mode (JSON Schema validation) and URL mode (out-of-band), and wrap callback-based async API with std::future using shared_ptr<promise> pattern from gopher-mcp.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| **C++20 std::stop_token** | C++20 | Cancellation token for cooperative cancellation | Standard library, composable, works with std::jthread, better than atomic<bool> |
| **std::future/std::promise** | C++11 | Ergonomic blocking API wrapper | Standard async primitive, well-understood, integrates with existing callback API |
| **nlohmann/json** | 3.11+ | JSON serialization for sampling/elicitation messages | Already used in project, de facto standard |
| **MCP Spec** | 2025-11-25 | Protocol definition for client capabilities | Official schema from modelcontextprotocol repo |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| **Google Test** | 1.11+ | Unit testing | For client capability unit tests |
| **CMake** | 3.10+ | Build system | Required; FetchContent for dependencies |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| std::stop_token | std::atomic<bool> | Atomic<bool> works but less composable; stop_token integrates with std::jthread and standard library |
| std::future | Folks futures library | Folks has more features but another dependency; std::future is sufficient |
| file:// URI only | Multiple URI schemes | Spec explicitly says "must start with file:// for now" - future versions may relax this |

**Installation:**
```cmake
# C++20 required for std::stop_token
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# nlohmann/json from Phase 1
# Already included via FetchContent
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── mcpp/
│   ├── core/              # JSON-RPC 2.0 core (from Phase 1)
│   ├── protocol/          # MCP-specific protocol (from Phase 1)
│   ├── transport/         # Transport abstraction (from Phase 1)
│   ├── async/             # Async callback infrastructure (from Phase 1)
│   ├── client/            # NEW: Client capability implementations
│   │   ├── roots.h        # Roots management (list, list_changed)
│   │   ├── sampling.h     # Sampling/createMessage support
│   │   ├── elicitation.h  # Elicitation (form, URL modes)
│   │   └── cancellation.h # Cancellation support (stop_token)
│   └── futures.h          # NEW: std::future wrapper API
```

### Pattern 1: Roots Management with file:// URIs

**What:** Client implements roots/list request handler and roots/list_changed notification
**When to use:** Server requests roots list, client roots change (user switches project)

**Example:**
```cpp
// Source: MCP 2025-11-25 schema - Roots interfaces
namespace mcpp::client {

/**
 * Root directory or file that the server can operate on.
 *
 * Per spec: "The URI identifying the root. This must start with file:// for now."
 */
struct Root {
    std::string uri;  // Must start with "file://"
    std::optional<std::string> name;
};

/**
 * Result for roots/list request from server.
 */
struct ListRootsResult {
    std::vector<Root> roots;
};

/**
 * Client roots manager.
 *
 * Manages the list of roots the client advertises to the server.
 * Sends roots/list_changed notification when roots change.
 */
class RootsManager {
public:
    // Set the roots list (typically during or after initialization)
    void set_roots(std::vector<Root> roots);

    // Get current roots
    const std::vector<Root>& get_roots() const { return roots_; }

    // Register callback for sending list_changed notification
    using NotifyCallback = std::function<void()>;
    void set_notify_callback(NotifyCallback cb) { notify_cb_ = std::move(cb); }

    // Send roots/list_changed notification
    void notify_changed();

private:
    std::vector<Root> roots_;
    NotifyCallback notify_cb_;
};

// Client registers roots/list handler:
client.set_request_handler("roots/list",
    [](const JsonValue& params) -> JsonValue {
        auto roots = roots_manager.get_roots();
        json result = {{"roots", json::array()}};
        for (const auto& root : roots) {
            result["roots"].push_back({
                {"uri", root.uri},
                {"name", root.name.value_or("")}
            });
        }
        return result;
    });

// When roots change (e.g., user opens new project):
roots_manager.set_roots(new_roots);
roots_manager.notify_changed();  // Sends notification

} // namespace mcpp::client
```

### Pattern 2: Sampling with createMessage

**What:** Client handles sampling/createMessage requests from server (LLM completion via client)
**When to use:** Server needs to generate text using an LLM through the client's LLM provider

**Example:**
```cpp
// Source: MCP 2025-11-25 schema - CreateMessageRequest/Result
namespace mcpp::client {

/**
 * Sampling message content block.
 *
 * Supports text, images, audio, tool use, and tool results.
 */
struct SamplingMessage {
    std::string role;  // "user" or "assistant"
    ContentBlock content;  // Text, Image, Audio, ToolUse, or ToolResult
};

/**
 * Model preferences for sampling.
 *
 * Allows server to express priorities for model selection.
 */
struct ModelPreferences {
    std::optional<double> cost_priority;      // 0-1
    std::optional<double> speed_priority;     // 0-1
    std::optional<double> intelligence_priority;  // 0-1
    std::optional<std::vector<ModelHint>> hints;
};

/**
 * Sampling request from server.
 */
struct CreateMessageRequest {
    std::vector<SamplingMessage> messages;
    std::optional<ModelPreferences> model_preferences;
    std::optional<std::string> system_prompt;
    std::optional<std::string> include_context;  // "none", "thisServer", "allServers"
    std::optional<double> temperature;
    int64_t max_tokens;
    std::optional<std::vector<std::string>> stop_sequences;
    std::optional<json> metadata;

    // Tool use support (CLNT-03)
    std::optional<std::vector<Tool>> tools;
    std::optional<ToolChoice> tool_choice;
};

/**
 * Sampling result returned to server.
 */
struct CreateMessageResult {
    std::string role;  // "assistant"
    ContentBlock content;
    std::string model;
    std::optional<std::string> stop_reason;  // "endTurn", "stopSequence", "maxTokens", "toolUse"
};

/**
 * Sampling handler callback.
 *
 * User provides this to handle actual LLM API calls.
 * Library wraps this in the request handler.
 */
using SamplingHandler = std::function<CreateMessageResult(
    const CreateMessageRequest& request
)>;

class SamplingClient {
public:
    // Set the handler for createMessage requests
    void set_sampling_handler(SamplingHandler handler) {
        sampling_handler_ = std::move(handler);
    }

    // Register with McpClient
    void register_handlers(McpClient& client) {
        client.set_request_handler("sampling/createMessage",
            [this](const JsonValue& params) -> JsonValue {
                return handle_create_message(params);
            });
    }

private:
    JsonValue handle_create_message(const JsonValue& params) {
        // Parse request from JSON
        CreateMessageRequest request = parse_create_message(params);

        // Check if tools are requested but not supported
        if (request.tools.has_value() && !supports_tool_use()) {
            // Return error per spec
            return create_error(-32602, "Tools not supported");
        }

        // Call user's sampling handler
        CreateMessageResult result = sampling_handler_(request);

        // Convert result to JSON
        return serialize_create_message_result(result);
    }

    SamplingHandler sampling_handler_;
};

} // namespace mcpp::client
```

### Pattern 3: Elicitation (Form and URL Modes)

**What:** Client handles elicitation/create requests from server (interactive user input)
**When to use:** Server needs user input during execution (e.g., sensitive data, confirmation)

**Example:**
```cpp
// Source: MCP 2025-11-25 schema - ElicitRequest interfaces
namespace mcpp::client {

/**
 * Primitive schema definition for elicitation form mode.
 *
 * Only top-level properties allowed, no nesting.
 * Types: string, number, integer, boolean, enum (single/multi select).
 */
struct PrimitiveSchema {
    std::string type;  // "string", "number", "integer", "boolean", "array"
    std::optional<std::string> title;
    std::optional<std::string> description;
    // Type-specific fields...
};

/**
 * Form mode elicitation request.
 */
struct ElicitRequestForm {
    std::string message;
    std::string mode = "form";
    std::map<std::string, PrimitiveSchema> requested_schema;
    std::optional<std::vector<std::string>> required;
};

/**
 * URL mode elicitation request.
 */
struct ElicitRequestURL {
    std::string message;
    std::string mode = "url";
    std::string elicitation_id;
    std::string url;
};

/**
 * Elicitation result (user's response).
 */
struct ElicitResult {
    std::string action;  // "accept", "decline", "cancel"
    std::optional<std::map<std::string, std::variant<std::string, double, bool, std::vector<std::string>>>> content;
};

/**
 * Handler for elicitation requests.
 *
 * User provides this to present UI and collect input.
 */
using ElicitationHandler = std::function<ElicitResult(
    const std::variant<ElicitRequestForm, ElicitRequestURL>& request
)>;

class ElicitationClient {
public:
    void set_elicitation_handler(ElicitationHandler handler) {
        elicitation_handler_ = std::move(handler);
    }

    // Register with McpClient
    void register_handlers(McpClient& client) {
        client.set_request_handler("elicitation/create",
            [this](const JsonValue& params) -> JsonValue {
                return handle_elicitation(params);
            });

        client.set_notification_handler("notifications/elicitation/complete",
            [this](const JsonValue& params) {
                handle_elicitation_complete(params);
            });
    }

private:
    JsonValue handle_elicitation(const JsonValue& params) {
        // Parse request (form or URL mode)
        auto request = parse_elicitation_request(params);

        // Call user's handler (presents UI to user)
        ElicitResult result = elicitation_handler_(request);

        // Return result to server
        return serialize_elicit_result(result);
    }

    ElicitationHandler elicitation_handler_;
};

} // namespace mcpp::client
```

### Pattern 4: Cancellation with std::stop_token

**What:** Use C++20 std::stop_token for cooperative cancellation
**When to use:** All async operations that support cancellation

**Example:**
```cpp
// Source: C++20 standard + MCP 2025-11-25 schema
namespace mcpp::client {

/**
 * Cancellation token wrapper using std::stop_token.
 *
 * Provides cooperative cancellation for async operations.
 */
class CancellationToken {
public:
    // Default constructor: no cancellation
    CancellationToken() = default;

    // Construct from stop_token
    explicit CancellationToken(std::stop_token token)
        : token_(std::move(token)) {}

    // Check if cancellation was requested
    bool is_cancelled() const {
        return token_.stop_possible();
    }

    // Get the underlying stop_token
    const std::stop_token& get() const { return token_; }

private:
    std::stop_token token_;
};

/**
 * Cancellation source that can request cancellation.
 *
 * Used to create tokens and signal cancellation.
 */
class CancellationSource {
public:
    CancellationSource() = default;

    // Get a token for this source
    CancellationToken get_token() const {
        return CancellationToken(source_.get_token());
    }

    // Request cancellation
    void cancel() {
        source_.request_stop();
    }

    // Check if cancellation was requested
    bool is_cancelled() const {
        return source_.stop_requested();
    }

private:
    std::stop_source source_;
};

/**
 * Enhanced request context with cancellation support.
 */
struct CancelableContext {
    RequestId id;
    std::string method;
    JsonValue params;
    CancellationSource cancellation;
    async::ResponseCallback on_success;
    async::ErrorCallback on_error;
};

/**
 * Handle notifications/cancelled from server.
 *
 * Matches request ID and signals cancellation.
 */
class CancellationManager {
public:
    // Register a cancelable request
    void register_request(RequestId id, CancellationSource source) {
        std::lock_guard lock(mutex_);
        pending_[id] = std::move(source);
    }

    // Handle cancel notification
    void handle_cancelled(RequestId id, const std::optional<std::string>& reason) {
        std::lock_guard lock(mutex_);
        auto it = pending_.find(id);
        if (it != pending_.end()) {
            it->second.cancel();
            pending_.erase(it);
        }
    }

    // Remove request (on completion)
    void unregister_request(RequestId id) {
        std::lock_guard lock(mutex_);
        pending_.erase(id);
    }

private:
    std::unordered_map<RequestId, CancellationSource> pending_;
    std::mutex mutex_;
};

// User code can poll for cancellation:
void long_running_operation(CancellationToken token) {
    for (int i = 0; i < 1000; ++i) {
        if (token.is_cancelled()) {
            // Clean up and exit
            return;
        }
        // Do work...
    }
}

} // namespace mcpp::client
```

### Pattern 5: std::future Wrapper for Ergonomic Blocking API

**What:** Wrap callback-based async API with std::future for blocking calls
**When to use:** User wants to wait for result synchronously (ASYNC-04)

**Example:**
```cpp
// Source: Based on gopher-mcp future wrapper pattern
namespace mcpp {

/**
 * Future wrapper for callback-based async operations.
 *
 * Provides ergonomic blocking API on top of callback-based core.
 * Pattern: Create shared_ptr<promise>, capture in lambda, return future.
 */
template<typename T>
class FutureBuilder {
public:
    // Create a promise/future pair
    static std::pair<std::shared_ptr<std::promise<T>>, std::future<T>> create() {
        auto promise = std::make_shared<std::promise<T>>();
        auto future = promise->get_future();
        return {std::move(promise), std::move(future)};
    }

    // Wrap a callback-based API with a future
    template<typename AsyncFn>
    static std::future<T> wrap(AsyncFn&& async_fn) {
        auto [promise, future] = create();

        // Call async function with callbacks that set the promise
        async_fn(
            [promise](const T& result) {
                promise->set_value(result);
            },
            [promise](const JsonRpcError& error) {
                promise->set_exception(std::make_exception_ptr(
                    std::runtime_error(error.message)));
            }
        );

        return future;
    }
};

/**
 * High-level client API with future-based methods.
 *
 * Wraps McpClient's callback API with std::future for ergonomic blocking.
 */
class McpClientBlocking {
public:
    explicit McpClientBlocking(McpClient& client) : client_(client) {}

    // Blocking initialize
    protocol::InitializeResult initialize(
        const protocol::InitializeRequestParams& params
    ) {
        return FutureBuilder<protocol::InitializeResult>::wrap(
            [&](auto on_success, auto on_error) {
                client_.initialize(params, on_success, on_error);
            }
        ).get();  // Block until complete
    }

    // Blocking list_roots
    client::ListRootsResult list_roots() {
        return FutureBuilder<client::ListRootsResult>::wrap(
            [&](auto on_success, auto on_error) {
                client_.send_request(
                    "roots/list",
                    nullptr,
                    [on_success](const JsonValue& result) {
                        on_success(parse_list_roots_result(result));
                    },
                    on_error
                );
            }
        ).get();
    }

    // Blocking sampling/createMessage
    client::CreateMessageResult create_message(
        const client::CreateMessageRequest& request
    ) {
        return FutureBuilder<client::CreateMessageResult>::wrap(
            [&](auto on_success, auto on_error) {
                // Send sampling/createMessage request
                JsonValue params = serialize_create_message(request);
                client_.send_request(
                    "sampling/createMessage",
                    params,
                    [on_success](const JsonValue& result) {
                        on_success(parse_create_message_result(result));
                    },
                    on_error
                );
            }
        ).get();
    }

    // With timeout support
    template<typename T>
    T with_timeout(std::future<T> future, std::chrono::milliseconds timeout) {
        auto status = future.wait_for(timeout);
        if (status == std::future_status::timeout) {
            throw std::runtime_error("Request timeout");
        }
        return future.get();
    }

private:
    McpClient& client_;
};

// Usage example:
McpClient client(std::move(transport));
client.connect();

McpClientBlocking blocking(client);

try {
    auto init_result = blocking.initialize(init_params);
    std::cout << "Connected to " << init_result.serverInfo.name << std::endl;

    auto roots = blocking.list_roots();
    for (const auto& root : roots.roots) {
        std::cout << "Root: " << root.uri << std::endl;
    }
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}

} // namespace mcpp
```

### Anti-Patterns to Avoid

- **Using atomic<bool> for cancellation:** Less composable than std::stop_token, doesn't work with std::jthread
- **Blocking in callback handlers:** Can deadlock the event loop; keep handlers fast
- **Forgetting file:// URI prefix:** Spec requires roots URIs to start with "file://"
- **Ignoring tool_use stop_reason:** When stop_reason is "toolUse", client must handle tool results
- **Race conditions in cancellation:** Always unregister cancellation tokens on completion
- **Not declaring capabilities:** Server may send requests client doesn't support; declare capabilities properly

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Cancellation tokens | atomic<bool> with mutex | std::stop_token | Standard library, composable, works with std::jthread |
| Future wrappers | Custom thread management | std::promise/std::future | Standard async primitive, well-understood |
| JSON Schema validation for elicitation | Custom validator | nlohmann/json schema | Already used in project, Draft-7 support |
| Thread management for blocking API | Manual thread creation | std::async or detached thread pattern | Standard, handles exceptions properly |

**Key insight:** Cancellation in particular is tricky with race conditions. std::stop_token was designed specifically for cooperative cancellation and integrates perfectly with the standard library.

## Common Pitfalls

### Pitfall 1: std::stop_token Requires C++20

**What goes wrong:** Code doesn't compile because project uses C++17
**Why it happens:** std::stop_token was introduced in C++20
**How to avoid:** Ensure CMake sets CXX_STANDARD to 20 for this phase
**Warning signs:** Compiler error: 'stop_token' is not a member of 'std'

### Pitfall 2: Roots URI Must Be file://

**What goes wrong:** Server rejects roots list with "invalid URI" error
**Why it happens:** MCP spec explicitly states "must start with file:// for now"
**How to avoid:** Validate and prefix all root URIs with "file://"
**Warning signs:** Server returns error -32602 (invalid params) for roots/list

### Pitfall 3: Sampling Tool Use Without Capability Declaration

**What goes wrong:** Server sends tools in createMessage request, client returns error
**Why it happens:** Client must declare sampling.tools capability in initialize
**How to avoid:** Advertise sampling.tools in ClientCapabilities if supported
**Warning signs:** Server sends error when including tools in sampling request

### Pitfall 4: Cancellation After Request Completion

**What goes wrong:** Cancellation notification arrives after response already processed
**Why it happens:** Communication latency; spec acknowledges this is always possible
**How to avoid:** Check request is still pending before acting on cancellation
**Warning signs:** Spurious cancellation of already-completed requests

### Pitfall 5: std::future Double set_value

**What goes wrong:** Promise already set exception when trying to set value
**Why it happens:** Both success and error callback get invoked, or timeout fires
**How to avoid:** Use atomic flag or try-catch around set_value
**Warning signs:** "std::future_error: Promise already satisfied"

### Pitfall 6: Elicitation Form Schema Too Complex

**What goes wrong:** Server sends nested object schema, client can't validate
**Why it happens:** Spec restricts to "primitive schema" - top-level properties only
**How to avoid:** Validate schema has only top-level properties (no nesting)
**Warning signs:** Elicitation form can't be rendered, validation fails

## Code Examples

Verified patterns from official sources:

### Client Capability Declaration

```cpp
// Source: MCP 2025-11-25 schema - ClientCapabilities
namespace mcpp::protocol {

/**
 * Client capabilities advertised during initialize.
 *
 * Roots, sampling, and elicitation are the focus of Phase 3.
 */
struct ClientCapabilities {
    // Roots support (CLNT-01)
    std::optional<RootsCapability> roots;

    // Sampling support (CLNT-02, CLNT-03)
    std::optional<SamplingCapability> sampling;

    // Elicitation support (CLNT-04, CLNT-05)
    std::optional<ElicitationCapability> elicitation;

    // Note: prompts, resources, tools are SERVER capabilities
    // The client does NOT declare these
};

struct RootsCapability {
    std::optional<bool> list_changed;  // Supports list_changed notification
};

struct SamplingCapability {
    // Context inclusion support
    std::optional<bool> context;

    // Tool use support (CLNT-03)
    std::optional<bool> tools;
};

struct ElicitationCapability {
    std::optional<bool> form;  // Form mode (CLNT-04)
    std::optional<bool> url;   // URL mode (CLNT-05)
};

} // namespace mcpp::protocol
```

### Cancellation Notification Handler

```cpp
// Source: MCP 2025-11-25 schema - CancelledNotification
namespace mcpp::client {

/**
 * Handle notifications/cancelled from server.
 *
 * Matches requestId to pending request and signals cancellation.
 */
void handle_cancelled_notification(
    CancellationManager& cancel_mgr,
    const JsonValue& params
) {
    // Extract requestId (optional for cancelling non-task requests)
    std::optional<RequestId> request_id;
    if (params.contains("requestId")) {
        request_id = parse_request_id(params["requestId"]);
    }

    // Extract reason (optional)
    std::optional<std::string> reason;
    if (params.contains("reason")) {
        reason = params["reason"].get<std::string>();
    }

    if (request_id) {
        cancel_mgr.handle_cancelled(*request_id, reason);
    }
}

// Register with McpClient:
client.set_notification_handler("notifications/cancelled",
    [&cancel_mgr](const JsonValue& params) {
        handle_cancelled_notification(cancel_mgr, params);
    });

} // namespace mcpp::client
```

### Roots List Changed Notification

```cpp
// Source: MCP 2025-11-25 schema - RootsListChangedNotification
namespace mcpp::client {

/**
 * Send roots/list_changed notification to server.
 *
 * Client sends this when the list of roots changes (e.g., user opens new project).
 */
void RootsManager::notify_changed() {
    if (!notify_cb_) {
        return;  // No callback registered
    }

    // Send notification with no parameters
    notify_cb_();
}

// Register notification sender:
roots_manager.set_notify_callback([&client]() {
    client.send_notification("notifications/roots/list_changed", nullptr);
});

} // namespace mcpp::client
```

### Sampling with Tool Use

```cpp
// Source: MCP 2025-11-25 schema - Sampling with tools
namespace mcpp::client {

/**
 * Create message with tool use support.
 *
 * When server includes tools in request, model may return tool_use content.
 * Client must handle the tool loop: call tools, collect results, send back.
 */
CreateMessageResult handle_sampling_with_tools(
    const CreateMessageRequest& request,
    LlmClient& llm_client,
    McpClient& mcp_client
) {
    // Initial messages from request
    std::vector<SamplingMessage> messages = request.messages;

    // Tool loop (agentic behavior)
    while (true) {
        // Call LLM with current messages
        auto llm_result = llm_client.complete({
            .messages = messages,
            .tools = request.tools,
            .tool_choice = request.tool_choice,
            .max_tokens = request.max_tokens,
        });

        // Check if model wants to use tools
        if (llm_result.stop_reason == "toolUse") {
            // Extract tool use from content
            auto tool_use = extract_tool_use(llm_result.content);

            // Call the tool via MCP server
            auto tool_result = mcp_client.call_tool_sync(
                tool_use.name,
                tool_use.arguments
            );

            // Add tool use and result to messages
            messages.push_back({.role = "assistant", .content = llm_result.content});
            messages.push_back({.role = "user", .content = tool_result});

            // Continue loop (model gets another turn with tool results)
        } else {
            // Model finished without tool use
            return llm_result;
        }
    }
}

} // namespace mcpp::client
```

### URL Elicitation Complete Notification

```cpp
// Source: MCP 2025-11-25 schema - ElicitationCompleteNotification
namespace mcpp::client {

/**
 * Handle notifications/elicitation/complete from server.
 *
 * Server sends this after out-of-band (URL mode) elicitation completes.
 */
struct ElicitationCompleteParams {
    std::string elicitation_id;  // ID from original request
};

void handle_elicitation_complete(
    const JsonValue& params
) {
    auto complete = parse_elicitation_complete(params);

    // Match elicitation_id to pending request and notify waiting code
    // Implementation depends on async/await pattern used

    // For callback-based: invoke callback with result
    // For future-based: set promise value
}

// Register with McpClient:
client.set_notification_handler("notifications/elicitation/complete",
    [](const JsonValue& params) {
        handle_elicitation_complete(params);
    });

} // namespace mcpp::client
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| atomic<bool> cancellation | std::stop_token | C++20 (2020) | Better composability, std::jthread integration |
| Callback-only async | Callback + std::future wrappers | C++11 (2011) | Ergonomic blocking API option |
| No tool use in sampling | Full agentic tool loops | MCP 2024+ | Servers can orchestrate complex workflows |
| No elicitation | Form + URL modes | MCP 2025-11-25 | Interactive user input during tool execution |
| Roots discovery via prompts | Dedicated roots/list | MCP 2025-11-25 | Standardized root directory sharing |

**Deprecated/outdated:**
- **atomic<bool> for cancellation:** Still works but std::stop_token is superior
- **Manual thread management for futures:** Use std::async or detached threads with shared_ptr<promise>
- **Ad-hoc root sharing:** Use roots/list instead of custom protocols
- **Blocking UI for elicitation:** Support both form (in-app) and URL (out-of-band) modes

## Open Questions

Things that couldn't be fully resolved:

1. **C++20 vs C++17 Requirement**
   - What we know: std::stop_token requires C++20, Phase 1-2 used C++17
   - What's unclear: Whether to require C++20 for entire project or just this phase
   - Recommendation: Upgrade entire project to C++20; benefits outweigh compatibility concerns

2. **Tool Loop Timeout**
   - What we know: Agentic tool loops could run indefinitely
   - What's unclear: Maximum iterations or total timeout for sampling with tools
   - Recommendation: Add configurable max_iterations (default 10) and total timeout (default 5 minutes)

3. **Elicitation URL Callback Registration**
   - What we know: Server sends elicitation/complete notification with elicitation_id
   - What's unclear: How client matches notification to original request without storing state
   - Recommendation: Store map of elicitation_id -> promise/condition_variable; clean up on timeout

4. **Roots File System Access**
   - What we know: Roots are file:// URIs, but spec doesn't define how server accesses them
   - What's unclear: Whether client needs to provide file contents or server reads directly
   - Recommendation: Server uses resources/read to read file contents via client; roots are just for discovery

## Sources

### Primary (HIGH confidence)
- [MCP Schema 2025-11-25 (TypeScript)](https://raw.githubusercontent.com/modelcontextprotocol/specification/main/schema/2025-11-25/schema.ts) - Complete protocol definition, all client capability types, roots/sampling/elicitation interfaces
- [gopher-mcp mcp_client.h](/home/kotdath/omp/personal/cpp/mcpp/thirdparty/gopher-mcp/include/mcp/client/mcp_client.h) - Reference implementation for client architecture, future wrapper patterns, cancellation support
- [rmcp client handler (Rust)](https://github.com/modelcontextprotocol/rust-sdk/blob/main/crates/rmcp/src/handler/client.rs) - Reference for client handler traits, request/notification routing
- [C++20 std::stop_token specification](https://en.cppreference.com/w/cpp/thread/stop_token) - Official C++20 cancellation token documentation
- [std::future/promise reference](https://en.cppreference.com/w/cpp/thread/promise) - Standard async API documentation

### Secondary (MEDIUM confidence)
- [Mastering C++ Cooperative Cancellation](https://www.abhinavsingh.dev/blog/mastering-cpp-cooperative-cancellation/) - Best practices for C++20 stop_token, published January 2025
- [stop_token vs atomic<bool> StackOverflow](https://stackoverflow.com/questions/67633328/benefits-of-using-stdstop-source-and-stdstop-token-instead-of-stdatomicbo) - Comparison of cancellation approaches
- [MCP Model Context Protocol Explained](https://www.bigbrotherlee.com/index.php/archives/627/) - Roots/list_changed notification explanation, June 2025
- [How MCP Works](https://www.vellum.ai/blog/how-does-mcp-work) - Real-world MCP client scenarios, April 2025

### Tertiary (LOW confidence)
- [MCP Context Protocol: From Theory to Practice](https://zhuanlan.zhihu.com/p/1891139164952584541) - Sampling/createMessage flow in Chinese, April 2025
- [MCP Server Development Guide](https://github.com/cyanheads/model-contextprotocol-resources/blob/main/guides/mcp-server-development-guide.md) - Server-side perspective on client capabilities

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - std::stop_token is C++20 standard; MCP schema is authoritative
- Architecture: HIGH - Based on official MCP schema and proven gopher-mcp patterns
- Pitfalls: MEDIUM - Cancellation patterns verified from C++20 docs; MCP specifics from spec

**Research date:** 2026-01-31
**Valid until:** 2026-03-02 (30 days - MCP spec is stable but check for updates)
