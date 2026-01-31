---
phase: 03-client-capabilities
plan: 06
subsystem: client
tags: [std::future, std::promise, blocking-api, callback-wrapper, timeout]

# Dependency graph
requires:
  - phase: 03-client-capabilities
    provides: [McpClient callback API, RootsManager, SamplingClient, ElicitationClient, CancellationManager]
provides:
  - FutureBuilder template for converting callback-style async to std::future
  - McpClientBlocking wrapper class for ergonomic synchronous API calls
  - Timeout support for all blocking operations (prevents infinite blocking)
  - McpClientBlockingError typed exception with error codes
affects: []

# Tech tracking
tech-stack:
  added: [std::future, std::promise, std::chrono for timeouts]
  patterns: [shared_ptr<promise> for lambda lifetime management, callback-to-future wrapping pattern]

key-files:
  created: [src/mcpp/client/future_wrapper.h, src/mcpp/client/future_wrapper.cpp, src/mcpp/client_blocking.h]
  modified: [src/mcpp/client/roots.h, src/mcpp/client/roots.cpp]

key-decisions:
  - "shared_ptr<promise> pattern ensures promise lifetime until lambda callback invoked"
  - "Exception callbacks store runtime_error with JsonRpcError message text"
  - "Default 30-second timeout prevents infinite blocking on unresponsive servers"
  - "McpClientBlocking holds reference (not ownership) to McpClient"
  - "Added ListRootsResult::from_json() to parse JSON responses in blocking wrapper"

patterns-established:
  - "Promise/future wrapping: create shared_ptr<promise>, capture in lambda callbacks, return future.get()"
  - "Blocking wrappers: wrap(callback invocation) -> return with_timeout(future, duration)"
  - "Error propagation: catch exceptions in callbacks, store as promise exception, rethrow as typed error"

# Metrics
duration: 2min
completed: 2026-01-31
---

# Phase 3 Plan 6: std::future Wrappers Summary

**FutureBuilder template with shared_ptr<promise> lifetime management and McpClientBlocking wrapper for synchronous initialize/roots/request calls with timeout protection**

## Performance

- **Duration:** 2 min
- **Started:** 2026-01-31T18:37:09Z
- **Completed:** 2026-01-31T18:39:01Z
- **Tasks:** 4
- **Files modified:** 5

## Accomplishments

- FutureBuilder template class converts callback-style async APIs to std::future-based blocking calls
- McpClientBlocking provides ergonomic synchronous API wrapping all McpClient operations
- Timeout support via with_timeout() prevents infinite blocking (30-second default)
- Added ListRootsResult::from_json() for JSON parsing in blocking context

## Task Commits

Each task was committed atomically:

1. **Task 1: Create FutureBuilder template** - `68d3d92` (feat)
2. **Task 2: Create FutureBuilder implementation** - `8341c7b` (feat)
3. **Task 3: Create McpClientBlocking class** - `fd88b28` (feat)
4. **Task 4: Add timeout and error handling** - `fd88b28` (integrated in Task 3)

## Files Created/Modified

- `src/mcpp/client/future_wrapper.h` - FutureBuilder template with create(), wrap(), with_timeout() methods (174 lines)
- `src/mcpp/client/future_wrapper.cpp` - Explicit instantiation for nlohmann::json (35 lines)
- `src/mcpp/client_blocking.h` - McpClientBlocking class with blocking initialize/list_roots/send_request (322 lines)
- `src/mcpp/client/roots.h` - Added ListRootsResult::from_json() declaration
- `src/mcpp/client/roots.cpp` - Added ListRootsResult::from_json() implementation

## Key API

### FutureBuilder Usage

```cpp
// Convert callback-style to future-based
auto future = FutureBuilder<InitializeResult>::wrap(
    [&](auto on_success, auto on_error) {
        client.initialize(params, on_success, on_error);
    }
);
auto result = future.get();  // Blocks until complete
```

### McpClientBlocking Usage

```cpp
McpClient client(std::make_unique<StdioTransport>());
client.connect();

McpClientBlocking blocking(client);
auto result = blocking.initialize(params);  // Blocks until complete
auto roots = blocking.list_roots();
auto response = blocking.send_request("tools/list", nullptr);
```

## Decisions Made

- **shared_ptr<promise> pattern**: Ensures promise lives until lambda callback is invoked, critical for async callback lifetime
- **Exception conversion**: JsonRpcError.message converted to std::runtime_error for future exception storage
- **Default timeout**: 30 seconds prevents indefinite hangs on unresponsive servers
- **Reference semantics**: McpClientBlocking holds reference to McpClient (not ownership) - user manages lifecycle
- **Typed exceptions**: McpClientBlockingError provides error codes from JSON-RPC errors

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Added ListRootsResult::from_json() method**
- **Found during:** Task 3 (Create McpClientBlocking class)
- **Issue:** Plan didn't specify from_json() method, but list_roots() needs to parse JSON response to ListRootsResult
- **Fix:** Added from_json() static method to ListRootsResult for parsing roots/list responses
- **Files modified:** src/mcpp/client/roots.h, src/mcpp/client/roots.cpp
- **Verification:** Parses JSON with roots array into ListRootsResult struct
- **Committed in:** fd88b28 (part of Task 3 commit)

---

**Total deviations:** 1 auto-fixed (1 missing critical)
**Impact on plan:** Addition necessary for correct operation of list_roots(). No scope creep.

## Issues Encountered

None - all tasks executed as planned.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

**Phase 3 (Client Capabilities) complete.** All 6 plans delivered:
- 03-01: Cancellation support
- 03-02: Roots management
- 03-03: Basic sampling
- 03-04: Sampling with tool use
- 03-05: Elicitation support
- 03-06: std::future wrappers

**Ready for Phase 4 (Advanced Features & HTTP Transport):**
- Resource and tool completion (RT-04, RT-05)
- HTTP transport with SSE/WebSocket support
- Progress token handling enhancements
- Advanced error recovery and retry logic

**No blockers.** Client-side capabilities are complete and fully functional.

---
*Phase: 03-client-capabilities*
*Completed: 2026-01-31*
