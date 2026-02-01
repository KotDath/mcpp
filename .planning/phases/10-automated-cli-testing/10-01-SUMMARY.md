---
phase: 10-automated-cli-testing
plan: 01
subsystem: testing
tags: bats-core, git-submodules, cli-testing, bash

# Dependency graph
requires:
  - phase: 09-inspector-server-integration
    provides: inspector_server binary, MCP stdio transport, mcp.json configuration
provides:
  - BATS testing framework via Git submodules (bats-core, bats-support, bats-assert, bats-file)
  - Common setup helper (_common_setup) for all CLI test files
  - Test environment configuration (PATH for build/examples, MCPP_DEBUG for debug output)
  - Utility functions for test automation (inspector_server_path, wait_for_file, wait_for_content)
affects: 10-02, 10-03, 10-04 (all subsequent CLI testing plans)

# Tech tracking
tech-stack:
  added: bats-core (v1.13.0), bats-support (v0.3.0), bats-assert (v2.2.4), bats-file (v0.2.0)
  patterns: Git submodule-based test framework, common setup function pattern, PATH-based binary discovery

key-files:
  created: .gitmodules, tests/cli/bats, tests/cli/test_helper/bats-support, tests/cli/test_helper/bats-assert, tests/cli/test_helper/bats-file, tests/cli/test_helper/common-setup.sh
  modified: none

key-decisions:
  - "Git submodules over system package installation for reproducibility - ensures pinned versions and works across platforms without package manager differences"
  - "PATH-based binary discovery for inspector_server - tests can run inspector_server without absolute paths or project-specific knowledge"
  - "MCPP_DEBUG=1 default for all tests - provides visibility into server behavior during test execution"

patterns-established:
  - "Pattern 1: Common _common_setup() function loaded by all test files via BATS setup()"
  - "Pattern 2: PROJECT_ROOT derived from BATS_TEST_FILENAME for location-independent test execution"
  - "Pattern 3: Helper functions in common-setup.sh for common test operations (wait_for_file, wait_for_content)"

# Metrics
duration: 2min
completed: 2026-02-01
---

# Phase 10 Plan 01: BATS Testing Infrastructure Summary

**BATS testing framework with Git submodules (bats-core, bats-support, bats-assert, bats-file) and common setup helper for CLI test automation**

## Performance

- **Duration:** 2 min
- **Started:** 2026-02-01T12:18:27Z
- **Completed:** 2026-02-01T12:20:18Z
- **Tasks:** 1
- **Files modified:** 6

## Accomplishments

- BATS testing framework installed as Git submodules (no system dependencies)
- Helper libraries configured (bats-support, bats-assert, bats-file)
- Common setup helper created with _common_setup() function
- Test environment configuration (PATH for build/examples, MCPP_DEBUG=1)
- Utility functions for test automation (inspector_server_path, wait_for_file, wait_for_content)

## Task Commits

Each task was committed atomically:

1. **Task 1: Set up bats-core Git submodules and directory structure** - `f51b193` (feat)

**Plan metadata:** [pending]

_Note: TDD tasks may have multiple commits (test -> feat -> refactor)_

## Files Created/Modified

- `.gitmodules` - Git submodule configuration for 4 BATS repositories
- `tests/cli/bats/` - BATS core testing framework (v1.13.0-10-g5f12b31)
- `tests/cli/test_helper/bats-support/` - BATS helper library for output formatting (v0.3.0-37-g0954abb)
- `tests/cli/test_helper/bats-assert/` - BATS assertion library (v2.2.4-2-g697471b)
- `tests/cli/test_helper/bats-file/` - BATS filesystem assertion library (v0.2.0-129-g6bee58b)
- `tests/cli/test_helper/common-setup.sh` - Common setup function and helper utilities for all test files

## Decisions Made

- Used Git submodules instead of system package installation (apt, brew) for bats-core and helper libraries - ensures reproducible builds and pinned versions across all platforms
- Exported MCPP_DEBUG=1 by default for all tests - provides server debug output during test execution for easier debugging
- PATH-based binary discovery for inspector_server - tests can invoke `inspector_server` directly without absolute paths
- Created utility functions in common-setup.sh for common test patterns (wait_for_file, wait_for_content)

## Deviations from Plan

None - plan executed exactly as written.

## Authentication Gates

None - no external authentication required for this plan.

## Issues Encountered

None - all Git submodules cloned successfully without errors.

## Next Phase Readiness

- BATS testing infrastructure ready for test file creation
- Next plan (10-02) can create test files using common-setup.sh
- All 4 submodules properly initialized and ready for use
- `tests/cli/bats/bin/bats` executable available for running tests

---
*Phase: 10-automated-cli-testing*
*Completed: 2026-02-01*
