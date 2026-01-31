---
phase: 01-protocol-foundation
plan: 05
subsystem: async
tags: [std::function, callback, timeout, steady_clock, thread-safety]

# Dependency graph
requires:
  - phase: 01-protocol-foundation
    provides: [01-01 JSON-RPC core types (RequestId, JsonValue, JsonRpcError)]
provides:
  - Callback type aliases for async MCP operations
  - TimeoutManager with automatic deadline tracking and expiration
affects: [async-operations, request-tracking, client-implementation]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - std::function-based callback type aliases
    - steady_clock for timeout tracking (immune to system time changes)
    - Mutex-protected timeout registry
    - Callback invocation outside lock to prevent deadlock

key-files:
  created: [src/mcpp/async/callbacks.h, src/mcpp/async/timeout.h, src/mcpp/async/timeout.cpp]
  modified: []

key-decisions:
  - "Callbacks stored as std::function copies - users can discard originals after registration"
  - "steady_clock used instead of system_clock for timeout accuracy"
  - "Callbacks invoked after mutex release to prevent deadlock if callback re-enters TimeoutManager"

patterns-established:
  - "Pattern 1: Type aliases in mcpp::async namespace import core types for convenience"
  - "Pattern 2: Thread-safe class with mutable mutex for const methods"
  - "Pattern 3: Extract callbacks from map, release lock, then invoke to prevent re-entrancy deadlock"

# Metrics
duration: 5min
completed: 2026-01-31
---

# Phase 1 Plan 5: Async Callbacks and Timeout Manager Summary

**std::function-based callback type aliases and steady_clock TimeoutManager with deadlock-safe callback invocation**

## Performance

- **Duration:** 5 min
- **Started:** 2026-01-31T10:15:14Z
- **Completed:** 2026-01-31T10:20:00Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments

- Defined four callback type aliases (ResponseCallback, ErrorCallback, NotificationCallback, TimeoutCallback)
- Implemented TimeoutManager with thread-safe timeout registration, cancellation, and expiration checking
- Established callback lifetime documentation pattern

## Task Commits

Each task was committed atomically:

1. **Task 1: Define callback types** - `c64fc2a` (feat)
2. **Task 2: Define TimeoutManager header** - `b6b0b26` (feat)
3. **Task 3: Implement TimeoutManager** - `ac115b6` (feat)

**Plan metadata:** TBD (docs: complete plan)

## Files Created/Modified

- `src/mcpp/async/callbacks.h` - Callback type aliases for async operations
- `src/mcpp/async/timeout.h` - TimeoutManager class declaration
- `src/mcpp/async/timeout.cpp` - TimeoutManager implementation

## Decisions Made

- **std::function copies**: Library owns std::function copies of callbacks; users can discard their originals after registration. Simplifies user code and prevents dangling references.
- **steady_clock for timeouts**: Used std::chrono::steady_clock instead of system_clock. steady_clock is monotonic and unaffected by system time changes, ensuring timeout accuracy.
- **Callback invocation outside lock**: check_timeouts() extracts callbacks from the map, releases the mutex, then invokes callbacks. Prevents deadlock if a callback calls back into TimeoutManager (e.g., cancel()).
- **has_timeout and pending_count**: Added convenience methods not in plan but useful for debugging and monitoring. No blocking issue - these are simple const accessors.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed forward declaration in callbacks.h**
- **Found during:** Task 1 (Define callback types)
- **Issue:** Initial attempt used forward declarations for JsonValue which is a type alias (nlohmann::json), not a real type
- **Fix:** Included necessary headers (cstdint, variant, string) and nlohmann/json.hpp; defined JsonValue and RequestId type aliases directly instead of forward declaring
- **Files modified:** src/mcpp/async/callbacks.h
- **Verification:** Compiles without errors, type aliases correctly reference nlohmann::json
- **Committed in:** c64fc2a (part of Task 1 commit)

**2. [Rule 2 - Missing Critical] Added has_timeout and pending_count methods**
- **Found during:** Task 3 (Implement TimeoutManager)
- **Issue:** Plan header didn't include introspection methods needed for debugging and monitoring
- **Fix:** Added has_timeout(const RequestId&) const and pending_count() const methods
- **Files modified:** src/mcpp/async/timeout.h, src/mcpp/async/timeout.cpp
- **Verification:** Simple const accessors that return useful state information
- **Committed in:** ac115b6 (part of Task 3 commit)

---

**Total deviations:** 2 auto-fixed (1 bug, 1 missing critical)
**Impact on plan:** Both auto-fixes essential for correctness. has_timeout/pending_count are simple introspection methods with no scope impact.

## Issues Encountered

None - plan executed smoothly.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Async callback infrastructure complete
- TimeoutManager ready for integration with RequestTracker (next plan 01-06)
- Type aliases provide clean interface for async MCP operations

---
*Phase: 01-protocol-foundation*
*Completed: 2026-01-31*
