---
phase: 03-client-capabilities
plan: 04
subsystem: client
tags: [sampling, tools, agentic, tool-loop, client]

# Dependency graph
requires:
  - phase: 01-protocol-foundation
    provides: JSON-RPC core types, RequestTracker, JsonValue type
  - phase: 02-core-server
    provides: Transport abstraction, McpClient base
  - phase: 03-client-capabilities
    plan: 03
    provides: SamplingClient, CreateMessageRequest, CreateMessageResult, SamplingHandler
provides:
  - ToolUseContent and ToolResultContent types for tool message content
  - ToolChoice variant (auto, required, none, specific tool)
  - Tool type for tool definitions with JSON Schema input_schema
  - ToolLoopConfig with max_iterations and timeout safety controls
  - execute_tool_loop function for agentic workflow execution
  - enable_tool_use_for_sampling() for McpClient tool call integration
affects: ["03-client-capabilities", "04-client-capabilities"]

# Tech tracking
tech-stack:
  added: []
  patterns: [agentic tool loops, variant-based content types, promise/future blocking for sync tool calls]

key-files:
  created: []
  modified: [src/mcpp/client/sampling.h, src/mcpp/client/sampling.cpp, src/mcpp/client.h, src/mcpp/client.cpp]

key-decisions:
  - "Tool types (Tool, ToolChoice) added for agentic sampling loops"
  - "ToolResultContent for structured tool return values with isError flag"
  - "CreateMessageRequest extended with optional tools and tool_choice parameters"
  - "CreateMessageResult extended with toolUseContent for multi-step tool loops"
  - "SamplingClient::handle_create_message_with_tools for tool loop execution"
  - "Maximum of 10 tool iterations to prevent infinite loops"

patterns-established:
  - "Agentic tool loop: LLM calls tools, results fed back, loop continues until done"
  - "Tool content as variant: ToolUseContent for calls, ToolResultContent for returns"
  - "Promise/future blocking for synchronous tool calls during tool loop"
  - "Safety controls: max_iterations and timeout prevent runaway tool loops"

# Metrics
duration: 2min
completed: 2026-01-31
---

# Phase 03: Client Capabilities - Plan 04 Summary

**Agentic tool loop support enabling LLMs to call MCP server tools with iteration limits and timeout protection**

## Performance

- **Duration:** 2 min
- **Started:** 2026-01-31T18:33:13Z
- **Completed:** 2026-01-31T18:35:48Z
- **Tasks:** 3
- **Files modified:** 4

## Accomplishments

- Extended ContentBlock variant to include ToolUseContent and ToolResultContent for tool messages
- Added ToolChoice variant (auto, required, none, specific tool) and Tool type for tool definitions
- Implemented execute_tool_loop with iteration limits and timeout protection
- Integrated tool loop with SamplingClient for automatic tool execution during sampling
- Connected tool loop to McpClient via enable_tool_use_for_sampling() with promise/future blocking

## Task Commits

Each task was committed atomically:

1. **Task 1: Extend sampling types for tool use** - `e241dfe` (feat)
2. **Task 2: Implement tool loop executor in SamplingClient** - `d4715e8` (feat)
3. **Task 3: Connect tool loop to McpClient** - `372df77` (feat)

**Plan metadata:** N/A (created after completion)

## Files Created/Modified

- `src/mcpp/client/sampling.h` - Extended with tool use types (462 lines)
  - ToolUseContent with id, name, arguments fields
  - ToolResultContent with tool_use_id, content, is_error fields
  - ToolChoice variant: ToolChoiceAuto, ToolChoiceRequired, ToolChoiceNone, ToolChoiceTool
  - Tool struct with name and input_schema (JSON Schema)
  - ToolLoopConfig with max_iterations (default 10) and timeout (default 5 minutes)
  - SamplingClient extended with set_tool_loop_config, set_tool_caller, clear_tool_caller
  - ContentBlock variant now includes ToolUseContent and ToolResultContent
- `src/mcpp/client/sampling.cpp` - Tool loop implementation (525 lines)
  - Helper functions: is_tool_use, contains_tool_use, extract_tool_uses
  - CreateMessageRequest::from_json extended to parse tools and tool_choice
  - call_tool helper for executing individual tool calls via tool_caller
  - execute_tool_loop main function with iteration/timeout checks
  - SamplingClient::handle_create_message updated to use tool loop when tools present
- `src/mcpp/client.h` - Added tool use methods (374 lines)
  - enable_tool_use_for_sampling(bool enable) for enabling/disabling tool calls
  - get_tool_loop_config() accessors for configuring iteration limits
- `src/mcpp/client.cpp` - Tool caller setup with promise/future blocking (600 lines)
  - enable_tool_use_for_sampling implementation with lambda capturing this
  - Synchronous tool_caller using std::promise/std::future with 30-second timeout
  - Tool caller sends JSON-RPC request via transport, blocks on response

## Decisions Made

- **ContentBlock as std::variant**: Extended to include ToolUseContent and ToolResultContent alongside TextContent
- **ToolChoice variant design**: Follows MCP spec with auto/required/none/specific tool options
- **Tool loop safety**: max_iterations (default 10) and timeout (default 5 min) prevent runaway loops
- **Synchronous tool calling**: Used promise/future blocking for tool calls during tool loop (acceptable limitation for MVP)
- **Tool result extraction**: Combines multiple text content blocks from tool/call response
- **Error handling**: Tool call failures wrapped in ToolResultContent with is_error=true

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Tool loop complete and ready for elicitation support (plan 03-05)
- ToolLoopConfig can be adjusted via get_tool_loop_config() accessor
- enable_tool_use_for_sampling() must be called before tool loop execution
- Synchronous tool blocking is MVP limitation; async approach would require non-blocking event loop

---
*Phase: 03-client-capabilities*
*Completed: 2026-01-31*
