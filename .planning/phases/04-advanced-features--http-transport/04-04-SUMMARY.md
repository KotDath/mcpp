---
phase: 04-advanced-features--http-transport
plan: 04
subsystem: resources
tags: [uri-template, resource-templates, subscriptions, notifications, rfc-6570]

# Dependency graph
requires:
  - phase: 04-01
    provides: UriTemplate for RFC 6570 Level 1-2 template expansion
  - phase: 02-05
    provides: McpServer with transport integration
provides:
  - Resource template registration with URI parameter extraction
  - Resource subscription tracking per URI with subscriber_id
  - Notification dispatch via transport for resource changes
affects: [mcp_server, resource-clients]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Template resource handlers receiving expanded URI and parameters
    - Regex-based template matching for URI-to-template resolution
    - Non-owning transport pointer for notification dispatch
    - Subscription map keyed by URI with vector of subscribers

key-files:
  created: []
  modified: [src/mcpp/server/resource_registry.h, src/mcpp/server/resource_registry.cpp]

key-decisions:
  - "Regex-based template matching instead of UriTemplate::expand for URI-to-template resolution"
  - "Non-owning transport pointer following RequestContext pattern"
  - "Template field in resource list to distinguish templates from static resources"
  - "Subscription map with vector of subscribers per URI"

patterns-established:
  - "Pattern 1: TemplateResourceHandler receives both expanded URI and extracted parameters"
  - "Pattern 2: set_transport() for non-owning transport injection"
  - "Pattern 3: Subscriptions tracked as unordered_map<uri, vector<Subscription>>"

# Metrics
duration: 2min
completed: 2026-01-31
---

# Phase 4: Plan 4 Summary

**Resource templates with URI parameter extraction and subscription-based change notifications**

## Performance

- **Duration:** 2 min
- **Started:** 2026-01-31T19:53:35Z
- **Completed:** 2026-01-31T19:55:54Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Resource templates supporting RFC 6570 Level 1-2 syntax with parameter extraction
- Template handlers receiving both expanded URI and extracted parameters
- Subscription tracking per URI with subscriber_id
- Notification dispatch via transport for resources/updated messages
- Resource list including templates with template field per MCP spec

## Task Commits

Each task was committed atomically:

1. **Task 1: Add resource template support to ResourceRegistry header** - `d1a8601` (feat)
2. **Task 2: Implement subscriptions and notification dispatch** - `eda4677` (feat)

**Plan metadata:** pending (docs: complete plan)

## Files Created/Modified

- `src/mcpp/server/resource_registry.h` - Added TemplateResourceHandler, TemplateResourceRegistration, template storage, subscription methods, transport pointer
- `src/mcpp/server/resource_registry.cpp` - Implemented register_template, subscribe, unsubscribe, notify_updated, set_transport, match_template, build_resource_result

## Decisions Made

- **Regex-based template matching:** Used std::regex to match URIs against templates instead of UriTemplate::expand, which is for building URIs from templates
- **Non-owning transport pointer:** Following RequestContext pattern, transport is non-owning pointer set by McpServer
- **Template field in list_resources:** Added optional "template" field to resource list to distinguish template resources from static ones
- **Subscription tracking:** unordered_map<uri, vector<Subscription>> for efficient subscriber lookup per URI

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- ResourceRegistry now supports templates and subscriptions
- McpServer can call registry.set_transport() to enable notifications
- Next phase should implement resources/subscribe and resources/unsubscribe handlers in McpServer

---
*Phase: 04-advanced-features--http-transport*
*Completed: 2026-01-31*
