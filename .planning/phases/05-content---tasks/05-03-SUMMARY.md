---
phase: 05-content---tasks
plan: 03
subsystem: server
tags: [notifications, callbacks, registries, client-capabilities]

# Dependency graph
requires:
  - phase: 02-core-server
    provides: ToolRegistry, ResourceRegistry, PromptRegistry, McpServer
provides:
  - List_changed notification support for all three registries (tools, resources, prompts)
  - Client capabilities storage and notification routing via McpServer
affects: [06-testing]

# Tech tracking
tech-stack:
  added: []
  patterns: [registry notification callbacks, client capability checks, mutual opt-in notifications]

key-files:
  created: []
  modified:
    - src/mcpp/server/tool_registry.h
    - src/mcpp/server/tool_registry.cpp
    - src/mcpp/server/resource_registry.h
    - src/mcpp/server/resource_registry.cpp
    - src/mcpp/server/prompt_registry.h
    - src/mcpp/server/prompt_registry.cpp
    - src/mcpp/server/mcp_server.h
    - src/mcpp/server/mcp_server.cpp

key-decisions:
  - "NotifyCallback pattern following RootsManager implementation (std::function<void()>)"
  - "Client capabilities stored from initialize for notification gating"
  - "Registry callbacks check experimental capabilities for listChanged before sending"
  - "Transport availability checked before sending notifications"

patterns-established:
  - "Registry notification pattern: set_notify_callback(), notify_changed(), private notify_cb_"
  - "Capability-gated notifications: only send when client advertises support"
  - "Automatic notification on successful registration via notify_changed() call"

# Metrics
duration: 3min
completed: 2026-01-31
---

# Phase 5 Plan 3: List Changed Notifications Summary

**Registry notification support with callback pattern following RootsManager, client capability storage, and capability-gated notification routing**

## Performance

- **Duration:** 3 min (190 seconds)
- **Started:** 2026-01-31T20:46:39Z
- **Completed:** 2026-01-31T20:49:49Z
- **Tasks:** 4
- **Files modified:** 8

## Accomplishments

- All three registries (ToolRegistry, ResourceRegistry, PromptRegistry) support list_changed notifications via callback pattern
- McpServer stores client capabilities from initialize and wires registry callbacks to transport
- Notifications only sent when client capability is enabled (mutual opt-in via experimental.listChanged)
- Pattern consistency with RootsManager for maintainability

## Task Commits

Each task was committed atomically:

1. **Task 1: Add notification support to ToolRegistry** - `ae1e093` (feat)
2. **Task 2: Add notification support to ResourceRegistry** - `3c72907` (feat)
3. **Task 3: Add notification support to PromptRegistry** - `3df5b84` (feat)
4. **Task 4: Add client capabilities storage and callback setup to McpServer** - `58d3202` (feat)

## Files Created/Modified

### Modified

- `src/mcpp/server/tool_registry.h` - Added NotifyCallback typedef, set_notify_callback(), notify_changed(), private notify_cb_
- `src/mcpp/server/tool_registry.cpp` - Implemented callback methods, call notify_changed() after registration
- `src/mcpp/server/resource_registry.h` - Added NotifyCallback typedef, set_notify_callback(), notify_changed(), private notify_cb_
- `src/mcpp/server/resource_registry.cpp` - Implemented callback methods, call notify_changed() after registration
- `src/mcpp/server/prompt_registry.h` - Added NotifyCallback typedef, set_notify_callback(), notify_changed(), private notify_cb_
- `src/mcpp/server/prompt_registry.cpp` - Implemented callback methods, call notify_changed() after registration
- `src/mcpp/server/mcp_server.h` - Added capabilities include, client_capabilities_ member, setup_registry_callbacks(), send_list_changed_notification()
- `src/mcpp/server/mcp_server.cpp` - Store client caps in initialize, setup callbacks, send notifications with capability check

## Decisions Made

- **NotifyCallback pattern:** Following RootsManager implementation using `std::function<void()>` for consistency across the codebase
- **Client capabilities storage:** Store in McpServer as `std::optional<protocol::ClientCapabilities>` to support capability-gated notifications
- **Capability check location:** Check capabilities in registry callbacks (set during initialize) before sending notifications
- **Experimental capabilities:** listChanged is nested under experimental for tools/resources/prompts per MCP spec

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - all tasks completed as specified without blocking issues.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Registry notification infrastructure complete
- Client capability detection and notification routing implemented
- Ready for additional server features or client notification handling

---
*Phase: 05-content---tasks*
*Plan: 03*
*Completed: 2026-01-31*
