# Phase 8: Bug Fix Foundation - Context

**Gathered:** 2026-02-01
**Status:** Ready for planning

## Phase Boundary

Library provides proper JSON-RPC request validation and stdio protocol compliance. This phase delivers library-layer fixes (new `JsonRpcRequest::from_json()`, output formatting helpers, debug routing) that the example server will use in Phase 9. No application changes or external dependencies.

## Implementation Decisions

### Validation layer placement
- Add `JsonRpcRequest::from_json()` as **static public method** mirroring `JsonRpcResponse::from_json()`
- Returns `std::optional<JsonRpcRequest>` on success, `nullopt` on parse failure
- Validates: `jsonrpc == "2.0"`, required fields present (`id`, `method`), `params` is object or array if present
- Invalid JSON produces detailed error diagnostics (missing field, wrong type) for debugging
- Located in `src/mcpp/core/json_rpc.h` (header-only implementation)

### Output delimiter strategy
- **Centralized approach**: Add `to_string_delimited()` helper to all JSON-RPC types (`JsonRpcRequest`, `JsonRpcResponse`, `JsonRpcNotification`)
- Each `to_string_delimited()` returns `.dump() + "\n"` for stdio compliance
- Keep existing `to_string()` (without delimiter) for non-stdio use cases (HTTP, testing)
- Phase 9 example server will use `to_string_delimited()` for all stdio output

### Debug output routing
- Logger output already routes to stderr via `logger.cpp` fallback (verified at line 264)
- **Add `MCPP_DEBUG_LOG` macro** that expands to `std::fprintf(stderr, ...)`
- Replace all `std::cout` usage in library code with `MCPP_DEBUG_LOG` or `Logger`
- For third-party code ( spdlog ), configure to write to stderr or file only
- Goal: zero `std::cout` writes in library code paths

### Validation error handling
- On `from_json()` failure, return `nullopt` (no exception)
- Add `JsonRpcRequest::ParseError` struct for detailed diagnostics when validation fails
- ParseError includes: error code enum, message string, optional request ID (extracted if available)
- Example server (Phase 9) can use ParseError to build proper error responses with correct `id` field
- **Strip invalid JSON from error messages** (security: avoid echoing potentially malicious input)

### Claude's Discretion
- Exact implementation of `ParseError` diagnostics (what fields to include)
- Whether `to_string_delimited()` also flushes or leaves that to caller
- Macro naming style for `MCPP_DEBUG_LOG` (could be `MCPP_LOG_DEBUG` or similar)
- Whether to add `-Werror` for treating debug output to stdout as compile error

## Specific Ideas

Based on research findings (SUMMARY.md):
- "Fix mirrors existing pattern: `JsonRpcResponse::from_json()` is already in the codebase — follow that structure"
- "Stdio pitfalls are clear: newline delimiter, stderr-only logging, explicit flush — bake these into library helpers"
- "Parse error needs detailed diagnostics for debugging — but strip actual JSON content for security"

## Deferred Ideas

None — discussion stayed within phase scope (library-layer fixes only).

---

*Phase: 08-bug-fix-foundation*
*Context gathered: 2026-02-01*
