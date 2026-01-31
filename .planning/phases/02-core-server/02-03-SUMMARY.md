---
phase: 02-core-server
plan: 03
subsystem: server
tags: [mcp, prompts, handler-registration, argument-substitution]

# Dependency graph
requires:
  - phase: 01-protocol-foundation
    provides: nlohmann/json, JSON-RPC core types, MCP protocol types
provides:
  - PromptRegistry with registration, discovery, and retrieval
  - PromptMessage, PromptArgument, PromptRegistration types
  - PromptHandler function type for prompt message generation
  - MCP-compliant prompts/list and prompts/get response formats
affects: [02-04-resources, 02-05-tools, mcp-server-integration]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Handler registration pattern (name + metadata + callable)
    - unordered_map for O(1) name-based lookup
    - std::function for handler storage
    - Optional fields with std::optional
    - MCP response format compliance

key-files:
  created:
    - src/mcpp/server/prompt_registry.h
    - src/mcpp/server/prompt_registry.cpp
  modified: []

key-decisions:
  - "Handler receives raw arguments JSON for flexible template substitution"
  - "PromptRegistry returns MCP GetPromptResult format directly from get_prompt()"
  - "Argument substitution is handler's responsibility (not enforced by registry)"

patterns-established:
  - "Pattern: Handler registration - similar pattern will be used for tools and resources"
  - "Pattern: Discovery via list_* methods returning JSON arrays with metadata"
  - "Pattern: Retrieval via get_* methods calling handlers and formatting results"

# Metrics
duration: 1min
completed: 2026-01-31
---

# Phase 2 Plan 3: Prompt Registry Summary

**Prompt registry with handler registration pattern supporting argument templating and MCP-compliant prompts/list and prompts/get responses**

## Performance

- **Duration:** 1 min (70 seconds)
- **Started:** 2026-01-31T10:59:27Z
- **Completed:** 2026-01-31T11:00:40Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- **Prompt registration with name-based lookup**: Library consumers can register prompts with name, description, argument definitions, and handler function
- **Prompt discovery via list_prompts()**: Returns MCP-compliant JSON array with name, optional description, and argument definitions
- **Prompt retrieval via get_prompt()**: Calls handler with arguments and returns GetPromptResult format with messages array
- **Handler-based argument substitution**: Handlers receive raw arguments JSON and perform template substitution

## Task Commits

Each task was committed atomically:

1. **Task 1: Create prompt types and handler signatures** - `604097c` (feat)
2. **Task 2: Create PromptRegistry class with registration and retrieval** - `658abc3` (feat)

**Plan metadata:** TBD (docs: complete plan)

_Note: TDD tasks may have multiple commits (test → feat → refactor)_

## Files Created/Modified

- `src/mcpp/server/prompt_registry.h` - Prompt message types, argument definitions, handler signature, registry class declaration
- `src/mcpp/server/prompt_registry.cpp` - PromptRegistry implementation (register, list, get, has_prompt)

## Decisions Made

- **Handler signature receives raw JSON**: PromptHandler receives `const nlohmann::json& arguments` directly rather than parsed types, giving handlers maximum flexibility for argument substitution
- **Registry doesn't validate arguments**: The registry doesn't enforce argument validation against declared arguments - this is the handler's responsibility, allowing for dynamic argument handling
- **get_prompt returns MCP format**: Directly returns GetPromptResult JSON structure (`{"messages": [...]}`) rather than internal types, simplifying integration with JSON-RPC layer

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

**Ready:**
- PromptRegistry provides complete registration, discovery, and retrieval interface
- Handler registration pattern established for use with tools and resources
- MCP response format compliance verified

**For consideration:**
- Similar pattern will be used for tool_registry and resource_registry
- Consider adding argument validation helper for common patterns (optional enhancement)

---
*Phase: 02-core-server*
*Completed: 2026-01-31*
