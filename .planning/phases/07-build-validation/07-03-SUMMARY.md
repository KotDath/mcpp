---
phase: 07-build-validation
plan: 03
subsystem: testing
tags: [json-rpc, compliance, testing, gtest, nlohmann-json]

# Dependency graph
requires:
  - phase: 01-protocol-foundation
    provides: JsonRpcRequest, JsonRpcResponse, JsonRpcError types
provides:
  - JSON-RPC 2.0 specification compliance test suite
  - Test fixture data for valid/invalid messages
  - Verified null ID handling in error responses
affects: []

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Data-driven testing with JSON fixtures
    - Compliance test patterns (spec validation)

key-files:
  created:
    - tests/data/jsonrpc_examples.json
    - tests/compliance/test_jsonrpc_spec.cpp
  modified:
    - src/mcpp/core/json_rpc.cpp
    - tests/CMakeLists.txt

key-decisions:
  - "Null ID values in error responses now parse correctly (using 0 as sentinel)"
  - "Test data files copied to build directory for CMake support"

patterns-established:
  - "Compliance test pattern: load fixtures from JSON, validate structure and parsing"
  - "Test data directory structure: tests/data/ for fixtures"

# Metrics
duration: 14min
completed: 2026-02-01
---

# Phase 7 Plan 3: JSON-RPC Compliance Tests Summary

**46 comprehensive JSON-RPC 2.0 specification compliance tests covering request/response formats, error codes, notifications, batch requests, and edge cases**

## Performance

- **Duration:** 14 min
- **Started:** 2026-01-31T22:54:47Z
- **Completed:** 2026-02-01T01:10:00Z
- **Tasks:** 3
- **Files modified:** 4

## Accomplishments

- Created comprehensive JSON-RPC 2.0 compliance test suite with 46 test cases
- Generated test fixture data covering valid/invalid requests, responses, notifications, and batch messages
- Fixed null ID parsing bug in JsonRpcResponse::from_json (spec compliance issue)
- Configured CMake to copy test data to build directory for reliable test access

## Task Commits

Each task was committed atomically:

1. **Task 1: Create JSON-RPC test fixture data** - `d853034` (test)
2. **Task 2: Update CMakeLists.txt for test data directory** - `830e187` (test)
3. **Task 3: Implement comprehensive JSON-RPC compliance tests** - `fb6b00a` (test)
4. **Task 4: Fix test implementation and null ID parsing** - `b7bf59e` (fix)

**Plan metadata:** (to be added after this summary)

## Files Created/Modified

- `tests/data/jsonrpc_examples.json` - JSON-RPC test fixtures (valid/invalid examples)
- `tests/compliance/test_jsonrpc_spec.cpp` - 46 compliance tests
- `src/mcpp/core/json_rpc.cpp` - Fixed null ID parsing per JSON-RPC 2.0 spec
- `tests/CMakeLists.txt` - Added TEST_DATA_DIR definition and data file copying

## Test Coverage

The compliance tests validate:

1. **Request Structure** (2 tests)
   - Required fields: jsonrpc="2.0", method (string)
   - ID types: integer, string, null
   - Parameter formats: positional array, named object, null

2. **Response Structure** (6 tests)
   - Required fields: jsonrpc="2.0", id, result OR error
   - Result/error mutual exclusivity
   - Success/error state methods

3. **Error Objects** (5 tests)
   - Standard error codes (-32700 to -32603)
   - Required fields: code (integer), message (string)
   - Optional data field with any JSON type
   - Factory method correctness

4. **Notifications** (3 tests)
   - No ID field requirement
   - Optional parameters
   - Valid method checking

5. **Batch Requests** (2 tests)
   - Array structure validation
   - Empty batch handling
   - Mixed notification/request batches

6. **Request Type Tests** (4 tests)
   - Serialization to valid JSON
   - String ID support
   - Null params handling
   - Array params support

7. **Response Type Tests** (5 tests)
   - Success result serialization
   - Null result handling
   - Error response serialization
   - Error data field
   - Parsing valid examples

8. **Edge Cases** (8 tests)
   - Empty string method
   - Large integer IDs (max safe integer)
   - Negative IDs
   - Zero ID
   - Very long string IDs (1000 chars)
   - Unicode in method names
   - Deeply nested parameters
   - Array params with null values

9. **Parse Error Cases** (6 tests)
   - Missing jsonrpc field
   - Wrong jsonrpc version
   - Missing id field
   - Both result and error (invalid)
   - Neither result nor error (invalid)
   - Invalid error object type

10. **Error Parse Cases** (5 tests)
    - Missing code field
    - Missing message field
    - Non-numeric code
    - Non-string message
    - Minimal valid error (code + message only)

## Decisions Made

1. **Null ID handling**: Extended `parse_request_id()` to accept null values per JSON-RPC 2.0 spec. Null IDs are used in error responses when the request ID couldn't be determined (e.g., parse errors). Implementation uses 0 as sentinel value.

2. **Test data location**: Placed fixtures in `tests/data/` and configured CMake to copy them to build directory. This ensures tests work in both in-tree and out-of-tree builds.

3. **Member assignment for aggregates**: Used explicit member assignment instead of aggregate initialization for struct construction in tests, avoiding C++ variant conversion ambiguities.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed null ID parsing in JsonRpcResponse**
- **Found during:** Task 2 (Test implementation)
- **Issue:** `parse_request_id()` returned nullopt for null IDs, causing error responses with `id: null` to fail parsing. This violates JSON-RPC 2.0 spec which allows null IDs in error responses.
- **Fix:** Extended `parse_request_id()` to handle `j.is_null()` case, returning 0 as sentinel value
- **Files modified:** src/mcpp/core/json_rpc.cpp
- **Verification:** Tests for error responses with null ID now pass (error_parse, error_invalid_request)
- **Committed in:** b7bf59e (Task 4)

**2. [Rule 1 - Bug] Fixed struct aggregate initialization in tests**
- **Found during:** Task 2 (Build verification)
- **Issue:** Initial brace-initialization syntax failed due to std::variant constructor ambiguity and struct field declaration order
- **Fix:** Changed to explicit member assignment (req.id = ..., req.method = ...) for all test code
- **Files modified:** tests/compliance/test_jsonrpc_spec.cpp
- **Verification:** All 46 tests compile and pass
- **Committed in:** b7bf59e (Task 4)

**3. [Rule 1 - Bug] Fixed InvalidRequest_Detection test logic**
- **Found during:** Task 2 (Test execution)
- **Issue:** Test expected "missing_method" invalid request to have a method field, which was incorrect
- **Fix:** Added special handling for the "missing_method" test case to verify it correctly lacks the method field
- **Files modified:** tests/compliance/test_jsonrpc_spec.cpp
- **Verification:** Test now correctly validates the invalid request structure
- **Committed in:** b7bf59e (Task 4)

**4. [Rule 1 - Bug] Fixed EdgeCase_NestedParams JSON construction**
- **Found during:** Task 2 (Test execution)
- **Issue:** Nested JSON initializer list was creating array instead of object, causing runtime exception
- **Fix:** Changed to explicit JSON object construction with bracket notation
- **Files modified:** tests/compliance/test_jsonrpc_spec.cpp
- **Verification:** Nested parameter test passes without exception
- **Committed in:** b7bf59e (Task 4)

---

**Total deviations:** 4 auto-fixed (all Rule 1 - Bug fixes)
**Impact on plan:** All fixes were necessary for correctness. One spec compliance bug (null ID), three test code bugs. No scope creep.

## Issues Encountered

1. **Build failures due to struct initialization**: C++ aggregate initialization with std::variant and mixed field types caused conversion ambiguities. Resolved by using explicit member assignment.

2. **CTest label not working**: Initial `ctest -L compliance` showed no tests due to stale build cache. Resolved by reconfiguring CMake.

## Verification

Run compliance tests:
```bash
cd build
cmake --build . --target mcpp_compliance_tests
ctest -R "^JsonRpcSpecCompliance\." --output-on-failure
```

All 46 tests pass:
- 100% tests passed, 0 tests failed
- Total test time: ~0.07 sec

## Next Phase Readiness

- JSON-RPC compliance validated against official 2.0 specification
- Test infrastructure in place for future protocol additions
- Ready for MCP Inspector integration (07-04)

---
*Phase: 07-build-validation*
*Plan: 03*
*Completed: 2026-02-01*
