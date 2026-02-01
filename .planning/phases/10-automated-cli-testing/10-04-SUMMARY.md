---
phase: 10-automated-cli-testing
plan: 04
subsystem: testing
tags: bats, cmake, ctest, lifecycle, process-management

# Dependency graph
requires:
  - phase: 10-automated-cli-testing
    plan: 10-02
    provides: Basic CLI test infrastructure with common-setup.bash
  - phase: 10-automated-cli-testing
    plan: 10-03
    provides: Protocol coverage tests for tools, resources, prompts
provides:
  - Server lifecycle tests verifying startup, shutdown, and cleanup
  - CMake/CTest integration for CLI tests via find_program(BATS_PROGRAM)
  - Individual test targets (cli-initialize, cli-tools-list, etc.)
  - Guaranteed process cleanup via BATS teardown() function
affects: []

# Tech tracking
tech-stack:
  added: []
  patterns:
    - BATS teardown() for guaranteed cleanup (instead of trap EXIT)
    - RUN_SERIAL property for sequential test execution
    - Conditional test registration based on tool availability

key-files:
  created:
    - tests/cli/06-lifecycle.bats
  modified:
    - tests/CMakeLists.txt

key-decisions:
  - "Use BATS teardown() instead of trap EXIT for cleanup (trap conflicts with bats internal test gathering)"
  - "CLI tests skipped non-fatally if bats or jq not available (find_program conditional)"
  - "RUN_SERIAL TRUE prevents parallel execution conflicts between server instances"

patterns-established:
  - "BATS lifecycle: setup() -> @test -> teardown() runs even on test failure"
  - "CMake conditional test registration: if(BATS_PROGRAM AND JQ_PROGRAM)"
  - "Process cleanup: kill with wait loop, use < /dev/null for background server stdin"

# Metrics
duration: 35min
completed: 2026-02-01
---

# Phase 10 Plan 04: Server Lifecycle Tests and CMake Integration Summary

**Server lifecycle tests with BATS teardown() for guaranteed cleanup and CMake/CTest integration for unified test execution**

## Performance

- **Duration:** 35 min
- **Started:** 2026-02-01T12:31:10Z
- **Completed:** 2026-02-01T13:06:00Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Created 5 server lifecycle tests covering startup, shutdown, and process cleanup
- Integrated BATS CLI tests with CMake/CTest build system
- All 45 CLI tests passing (40 existing + 5 new lifecycle tests)
- No orphaned inspector_server processes after test execution

## Task Commits

Each task was committed atomically:

1. **Task 1: Create server lifecycle tests** - `f808584` (feat)
2. **Task 2: Add CMake integration for CLI tests** - `b3f9d61` (feat)
3. **Bug fix: Use teardown instead of trap** - `0bd5823` (fix)

**Plan metadata:** None (docs committed with task commits)

_Note: Task 1 required bug fix due to trap EXIT conflict with BATS internals_

## Files Created/Modified

- `tests/cli/06-lifecycle.bats` - Server lifecycle tests (5 tests)
- `tests/CMakeLists.txt` - CMake integration for CLI tests

## Decisions Made

- **Use BATS teardown() instead of trap EXIT**: The plan specified using `trap "kill \$SERVER_PID 2>/dev/null || true" EXIT` but this prevents BATS from gathering subsequent tests. Changed to use BATS' built-in `teardown()` function which runs after each test (even on failure).
- **Conditional test registration**: CLI tests only registered if both `bats` and `jq` are found via `find_program()`. Not fatal if missing - CLI tests simply skipped.
- **RUN_SERIAL TRUE**: All CLI test targets set with RUN_SERIAL property to prevent parallel execution conflicts (port binding, process interference).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed trap EXIT preventing test gathering**
- **Found during:** Task 1 verification (only 1 of 5 tests running)
- **Issue:** Using `trap EXIT` in helper functions conflicts with BATS' internal test gathering mechanism, causing subsequent tests to be skipped
- **Fix:** Changed from `trap "kill \$SERVER_PID 2>/dev/null || true" EXIT` to BATS `teardown()` function which runs after each test (even on failure)
- **Files modified:** tests/cli/06-lifecycle.bats
- **Verification:** All 5 tests now execute and pass; 45 total CLI tests passing
- **Committed in:** `0bd5823`

**2. [Rule 1 - Bug] Fixed regex escape in grep command**
- **Found during:** Task 1 (bash -n syntax check)
- **Issue:** Double-escaped backslash `\\|` in grep -i regex caused syntax error
- **Fix:** Changed to `grep -iE 'error|fatal|failed'` using -E flag for extended regex
- **Files modified:** tests/cli/06-lifecycle.bats
- **Verification:** Syntax check passes, grep works correctly in tests
- **Committed in:** `0bd5823`

**3. [Rule 2 - Missing Critical] Added stdin handling for background server**
- **Found during:** Task 1 (server exits immediately without stdin)
- **Issue:** Background server process exits immediately when stdin closes, making process verification fail
- **Fix:** Added `< /dev/null` to `inspector_server < /dev/null > ...` to keep background process alive for lifecycle testing
- **Files modified:** tests/cli/06-lifecycle.bats
- **Verification:** `server_is_running` checks work, background process stays alive until killed
- **Committed in:** `0bd5823`

---

**Total deviations:** 3 auto-fixed (3 bugs)
**Impact on plan:** All auto-fixes necessary for correct operation. Changed cleanup mechanism from trap to teardown (architectural adjustment within test file only, no scope creep).

## Issues Encountered

- **BATS trap EXIT conflict**: Using `trap EXIT` in helper functions prevented BATS from gathering tests beyond the first. Resolved by using BATS `teardown()` function instead.
- **Server exit on closed stdin**: Background server exited immediately when started without stdin source. Resolved by redirecting `/dev/null` to stdin for background lifecycle tests.

## User Setup Required

None - no external service configuration required.

Note: For CMake/CTest to discover CLI tests, `bats` must be in PATH. The Git submodule at `tests/cli/bats/bin/bats` can be used if system bats is not available:

```bash
export PATH="$PWD/tests/cli/bats/bin:$PATH"
cmake --build build
ctest -R cli
```

## Next Phase Readiness

- Server lifecycle testing complete with 5 passing tests
- CMake/CTest integration enables `make test` or `ctest` to run all tests (unit, integration, compliance, CLI)
- No orphaned processes after test execution
- Phase 10 remaining: 10-04 (this plan) completes all scheduled Phase 10 work

---
*Phase: 10-automated-cli-testing*
*Completed: 2026-02-01*
