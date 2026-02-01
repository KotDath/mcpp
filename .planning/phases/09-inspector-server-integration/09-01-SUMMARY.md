---
phase: 09-inspector-server-integration
plan: 01
subsystem: json-rpc
tags: [json-rpc, request-id, error-handling, malformed-json]

# Dependency graph
requires:
  - phase: 08-bug-fix-foundation
    provides: JsonRpcRequest::from_json() validation with ParseError diagnostics
provides:
  - JsonRpcRequest::extract_request_id() static helper method for extracting IDs from malformed JSON
  - Best-effort ID extraction supporting string, numeric, and null ID formats
  - Foundation for constructing parse error responses per JSON-RPC 2.0 spec
affects: [09-02-inspector-server-error-handling, examples/inspector_server.cpp]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "String search instead of JSON parsing for malformed input recovery"
    - "Null ID fallback pattern for error responses"

key-files:
  created: []
  modified:
    - src/mcpp/core/json_rpc.h
    - src/mcpp/core/json_rpc.cpp

key-decisions:
  - "String search approach for ID extraction instead of JSON parsing (works on malformed input)"
  - "Null ID (int64_t=0) as universal fallback for any extraction failure"

patterns-established:
  - "Best-effort parsing: Attempt to extract useful data even from malformed input"
  - "Static helper pattern for utilities that don't require instance state"

# Metrics
duration: 2min
completed: 2026-02-01
---

# Phase 9 Plan 1: Extract Request ID Helper Summary

**Best-effort request ID extraction from malformed JSON using string search, supporting string/numeric/null ID formats per JSON-RPC 2.0 spec**

## Performance

- **Duration:** 2 min (124 seconds)
- **Started:** 2026-02-01T09:17:45Z
- **Completed:** 2026-02-01T09:19:49Z
- **Tasks:** 3
- **Files modified:** 2

## Accomplishments

- Added `JsonRpcRequest::extract_request_id()` static method declaration with complete documentation
- Implemented best-effort ID extraction using string search (works even when JSON is malformed)
- Supports all three JSON-RPC 2.0 ID formats: string ("id": "req-123"), numeric ("id": 42), and null ("id": null)
- Returns null RequestId (int64_t=0) for any failure case (missing, malformed, or unhandled types)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add extract_request_id() declaration to JsonRpcRequest** - `e7797b0` (feat)
2. **Task 2: Implement extract_request_id() in json_rpc.cpp** - `b2c1157` (feat)
3. **Task 3: Build and run tests to verify implementation** - No new code changes

**Plan metadata:** (to be committed after this file)

## Files Created/Modified

- `src/mcpp/core/json_rpc.h` - Added `extract_request_id()` static method declaration
- `src/mcpp/core/json_rpc.cpp` - Added `extract_request_id()` implementation with string-based parsing

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - build succeeded on first attempt, all 121 unit tests passing.

## Authentication Gates

None encountered.

## Next Phase Readiness

- `extract_request_id()` helper is now available for use in `examples/inspector_server.cpp`
- Ready for Plan 09-02: Integrate extract_request_id() into inspector server error handling
- No blockers or concerns

---
*Phase: 09-inspector-server-integration*
*Completed: 2026-02-01*
