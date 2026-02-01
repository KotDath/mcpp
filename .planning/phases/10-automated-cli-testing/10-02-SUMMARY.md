---
phase: 10-automated-cli-testing
plan: 02
subsystem: testing
tags: [bats, bats-core, json-rpc, mcp-protocol, jq, cli-testing]

# Dependency graph
requires:
  - phase: 10-automated-cli-testing
    plan: 01
    provides: BATS testing infrastructure, common-setup helper, PATH configuration
provides:
  - MCP initialize handshake protocol tests
  - MCP tools/list endpoint tests
  - JSON-RPC request/response testing pattern using bats-core and jq
affects: [10-03-protocol-coverage-tests, 10-04-test-stabilization]

# Tech tracking
tech-stack:
  added: []
  patterns: [JSON-RPC CLI testing with bats-core + jq, MCP protocol validation pattern]

key-files:
  created: [tests/cli/05-initialize.bats, tests/cli/01-tools-list.bats]
  modified: [tests/cli/test_helper/common-setup.bash]

key-decisions:
  - "Renamed common-setup.sh to common-setup.bash for bats-core compatibility"
  - "Fixed project root calculation (../.. instead of ../../../)"
  - "Used inline JSON requests instead of helper functions to avoid subshell issues"

patterns-established:
  - "Pattern: Send JSON-RPC request via echo to inspector_server, parse with jq"
  - "Pattern: Use run bash -c for complex command pipelines with jq"
  - "Pattern: MCP protocol requires initialize before tools/list"

# Metrics
duration: 4min
completed: 2026-02-01
---

# Phase 10 Plan 02: Inspector Server Basic Tests Summary

**BATS-based MCP protocol tests for initialize handshake and tools/list endpoint using JSON-RPC validation with jq**

## Performance

- **Duration:** 4 min (225 seconds)
- **Started:** 2026-02-01T12:22:14Z
- **Completed:** 2026-02-01T12:25:59Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Created 4 tests validating MCP initialize handshake (protocol version, capabilities, server info)
- Created 4 tests validating tools/list endpoint (expected tools, schema validation, graceful failure)
- Established JSON-RPC request/response testing pattern using bats-core and jq

## Task Commits

Each task was committed atomically:

1. **Task 1: Create 05-initialize.bats for initialize handshake tests** - `fec67cf` (feat)
2. **Task 2: Create 01-tools-list.bats for tools/list endpoint** - `3fd0aa7` (feat)

## Files Created/Modified

- `tests/cli/05-initialize.bats` - 4 tests for MCP initialize handshake (JSON-RPC 2.0, protocol version 2025-11-25, server capabilities, server info)
- `tests/cli/01-tools-list.bats` - 4 tests for tools/list endpoint (response structure, expected tools, schema validation, pre-initialize behavior)
- `tests/cli/test_helper/common-setup.bash` - Renamed from .sh, fixed project root path calculation

## Decisions Made

- Renamed `common-setup.sh` to `common-setup.bash` because bats-core's `load` builtin expects `.bash` extension
- Fixed project root calculation from `../../../` to `../..` since BATS_TEST_DIRNAME is already at tests/cli/
- Used inline JSON requests in tests instead of helper functions to avoid subshell export issues

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed bats-core helper loading**
- **Found during:** Task 1 (05-initialize.bats creation)
- **Issue:** common-setup.sh couldn't be loaded by bats-core's `load` builtin (expects .bash extension)
- **Fix:** Renamed common-setup.sh to common-setup.bash
- **Files modified:** tests/cli/test_helper/common-setup.bash
- **Verification:** `bats -c` successfully counts 4 tests
- **Committed in:** `fec67cf` (part of Task 1 commit)

**2. [Rule 3 - Blocking] Fixed project root path calculation**
- **Found during:** Task 1 (running tests)
- **Issue:** PROJECT_ROOT was calculated as `/home/kotdath/omp/personal/` instead of `/home/kotdath/omp/personal/cpp/mcpp`
- **Fix:** Changed path from `BATS_TEST_DIRNAME/../../../` to `BATS_TEST_DIRNAME/../..`
- **Files modified:** tests/cli/test_helper/common-setup.bash
- **Verification:** inspector_server found in PATH, all tests pass
- **Committed in:** `fec67cf` (part of Task 1 commit)

**3. [Rule 1 - Bug] Fixed subshell function export issue**
- **Found during:** Task 1 (05-initialize.bats execution)
- **Issue:** Helper function `send_initialize` defined in test file wasn't available in `run bash -c` subshells (exit code 127)
- **Fix:** Used inline JSON requests with bash -c instead of calling helper functions
- **Files modified:** tests/cli/05-initialize.bats
- **Verification:** All 4 tests pass
- **Committed in:** `fec67cf` (part of Task 1 commit)

---

**Total deviations:** 3 auto-fixed (2 blocking, 1 bug)
**Impact on plan:** All auto-fixes were necessary for tests to run. The pattern changes establish better practices for BATS testing.

## Issues Encountered

None - all issues were resolved via deviation rules (Rule 1 and Rule 3).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- MCP protocol testing pattern established (initialize + tools/list)
- Ready for 10-03 (Protocol Coverage Tests) to add tests for tools/call, resources/list, prompts/list
- JSON-RPC validation pattern with jq is working for response structure checking

---
*Phase: 10-automated-cli-testing*
*Completed: 2026-02-01*
