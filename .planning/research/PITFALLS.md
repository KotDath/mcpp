# Pitfalls Research

**Domain:** C++ Networking / Protocol Library (MCP - Model Context Protocol)
**Researched:** 2025-01-31
**Confidence:** HIGH

## Critical Pitfalls

Mistakes that cause rewrites, major issues, or production disasters.

---

### Pitfall 1: Lambda Reference Capture in Async Callbacks

**What goes wrong:**
Lambda callbacks capture local variables by reference. When the async operation completes after the capturing scope has exited, the lambda holds dangling references. Use-after-free, heap corruption, undefined behavior.

**Why it happens:**
C++ allows `[&]` capture by reference, which is convenient for synchronous callbacks. In async contexts, the callback executes later (often on a different thread). The compiler cannot detect this lifetime mismatch. The C++ type system explicitly cannot enforce async callback safety.

**Consequences:**
- Segfaults that are difficult to reproduce (timing-dependent)
- Heap corruption that manifests far from the actual bug
- Security vulnerabilities from memory disclosure

**How to avoid:**
```cpp
// WRONG: Reference capture in async context
void sendRequestAsync(const std::string& data) {
    std::string requestId = generateId();
    transport_->asyncWrite(data, [&requestId](const auto& ec) {
        // requestId may be dangling here!
        log("Request " + requestId + " completed");
    });
}

// CORRECT: Capture by value or use shared_ptr
void sendRequestAsync(const std::string& data) {
    std::string requestId = generateId();
    transport_->asyncWrite(data, [requestId](const auto& ec) {
        // Safe: requestId is a copy
        log("Request " + requestId + " completed");
    });
}

// CORRECT: For objects that must outlive callback
void sendRequestAsync(const std::string& data) {
    auto context = std::make_shared<RequestContext>();
    context->id = generateId();
    transport_->asyncWrite(data, [context](const auto& ec) {
        // Safe: shared_ptr keeps context alive
        log("Request " + context->id + " completed");
    });
}
```

**Warning signs:**
- Use of `[&]` or `[&var]` in any lambda passed to an async operation
- Capturing `this` in a lambda where the owning object might be destroyed
- Heisenbugs that only appear under load or with ThreadSanitizer

**Phase to address:** Phase 1 (Foundation) - Establish coding guidelines and static analysis rules before writing async code.

**Sources:**
- StackOverflow: "An async callback is a fundamentally different beast than a sync callback, and the C++ type system cannot enforce safety here" - [Is there a way to prevent lambda capture by reference](https://stackoverflow.com/questions/39886531/is-there-a-way-to-prevent-lambda-capture-by-reference)
- PVS-Studio Blog: "C++ programmer's guide to undefined behavior" (January 2025) - covers implicit reference captures

---

### Pitfall 2: std::string_view Dangling References

**What goes wrong:**
`std::string_view` is a non-owning reference. When used to store parsed JSON values, pass temporary strings, or expose through API boundaries, it silently creates dangling references.

**Why it happens:**
`std::string_view` was designed for function parameters (borrowed data). It's tempting to use it everywhere to avoid allocations. But it doesn't own the underlying string - if the source is destroyed or modified, the view becomes invalid.

**Consequences:**
- Silent data corruption (reads garbage that looks like valid data)
- Security vulnerabilities from reading unintended memory
- Crashes that are extremely difficult to debug

**How to avoid:**
```cpp
// WRONG: Returning string_view from function
std::string_view getName() {
    std::string name = parseName();  // Temporary destroyed when function returns
    return name;  // Dangling!
}

// WRONG: Storing string_view from JSON
struct Tool {
    std::string_view name;  // Dangling when JSON parser is destroyed
    std::string_view description;
};

// CORRECT: Use std::string for owned data
struct Tool {
    std::string name;
    std::string description;
};

// CORRECT: string_view only as function parameter
void processName(std::string_view name);  // Safe: borrowed for call duration

// CORRECT: If you must store a view, document lifetime requirement
class JsonParser {
public:
    // Returned views valid only until next parser call
    std::string_view getString() const;
};
```

**Warning signs:**
- `std::string_view` as a member variable
- `std::string_view` as a return type (except documented temporary views)
- Storing string_views from temporary expressions (e.g., `substr()` results)
- Research indicating "70% of C++ Security Bugs Stem from std::string_view Misuse"

**Phase to address:** Phase 2 (Protocol Layer) - When defining JSON-RPC types and message structures.

**Sources:**
- "70% of C++ Security Bugs Stem from std::string_view Misuse" (Medium 2024) - [link](https://medium.com/@dikhyantkrishnadalai/70-of-c-security-bugs-stem-from-std-string-view-misuse-heres-how-to-prevent-them-4aca2ba9bb86)
- "To string_view, or not to string_view" (May 2024) - [link](https://tomhultonharrop.com/posts/string-view/)
- "Detecting lifetime errors of std::string_view objects in C++" (arXiv 2024) - [link](https://arxiv.org/abs/2408.09325)

---

### Pitfall 3: std::shared_ptr Thread Safety Misconception

**What goes wrong:**
Developers assume `std::shared_ptr` makes the pointed-to object thread-safe. It doesn't. Only the reference count is atomic - concurrent access to the managed object without external synchronization is a data race.

**Why it happens:**
The naming is misleading. "shared" implies "shared safely." The reference count operations are indeed thread-safe, but the object itself is not automatically protected.

**Consequences:**
- Race conditions on shared objects
- Use-after-free when one thread destroys object while another accesses it
- Incorrect `use_count()` assumptions (even when it returns 1, other threads may still access)

**How to avoid:**
```cpp
// WRONG: Assuming shared_ptr provides thread safety
class RequestManager {
    std::shared_ptr<Request> current_;
    void updateRequest(const Request& req) {
        // Data race! Multiple threads can call this
        *current_ = req;
    }
};

// CORRECT: Use mutex for object access
class RequestManager {
    std::shared_ptr<Request> current_;
    std::mutex mutex_;
    void updateRequest(const Request& req) {
        std::lock_guard lock(mutex_);
        *current_ = req;
    }
};

// CORRECT: Use atomic_store/load for pointer itself
std::shared_ptr<Config> global_config;
void updateConfig(std::shared_ptr<Config> new_config) {
    std::atomic_store(&global_config, new_config);
}

// CORRECT: Each thread gets its own copy
void processRequest(std::shared_ptr<Request> req) {
    // Safe: req is owned by this thread
    req->process();
}
```

**Warning signs:**
- Using `shared_ptr` without mutex for concurrent write access
- Checking `use_count() == 1` to determine safety (cppreference explicitly warns against this)
- Multiple threads calling non-const methods on same shared_ptr target

**Phase to address:** Phase 1 (Foundation) - Core types must establish correct patterns before use spreads.

**Sources:**
- StackOverflow: "std::shared_ptr is a mechanism to ensure object destruction, not for thread synchronization"
- Lei Mao's Blog: "C++ Shared Pointer Thread-Safety" (November 2024) - [link](https://leimao.github.io/blog/CPP-Shared-Ptr-Thread-Safety/)
- cppreference: use_count() warning - "When use_count returns 1, it does not imply that the object is safe to modify"
- Blog: "Of common problems with shared pointers" (September 2024) - [link](https://twdev.blog/2024/09/sharedptr/)

---

### Pitfall 4: Blocking the Event Loop

**What goes wrong:**
User callbacks or handlers perform blocking operations (file I/O, heavy computation, network calls) on the event loop thread. The entire event loop stalls. No other connections can be processed. Deadlocks occur when waiting for the same event loop.

**Why it happens:**
It's easier to write synchronous code. Users "just want to read a file" and don't realize they're blocking all connections. Without explicit async-only API, users naturally reach for blocking calls.

**Consequences:**
- All connections stall during blocking operation
- Watchdog timeouts
- Cascading failures as other connections time out
- Deadlocks when waiting for event loop that can't progress

**How to avoid:**
```cpp
// WRONG: Blocking in event loop callback
void onToolCall(const json& args) {
    std::string result = executeHeavyComputation(args);  // Blocks!
    sendResponse(result);
}

// CORRECT: Offload to worker thread
void onToolCall(const json& args) {
    threadPool_->post([this, args]() {
        std::string result = executeHeavyComputation(args);
        // Post back to event loop for response
        dispatcher_->post([this, result]() {
            sendResponse(result);
        });
    });
}

// CORRECT: API design - only async API exposed
class McpClient {
    // No blocking callTool() method exists
    std::future<ToolResult> callToolAsync(const std::string& name, const json& args);
    // Users who want blocking must explicitly:
    // auto result = client.callToolAsync(...).get();
};
```

**Warning signs:**
- Synchronous network I/O in handlers
- `std::this_thread::sleep_for()` anywhere in event loop code
- File I/O without async equivalent
- Long-running computations in callbacks
- All connections stop responding when one request processes

**Phase to address:** Phase 1 (Foundation) - Event loop design must establish async-only patterns from day one.

**Sources:**
- Chromium C++ Style Guide: Recommends coroutines as safer alternative to callbacks for async code
- CSDN Blog: "C++ std::async underlying mechanisms: How to efficiently avoid thread blocking" (2025)

---

### Pitfall 5: Lock Contention on Hot Path

**What goes wrong:**
Protecting frequently accessed data structures (request map, connection state, message queues) with mutexes. At scale, threads spend most time waiting for locks rather than doing work.

**Why it happens:**
Naive threading approach: protect everything with mutexes. It's "correct" but performs terribly. Lock contention scales superlinearly with thread count.

**Consequences:**
- Performance degrades with more threads (not improves)
- Latency spikes under load
- Throughput plateaus far below hardware capacity

**How to avoid:**
```cpp
// WRONG: Shared mutex on hot path
class RequestTracker {
    std::mutex mutex_;
    std::unordered_map<RequestId, RequestContext> requests_;
    Response onResponse(const Response& resp) {
        std::lock_guard lock(mutex_);  // Contention!
        auto it = requests_.find(resp.id);
        // ...
    }
};

// CORRECT: Thread-local dispatcher pattern
class RequestTracker {
    // Only accessed from dispatcher thread
    std::unordered_map<RequestId, RequestContext> requests_;
    Response onResponse(const Response& resp) {
        // No lock needed - always on dispatcher thread
        auto it = requests_.find(resp.id);
        // ...
    }
};
// Cross-thread via thread-safe posting only
```

**Warning signs:**
- Mutex contention in profiler under load
- Performance doesn't improve (or worsens) with more threads
- Locks protecting data structures accessed on every request

**Phase to address:** Phase 1 (Foundation) - Architecture must commit to thread-local or lock-free design before implementation.

**Sources:**
- Envoy Proxy Threading Model: "no locks on data path" architecture - [link](https://blog.envoyproxy.io/envoy-threading-model-a8d44b922310)
- Architecture Research: Single-threaded event loop with thread-safe posting pattern

---

### Pitfall 6: Incorrect Memory Ordering with Atomics

**What goes wrong:**
Using `std::atomic` with default memory ordering or incorrectly ordered operations. Data races manifest as subtle bugs that only appear on specific architectures or under optimization.

**Why it happens:**
Memory ordering is complex. Default `seq_cst` is safe but slow. Developers "optimize" to `relaxed` without understanding the implications. Or they use atomics thinking they provide full synchronization when they don't.

**Consequences:**
- Subtle bugs that only appear on ARM/POWER (not x86)
- Bugs that only appear with optimizations enabled
- Reordering violations causing stale data access

**How to avoid:**
```cpp
// WRONG: Assuming relaxed provides synchronization
std::atomic<bool> ready_;
void producer() {
    data_ = compute();  // Non-atomic
    ready_.store(true, std::memory_order_relaxed);  // May reorder!
}
void consumer() {
    if (ready_.load(std::memory_order_relaxed)) {
        use(data_);  // May see uninitialized data_!
    }
}

// CORRECT: Use acquire/release for synchronization
void producer() {
    data_ = compute();
    ready_.store(true, std::memory_order_release);
}
void consumer() {
    if (ready_.load(std::memory_order_acquire)) {
        use(data_);  // Safe: data_ is initialized
    }
}

// BEST: Default to seq_cst until profiling shows need
std::atomic<bool> ready_;
void producer() { ready_.store(true); }  // seq_cst is safe default
```

**Warning signs:**
- Use of `memory_order_relaxed` for synchronization (not just counters)
- Data races that only appear on ARM
- Assumptions about compiler not reordering across atomics

**Phase to address:** Phase 1 (Foundation) - Any atomic usage should be code-reviewed and documented.

**Sources:**
- StackOverflow: GCC relaxed atomic operation and compiler fence behavior - [link](https://stackoverflow.com/questions/79323385/does-gcc-treat-relaxed-atomic-operation-as-a-compiler-fence)
- C++ Standard Library Active Issues List: Ongoing atomic-related issues

---

### Pitfall 7: Coroutine Lifetime Issues

**What goes wrong:**
C++20 coroutines capture references or hold pointers that become invalid before the coroutine resumes. Or coroutine frame is destroyed while suspended.

**Why it happens:**
Coroutines suspend and resume across different points in time. It's easy to forget what's still alive when resumption happens. The compiler doesn't catch lifetime violations in coroutines.

**Consequences:**
- Use-after-free when coroutine resumes after captured object destroyed
- Dangling references to coroutine frame
- Extremely difficult-to-reproduce bugs

**How to avoid:**
```cpp
// WRONG: Capturing reference in coroutine
Task<Response> sendRequest(const std::string& data) {
    co_return co_await transport_->write(data);  // data may be dangling
}

// CORRECT: Capture by value
Task<Response> sendRequest(std::string data) {
    co_return co_await transport_->write(data);  // Safe copy
}

// CORRECT: Use shared_ptr for ownership
Task<Response> sendRequest(std::shared_ptr<RequestContext> ctx) {
    co_return co_await transport_->write(ctx->data);
}

// WRONG: Returning coroutine with temporary
Task<void> process() {
    auto task = doSomething();  // task destructor may kill coroutine
    // Forget to co_await or store task
}

// CORRECT: Always handle coroutine return
Task<void> process() {
    auto task = doSomething();
    co_await task;  // Properly await
}
```

**Warning signs:**
- `const&` parameters to coroutine functions
- Coroutines that outlive their capturing scope
- "Immediately invoked coroutine lambdas" without explicit lifetime management
- CppCon 2024/2025 talks highlighting "lifetime issues are difficult to manage without established best practices"

**Phase to address:** Phase 5 (High-Level API) - If using coroutines for ergonomics, establish strict guidelines first.

**Sources:**
- ACCU 2025: "C++ Coroutines Demystified" by Phil Nash - [link](https://www.youtube.com/watch?v=b6pYieNd_OY)
- CppCon 2024: "Deciphering C++ Coroutines" series by Andreas Weis
- "C++ Coroutines and Structured Concurrency in Practice" - emphasizes lifetime management challenges

---

### Pitfall 8: JSON-RPC Request ID Mishandling

**What goes wrong:**
Request IDs collide, are not tracked properly, or responses are matched to wrong requests. Race conditions in request ID generation and tracking cause responses to be lost or misdirected.

**Why it happens:**
Request ID generation and tracking is subtle. Incrementing IDs can collide when connections reset. Using random IDs may collide. The request map must be thread-safe. Cleanup of timed-out requests must not remove pending responses.

**Consequences:**
- Responses matched to wrong requests (data leakage, incorrect results)
- Lost responses (memory leak, hangs)
- Request ID exhaustion

**How to avoid:**
```cpp
// WRONG: Simple incrementing ID (collides after reconnect)
class RequestTracker {
    int nextId_ = 0;
    RequestId generateId() { return ++nextId_; }
};

// WRONG: Not cleaning up old IDs
class RequestTracker {
    std::unordered_map<RequestId, Context> pending_;
    void onResponse(const Response& resp) {
        auto it = pending_.find(resp.id);
        pending_.erase(it);  // Only removed on response
        // Timeouts leak!
    }
};

// CORRECT: Connection-scoped IDs with timeout cleanup
class RequestTracker {
    uint64_t nextId_ = 0;
    std::unordered_map<RequestId, Context> pending_;
    std::unordered_map<RequestId, Timestamp> requestTimestamps_;

    RequestId generateId() {
        return ++nextId_;  // Reset on new connection
    }

    void onResponse(const Response& resp) {
        auto it = pending_.find(resp.id);
        if (it != pending_.end()) {
            auto ctx = std::move(it->second);
            pending_.erase(it);
            requestTimestamps_.erase(resp.id);
            ctx.promise.setValue(resp);
        }
    }

    void cleanupTimeouts(std::chrono::milliseconds timeout) {
        auto now = Clock::now();
        for (auto it = requestTimestamps_.begin(); it != requestTimestamps_.end(); ) {
            if (now - it->second > timeout) {
                auto reqIt = pending_.find(it->first);
                if (reqIt != pending_.end()) {
                    reqIt->second.promise.set_exception(
                        std::make_exception_ptr(TimeoutError()));
                    pending_.erase(reqIt);
                }
                it = requestTimestamps_.erase(it);
            } else {
                ++it;
            }
        }
    }
};
```

**Warning signs:**
- Increment-only request IDs
- No timeout handling for pending requests
- Request map grows without bound
- Responses arriving for "wrong" requests

**Phase to address:** Phase 2 (Protocol Layer) - JSON-RPC request tracking is core functionality.

**Sources:**
- MCP Specification: Request/Response matching rules
- JSON-RPC 2.0 Specification: ID handling requirements - [link](https://www.jsonrpc.org/specification)

---

### Pitfall 9: stdio Transport Deadlocks

**What goes wrong:**
When using stdio transport, writing to stdout and reading from stdin can deadlock if the subprocess buffer fills. Parent process waits for child to read stdout; child waits for parent to read stdout. Circular wait.

**Why it happens:**
Blocking I/O on stdio. The subprocess writes to stdout, but the parent isn't reading yet (blocked on something else). Buffer fills, write blocks. Parent waits for child response, child waits for buffer space.

**Consequences:**
- Complete hang of both processes
- Requires SIGKILL to terminate
- Intermittent (depends on buffer size, timing)

**How to avoid:**
```cpp
// WRONG: Blocking read/write on stdio
class StdioTransport {
    void sendMessage(const std::string& msg) {
        std::cout << msg << std::endl;  // May block if buffer full!
    }
    std::string readMessage() {
        std::string line;
        std::getline(std::cin, line);  // May block waiting!
        return line;
    }
};

// CORRECT: Non-blocking I/O with event loop
class StdioTransport {
    Dispatcher& dispatcher_;
    std::string readBuffer_;

    void start() {
        // Register stdin for read events
        dispatcher_.registerFileEvent(STDIN_FILENO, FileEvent::Read,
            [this](int fd) {
                char buffer[4096];
                ssize_t n = read(fd, buffer, sizeof(buffer));
                if (n > 0) {
                    readBuffer_.append(buffer, n);
                    processMessages();
                }
            });
    }

    void sendMessage(const std::string& msg) {
        // Use event loop for write readiness
        dispatcher_.registerFileEvent(STDOUT_FILENO, FileEvent::Write,
            [this, msg](int fd) {
                write(fd, msg.data(), msg.size());
            });
    }
};

// ALSO: Always handle stderr on separate thread to prevent blocking
```

**Warning signs:**
- Using `std::cin`/`std::cout` directly for subprocess communication
- Hangs that depend on message size
- Works in development, fails in production (different buffer sizes)

**Phase to address:** Phase 3 (Transport Implementations) - stdio transport requires careful non-blocking design.

**Sources:**
- MCP Specification: stdio transport requirements
- Common subprocess I/O patterns: "Blocking stdio reading" anti-pattern

---

### Pitfall 10: MCP Progress/Cancellation Race Conditions

**What goes wrong:**
Progress notifications arrive after request completes. Cancellation notification races with in-progress operation. Result and progress for same token arrive out of order.

**Why it happens:**
MCP allows progress tokens to be attached to requests. Progress notifications are separate from the response. Cancellation is also a notification. These can all race. Without careful state machine design, you'll set promises twice, miss progress, or fail to cancel.

**Consequences:**
- "Promise already satisfied" exceptions
- Lost progress updates
- Cancellation silently ignored
- Memory leaks from uncleaned state

**How to avoid:**
```cpp
// WRONG: Simple state doesn't handle races
enum class RequestState { Pending, Complete };
RequestState state_;
void onProgress(const Progress& p) {
    if (state_ == RequestState::Complete) return;
    emitProgress(p);
}
void onResponse(const Response& r) {
    state_ = RequestState::Complete;
    promise_.setValue(r);
}

// CORRECT: Explicit state machine for all transitions
enum class RequestState {
    Pending,
    Canceling,
    Progress,
    Complete,
    Failed
};

class RequestContext {
    RequestState state_ = RequestState::Pending;
    std::promise<Response> promise_;
    ProgressCallback progressCallback_;

    bool transitionTo(RequestState newState) {
        // Validate state transitions
        switch (state_) {
        case RequestState::Pending:
            if (newState == RequestState::Cancelling ||
                newState == RequestState::Progress ||
                newState == RequestState::Complete ||
                newState == RequestState::Failed) {
                state_ = newState;
                return true;
            }
            return false;
        case RequestState::Cancelling:
            if (newState == RequestState::Complete || newState == RequestState::Failed) {
                state_ = newState;
                return true;
            }
            return false;
        case RequestState::Progress:
            // Progress -> Complete/Failed/Canceling all valid
            state_ = newState;
            return true;
        case RequestState::Complete:
        case RequestState::Failed:
            return false;  // Terminal states
        }
        return false;
    }

    void onProgress(const Progress& p) {
        if (transitionTo(RequestState::Progress)) {
            if (progressCallback_) {
                progressCallback_(p);
            }
            // Transition back to non-progress state
            // (implementation tracks progress state separately)
        }
    }

    void onResponse(const Response& r) {
        if (transitionTo(RequestState::Complete)) {
            promise_.setValue(r);
        } else {
            // Log or handle duplicate response
        }
    }

    void onCancel(const Cancel& c) {
        if (transitionTo(RequestState::Cancelling)) {
            // Notify in-flight operation
        }
    }
};
```

**Warning signs:**
- Simple boolean "complete" flag
- No explicit state machine for request lifecycle
- Progress delivered after completion
- Testing shows rare "promise already satisfied" errors

**Phase to address:** Phase 2 (Protocol Layer) - Progress and cancellation are required MCP features.

**Sources:**
- MCP Specification: Progress and Cancellation semantics
- GitHub Discussion: "Asynchronous operations in MCP" (#491)

---

## Moderate Pitfalls

Mistakes that cause delays or technical debt but not rewrites.

---

### Pitfall 11: Using Wrong JSON Library for Performance

**What goes wrong:**
Choosing `nlohmann/json` for hot-path parsing when performance matters. It's convenient but slow. At scale, JSON parsing becomes the bottleneck.

**Why it happens:**
`nlohmann/json` is the de facto standard C++ JSON library. It's easy to use, header-only, well-documented. But it's not optimized for speed.

**Consequences:**
- JSON parsing dominates CPU profile at scale
- Throughput limited by parsing speed
- Expensive migration to faster library later

**How to avoid:**
- Use `nlohmann/json` for convenience (config, low-rate messages)
- Use `simdjson` for hot-path parsing (high-throughput message paths)
- Benchmark both with representative workload
- Design JSON abstraction layer to allow swapping

**Phase to address:** Phase 2 (Protocol Layer) - Choose JSON library before committing to API.

**Sources:**
- Reddit r/cpp: "What's the go-to JSON parser in 2024/2025?" - "If you need performance, avoid nlohmann/json"
- CppCon 2025: "Reflection-based JSON in C++ at Gigabytes per Second" - Modern approaches

---

### Pitfall 12: Protocol Buffer Move Semantics Misuse

**What goes wrong:**
Using `std::move()` on protobuf objects incorrectly. Moving from const objects does nothing (copy). Moving in return statements prevents copy elision. Using moved-from objects.

**Why it happens:**
Protobuf historically didn't support move semantics well. Even now, move semantics have subtle gotchas. Developers "move everything" without understanding protobuf specifics.

**Consequences:**
- Unexpected copies (performance hit)
- Use-after-move on protobuf objects
- "Use after std::move" cited as "most notorious C++ errors"

**How to avoid:**
```cpp
// WRONG: Moving from const
void process(const Request& req) {
    cache_.insert(std::make_pair(id, std::move(req)));  // Copy, not move!
}

// WRONG: Move in return statement prevents copy elision
Request buildRequest() {
    Request req;
    req.set_name("test");
    return std::move(req);  // WRONG: prevents RVO/NRVO
}

// CORRECT: Just return
Request buildRequest() {
    Request req;
    req.set_name("test");
    return req;  // Compiler applies copy elision
}

// CORRECT: Move from non-const
void process(Request&& req) {
    cache_.insert(std::make_pair(id, std::move(req)));  // Actual move
}
```

**Phase to address:** Phase 4 (Filter Implementations) - When using protobuf or similar serialization.

**Sources:**
- Protobuf News: "New RepeatedPtrField Layout" (September 2025) - changing copy/move semantics
- PVS-Studio: "use after std::move" warning V012

---

### Pitfall 13: Header-Only Library ABI Fragility

**What goes wrong:**
Distributing header-only library without considering ABI stability. Users compile with different compiler flags, standards, or STL implementations. Incompatibilities manifest as link errors or runtime crashes.

**Why it happens:**
Header-only is convenient (no build step). But headers are compiled into user's binary with their compiler settings. ABI is not stable across compilers or even compiler options.

**Consequences:**
- "Compatible compilation units" errors
- Crashes when crossing module boundaries
- Users cannot upgrade library without recompiling all dependencies

**How to avoid:**
```cpp
// WRONG: Exposing standard library types in ABI
class API {
    std::string name_;  // Size/layout varies by compiler
    std::vector<int> data_;
};

// CORRECT: Pimpl for ABI stability
class API {
public:
    API();
    ~API();
    // ...
private:
    class Impl;
    std::unique_ptr<Impl> impl_;  // Stable ABI
};

// CORRECT: Or explicitly document ABI limitations
// "This header-only library requires rebuilding all dependencies when upgrading"
```

**Phase to address:** Phase 5 (High-Level API) - Public API design must consider ABI.

**Sources:**
- CSDN Blog: "Solving C++ project compatibility nightmare: cpp-httplib ABI issues" (September 2025)
- C++ Core Guidelines I.27: "For stable library ABI, consider the Pimpl idiom"
- Stack Overflow: "One of the biggest uses of pimpl is creating stable C++ ABIs"

---

### Pitfall 14: Forgetting to Handle stdio Transport Newlines

**What goes wrong:**
MCP stdio transport requires newline-delimited JSON messages. Forgetting to add trailing `\n` or misreading partial lines causes protocol violations.

**Why it happens:**
The spec says "newline delimited" but it's easy to forget. Standard JSON serialization doesn't add newlines. `std::getline` reads line-by-line but may read incomplete JSON if newlines appear within JSON strings.

**Consequences:**
- Messages not recognized by peer
- Invalid JSON errors
- Intermittent failures (works until JSON contains embedded newlines)

**How to avoid:**
```cpp
// WRONG: Just serialize JSON
std::string msg = json.dump();  // No newline!
write(stdout, msg.data(), msg.size());

// CORRECT: Add newline delimiter
std::string msg = json.dump() + "\n";
write(stdout, msg.data(), msg.size());

// CORRECT: Read by newline, not assuming one JSON per read
void processInput() {
    std::string buffer;
    char chunk[4096];
    ssize_t n;
    while ((n = read(STDIN_FILENO, chunk, sizeof(chunk))) > 0) {
        buffer.append(chunk, n);
        size_t pos = 0;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            handleMessage(line);
        }
    }
}
```

**Phase to address:** Phase 3 (Transport Implementations) - stdio framing.

**Sources:**
- MCP Specification: stdio transport format
- Chinese documentation: "MCP communication mechanism" - details on stdio newline requirements

---

## Minor Pitfalls

Mistakes that cause annoyance but are fixable.

---

### Pitfall 15: Future Not Retrieved

**What goes wrong:**
`std::future` returned from async operation is discarded. The promise is never satisfied or its destructor blocks waiting.

**Why it happens:**
Caller doesn't care about result, or forgets to store the future. `std::future` destructor may block (shared_future) or may not (unique future), making behavior inconsistent.

**Consequences:**
- Silent blocking in destructor (shared_future)
- Fire-and-forget operations that silently fail
- Unhandled exceptions in detached tasks

**How to avoid:**
```cpp
// WRONG: Discarding future
void sendNotification(const json& msg) {
    client_.sendAsync(msg);  // future discarded
}

// CORRECT: Explicitly handle or store
void sendNotification(const json& msg) {
    auto future = client_.sendAsync(msg);
    // Store or explicitly detach
}

// CORRECT: Provide fire-and-forget API that's explicit
void sendNotification(const json& msg) {
    client_.sendAndForget(msg);  // Clearly intentional
}
```

---

### Pitfall 16: Integer Overflow in Message Length

**What goes wrong:**
Message length prefix doesn't fit in integer type. Signed integer overflow causes undefined behavior.

**Why it happens:**
Using `int32_t` for length prefix when message could exceed 2GB. Not validating length before allocating buffer.

**Consequences:**
- Negative length interpreted as huge allocation
- Integer overflow → buffer overflow
- Denial of service via malicious length prefix

**How to avoid:**
```cpp
// WRONG: Using signed int for length
int32_t length = readLength();
std::string buffer(length, '\0');  // May allocate gigabytes

// CORRECT: Use size_t, validate max
uint32_t lengthRaw = readLength();
constexpr size_t MAX_MESSAGE_SIZE = 100 * 1024 * 1024;  // 100MB
if (lengthRaw > MAX_MESSAGE_SIZE) {
    throw std::runtime_error("Message too large");
}
size_t length = lengthRaw;
std::string buffer(length, '\0');
```

**Phase to address:** Phase 2 (Protocol Layer) - Message framing.

---

### Pitfall 17: Ignoring MCP Specification Versioning

**What goes wrong:**
Not handling protocol version negotiation. Assuming peer speaks same version. Breaking spec changes cause incompatibility.

**Why it happens:**
MCP is evolving quickly (2025-11-25 spec, with updates scheduled). Implementing for current spec without versioning creates future compatibility issues.

**Consequences:**
- Cannot interoperate with different spec versions
- Breaking changes require coordinated upgrade
- "Works now, breaks later"

**How to avoid:**
- Always send protocol version in initialize
- Validate peer's version is compatible
- Document version support matrix
- Test against multiple spec versions

**Phase to address:** Phase 2 (Protocol Layer) - Initialization handshake.

**Sources:**
- MCP Specification: Versioning requirements
- ModelContextProtocol Blog: "Update on the Next MCP Protocol Release" (September 2025)

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| **Global state for configuration** | Simpler initialization, easier access | Cannot support multiple connections, testing issues, thread-safety complexity | Never - use per-instance config |
| **Blocking I/O in MVP** | Faster to implement, easier debugging | Performance issues, difficult to make async later | Only for prototype, not production |
| **Hard-coded transport** | Simpler code, fewer abstractions | Cannot add transports without breaking API | Only during initial development |
| **Skipping error handling** | Faster development | Crashes in production, difficult debugging | Never - handle errors from day one |
| **Using nlohmann/json everywhere** | One library to learn | Performance bottleneck at scale | Use for MVP, switch to simdjson for hot paths later |
| **Returning std::string_view from API** | Avoids copies | Dangling references, API brittleness | Never in public API |
| **Mixing sync/async APIs** | Ergonomic for simple cases | Users accidentally block event loop | Provide async-only, users can wrap with .get() |
| **Thread-unsafe first, add locks later** | Faster initial development | Lock contention, requires rewrite | Never - design thread-safety from start |
| **Header-only distribution** | Easier integration | ABI fragility, longer compile times | Acceptable for small libraries, document ABI limitations |
| **Subprocess spawning in library** | Convenient for stdio transport | Security issues, platform differences | Users should spawn processes, library speaks to provided streams |

---

## Integration Gotchas

Common mistakes when connecting to external services.

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| **nlohmann/json** | Assuming thread safety - parsing from multiple threads without locks | Each thread parses its own JSON, or protect with mutex |
| **libevent** | Mixing libevent threads with own threads incorrectly | Use libevent's threading functions or single-threaded with posting |
| **OpenSSL** | Not initializing for multi-threaded use | Call `SSL_library_init()` and set thread locking callbacks |
| **subprocess spawning** | Blocking on child stdout/stderr causing deadlocks | Use non-blocking I/O or separate threads for stdout/stderr |
| **HTTP via libcurl** | Using libcurl's easy interface in event loop (blocks) | Use libcurl's multi interface or separate thread pool |
| **ThreadPool** | Oversubscribing (more threads than cores) | Use hardware_concurrency - 1 for event loop, rest for work |
| **spdlog** | Creating loggers from multiple threads without init | Initialize logging before threading, or use thread-safe logger factory |

---

## Performance Traps

Patterns that work at small scale but fail as usage grows.

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| **Locking on every request** | Latency increases with load, CPU stuck in kernel | Thread-local dispatcher pattern, lock-free posting | 100+ concurrent requests |
| **Copying every message** | High CPU usage, memory allocations spike | Use views, move semantics, zero-copy parsing | 1000+ messages/second |
| **nlohmann/json on hot path** | JSON parsing dominates CPU profile | Use simdjson for hot-path messages | 10,000+ RPC calls/second |
| **No connection pooling** | High latency for HTTP transport, connection churn | Pool connections, reuse with keep-alive | Multiple concurrent HTTP requests |
| **Synchronous timeout handling** | Timeouts delayed by other work | Asynchronous deadline timers in event loop | Any timeout-sensitive workload |
| **Unbounded queues** | Memory grows without bound under load | Bounded queues with backpressure | Producer faster than consumer |
| **Per-message allocations** | malloc/free dominates CPU | Buffer pooling, arena allocators | High message rate |
| **Logging in hot path** | I/O on every request slows everything | Sampling, async logging, level filtering | Production under load |

---

## Security Mistakes

Domain-specific security issues beyond general web security.

| Mistake | Risk | Prevention |
|---------|------|------------|
| **Not validating message size** | DoS via huge message allocation | Enforce max message size before parsing |
| **Trusting client-provided request IDs** | Request confusion, privilege escalation | Generate own IDs, map client IDs internally |
| **Not sanitizing error messages** | Information disclosure | Generic errors in production, verbose only in debug |
| **Allowing arbitrary filesystem access** | Path traversal, data exposure | Validate paths, restrict to allowed roots, use Roots capability |
| **Ignoring MCP authorization** | Unauthorized access to tools/resources | Implement authorization per MCP spec |
| ** subprocess injection** | Command execution vulnerabilities | Never shell-escape user input, use explicit args |
| **Not validating JSON Schema input** | Invalid data crashes tools | Validate all tool inputs against declared schema |
| **Exposing internal state via errors** | Information leakage | Sanitize error messages before sending to client |

**Sources:**
- CVE-2025-6515: MCP Prompt Hijacking Attack (October 2025) - [link](https://jfrog.com/blog/mcp-prompt-hijacking-vulnerability/)
- Pomerium Blog: "June 2025 MCP Content Round-Up: Incidents, Updates" - hundreds of misconfigured MCP servers

---

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces.

- [ ] **JSON-RPC Layer:** Often missing request ID deduplication — verify responses aren't processed multiple times
- [ ] **Cancellation:** Often missing cleanup of canceled requests — verify pending requests map is cleaned up
- [ ] **Progress:** Often missing late progress handling — verify progress after response is ignored
- [ ] **stdio Transport:** Often missing stderr handling — verify stderr doesn't block stdout
- [ ] **Streamable HTTP:** Often missing session resumption — verify reconnect works with existing session
- [ ] **Error Handling:** Often missing error JSON schema validation — verify error responses follow spec
- [ ] **Capability Negotiation:** Often missing capability downgrade — verify client/server handle mismatched capabilities
- [ ] **Resource Subscriptions:** Often missing unsubscribe cleanup — verify unsubscribed resources don't send notifications
- [ ] **Roots:** Often missing URI validation — verify file:// URIs are within declared roots
- [ ] **Sampling:** Often missing model preference handling — verify sampling respects client model preferences
- [ ] **Elicitation URL Mode:** Often missing CSRF protection — verify URL responses can't be forged
- [ ] **Initialization:** Often missing shutdown sequence — verify graceful close sends shutdown notification

---

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| **Lambda dangling reference** | HIGH | Find all `[&]` captures in async contexts via code review and static analysis, convert to value captures or shared_ptr |
| **string_view dangling** | MEDIUM | Audit all string_view members and return types, replace with std::string for owned data |
| **shared_ptr misuse** | MEDIUM | Add mutex protection for all shared objects accessed by multiple threads |
| **Blocking event loop** | HIGH | Profile to find blocking operations, move to worker threads, rewrite API to be async-only |
| **Lock contention** | HIGH | Redesign to thread-local pattern, remove all locks from data path |
| **Wrong memory order** | MEDIUM | Audit all atomics, default to seq_cst, relax only after profiling shows benefit |
| **Coroutine lifetime** | HIGH | Disable coroutines if not expertly implemented, use callback-based API instead |
| **Request ID collisions** | MEDIUM | Implement proper request tracking with timeout cleanup, connection-scoped IDs |
| **stdio deadlock** | MEDIUM | Rewrite with non-blocking I/O and event loop integration |
| **Progress/cancellation races** | HIGH | Implement explicit state machine with validated transitions |
| **nlohmann/json bottleneck** | MEDIUM | Profile and replace hot paths with simdjson, add JSON abstraction layer |
| **Header-only ABI issues** | HIGH | Add Pimpl to public API classes or document ABI instability |

---

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls.

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Lambda reference capture | Phase 1 (Foundation) | Static analysis, code review guidelines |
| string_view dangling | Phase 2 (Protocol Layer) | Review all string_view usage, lifetime annotations |
| shared_ptr misuse | Phase 1 (Foundation) | ThreadSanitizer, code review for shared mutable state |
| Blocking event loop | Phase 1 (Foundation) | Profiling, latency tests under load |
| Lock contention | Phase 1 (Foundation) | Load testing with thread sanitizer, contention profiler |
| Wrong memory order | Phase 1 (Foundation) | Code review of all atomic usage, TSAN on ARM |
| Coroutine lifetime | Phase 5 (High-Level API) | If using coroutines: lifetime analysis, ASAN/TSAN |
| Request ID handling | Phase 2 (Protocol Layer) | Stress tests with many concurrent requests |
| stdio deadlock | Phase 3 (Transport Implementations) | Integration tests with subprocess, large messages |
| Progress/cancellation races | Phase 2 (Protocol Layer) | Concurrent stress tests, race detector |
| JSON library performance | Phase 2 (Protocol Layer) | Benchmarking before committing |
| ABI stability | Phase 5 (High-Level API) | ABI compatibility tests across compilers |
| MCP versioning | Phase 2 (Protocol Layer) | Tests against multiple spec versions |
| Security issues | Phase 4 (Filter Implementations) | Security audit, fuzzing of message parsing |

---

## Sources

- **Async C++ Patterns:**
  - StackOverflow: "An async callback is a fundamentally different beast than a sync callback" - [Is there a way to prevent lambda capture by reference](https://stackoverflow.com/questions/39886531/is-there-a-way-to-prevent-lambda-capture-by-reference)
  - Chromium C++ Style Guide: "Coroutines as safer alternative for async code" - [link](https://github.com/chromium/chromium/blob/main/styleguide/c%2B%2B/c%2B%2B-features.md)
  - PVS-Studio: "C++ programmer's guide to undefined behavior: part 12 of 11" (January 2025) - [link](https://pvs-studio.com/en/blog/posts/cpp/1211/)

- **Lifetime Issues:**
  - "70% of C++ Security Bugs Stem from std::string_view Misuse" (Medium 2024) - [link](https://medium.com/@dikhyantkrishnadalai/70-of-c-security-bugs-stem-from-std-string-view-misuse-heres-how-to-prevent-them-4aca2ba9bb86)
  - "To string_view, or not to string_view" (May 2024) - [link](https://tomhultonharrop.com/posts/string-view/)
  - "Detecting lifetime errors of std::string_view objects in C++" (arXiv 2024) - [link](https://arxiv.org/abs/2408.09325)
  - CppCon/ACCU 2025 talks on coroutine lifetime issues - [link](https://www.youtube.com/watch?v=b6pYieNd_OY)

- **Thread Safety:**
  - StackOverflow: std::shared_ptr thread safety discussions
  - Lei Mao's Blog: "C++ Shared Pointer Thread-Safety" (November 2024) - [link](https://leimao.github.io/blog/CPP-Shared-Ptr-Thread-Safety/)
  - CSDN Blog: "std::shared_ptr thread safety and best practices" (November 2025)
  - StackOverflow: GCC relaxed atomic operation behavior - [link](https://stackoverflow.com/questions/79323385/does-gcc-treat-relaxed-atomic-operation-as-a-compiler-fence)

- **Networking TS and Async:**
  - P3373R0: "Of Operation States and Their Lifetimes" (August 2024) - [link](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3373r0.pdf)
  - Reddit: "Cancellations in Asio: a tale of coroutines and timeouts" - [link](https://www.reddit.com/r/cpp/comments/1laftcm/cancellations_in_asio_a_tale_of_coroutines_and/)
  - Boost.Asio Thread Safety - [link](https://stackoverflow.com/questions/7362894/boostasiosocket-thread-safety)

- **JSON Libraries:**
  - Reddit r/cpp: "What's the go to JSON parser in 2024/2025?" - "If you need performance, avoid nlohmann/json"
  - CppCon 2025: "Reflection-based JSON in C++ at Gigabytes per Second"
  - nlohmann/json GitHub Issue #800: Thread safety of parse() - [link](https://github.com/nlohmann/json/issues/800)

- **ABI Stability:**
  - CSDN Blog: "Solving C++ project compatibility nightmare: cpp-httplib ABI issues" (September 2025)
  - C++ Core Guidelines: Rule I.27 - "For stable library ABI, consider the Pimpl idiom"
  - Reddit: "Is it too late to break ABI?" - [link](https://www.reddit.com/r/cpp/comments/w2zt40/is_it_too_late_to_break_abi/)

- **MCP Specific:**
  - MCP Specification: Official docs and schema
  - GitHub Discussion #603: C++ MCP SDK contribution (October 2025)
  - CVE-2025-6515: MCP Prompt Hijacking Attack (October 2025) - [link](https://jfrog.com/blog/mcp-prompt-hijacking-vulnerability/)
  - Pomerium Blog: "June 2025 MCP Content Round-Up" - security concerns
  - GitHub Discussion #491: Async operations in MCP
  - Dev.to: "Complete MCP JSON-RPC Reference Guide" - [link](https://dev.to/portkey/mcp-message-types-complete-mcp-json-rpc-reference-guide-3gja)

- **Protocol Buffer/Serialization:**
  - Protobuf News: "New RepeatedPtrField Layout" (September 2025) - [link](https://protobuf.dev/news/2025-09-19)
  - StackOverflow: Protocol buffer thread safety - "unless explicitly noted otherwise, it is always safe to use an object from multiple threads as long as they don't share the same object"

---
*Pitfalls research for: C++ MCP Library (mcpp)*
*Researched: 2025-01-31*
