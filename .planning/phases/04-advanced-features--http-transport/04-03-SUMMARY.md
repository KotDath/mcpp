---
phase: 04-advanced-features--http-transport
plan: 03
subsystem: tool-registry
tags: [tool-annotations, output-schema, json-schema-validation, mcp-tools]

# Dependency graph
requires:
  - phase: 02-core-server
    provides: ToolRegistry with input_schema validation
provides:
  - ToolAnnotations struct (destructive, read_only, audience, priority)
  - Output schema validation for tool results
  - Extended tool discovery with annotations and outputSchema
affects: [client-integrations, tool-ui-displays, validation-framework]

# Tech tracking
tech-stack:
  added: []
  patterns: [overloaded-method-backward-compat, schema-validator-reuse, annotations-for-discovery]

key-files:
  created: []
  modified: [src/mcpp/server/tool_registry.h, src/mcpp/server/tool_registry.cpp]

key-decisions:
  - "Backward compatible register_tool overload (original calls new with defaults)"
  - "Output validation returns CallToolResult with isError=true (not JSON-RPC error)"
  - "Annotations use struct with constructor for easy initialization"
  - "Output schema compilation follows same pattern as input schema"

patterns-established:
  - "Pattern: Original method delegates to overloaded implementation for backward compatibility"
  - "Pattern: Tool errors vs protocol errors distinguished by response format"
  - "Pattern: Schema validators compiled once at registration for performance"

# Metrics
duration: 2min
completed: 2026-01-31
---

# Phase 04 Plan 03: Tool Metadata and Output Validation Summary

**Tool annotations (audience, priority, destructive/read-only indicators) and structured output (outputSchema with JSON Schema validation using nlohmann/json-schema-validator)**

## Performance

- **Duration:** 2min
- **Started:** 2026-01-31T19:53:20Z
- **Completed:** 2026-01-31T19:55:32Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- **ToolAnnotations struct** with destructive, read_only, audience, priority fields for rich tool discovery
- **Output schema validation** using existing nlohmann/json_schema_validator dependency
- **Backward compatible API** - existing register_tool calls work without modification
- **Extended tool discovery** - tools/list now includes annotations and outputSchema fields
- **Proper error format** - output validation failures return CallToolResult with isError=true

## Task Commits

Each task was committed atomically:

1. **Task 1: Add tool annotations and output schema to ToolRegistration** - `55aa5a3` (feat)
2. **Task 2: Implement output schema validation in tool execution** - `d4905e7` (feat)

**Plan metadata:** TBD (docs: complete plan)

## Files Created/Modified

- `src/mcpp/server/tool_registry.h` - Added ToolAnnotations struct, extended ToolRegistration with output_schema/output_validator/annotations, added overloaded register_tool method
- `src/mcpp/server/tool_registry.cpp` - Implemented output schema compilation and validation, updated list_tools to include annotations and outputSchema, added make_output_validation_error helper

## Decisions Made

- **Backward compatibility via delegation**: Original register_tool() delegates to new overload with default values (ToolAnnotations{}, std::nullopt), ensuring existing code works unchanged
- **Output validation error format**: Output schema failures return CallToolResult with isError=true rather than JSON-RPC error, distinguishing tool contract violations from protocol errors
- **Struct initialization**: ToolAnnotations provides both default constructor and parameterized constructor for flexible initialization
- **Schema compilation reuse**: Output validator follows same compilation pattern as input validator (set_root_schema at registration time)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - implementation straightforward, no blocking issues.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Tool annotations enable UI filtering and prioritization in client implementations
- Output schema validation provides strong contracts for tool results
- Existing tools continue to work without modification
- Ready for resource template support (plan 04-04)

---
*Phase: 04-advanced-features--http-transport*
*Plan: 03*
*Completed: 2026-01-31*
