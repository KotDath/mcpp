---
phase: 07-build-validation
plan: 02a
type: execute
wave: 1
depends_on: [07-01]
files_modified:
  - tests/CMakeLists.txt
  - tests/fixtures/common.h
  - tests/integration/test_client_server.cpp
  - tests/compliance/test_jsonrpc_spec.cpp
  - CMakeLists.txt
autonomous: true
user_setup: []

must_haves:
  truths:
    - "Library consumer can enable tests via MCPP_BUILD_TESTS CMake option"
    - "GoogleTest is fetched via FetchContent for reproducible builds"
    - "Test directory structure exists (unit, integration, compliance, fixtures, data)"
    - "Stub test files exist for integration and compliance tests (populated in later plans)"
  artifacts:
    - path: "tests/CMakeLists.txt"
      provides: "Test configuration with GoogleTest integration"
      contains: "FetchContent_Declare", "gtest_discover_tests", "MCPP_BUILD_TESTS"
    - path: "tests/fixtures/common.h"
      provides: "Common test fixtures and utilities"
      contains: "class JsonFixture", "class TimeFixture"
    - path: "tests/unit/"
      provides: "Unit test directory"
    - path: "tests/integration/"
      provides: "Integration test directory"
    - path: "tests/compliance/"
      provides: "Compliance test directory"
  key_links:
    - from: "CMakeLists.txt"
      to: "tests/CMakeLists.txt"
      via: "MCPP_BUILD_TESTS option in add_subdirectory"
      pattern: "if\\(MCPP_BUILD_TESTS\\)"
    - from: "tests/CMakeLists.txt"
      to: "mcpp library"
      via: "target_link_libraries in test executables"
      pattern: "target_link_libraries.*mcpp"
---

<objective>
Set up test infrastructure: directory structure, GoogleTest integration via FetchContent, common test fixtures, and stub test files for integration and compliance tests.

Purpose: Establish the foundation for all testing (unit, integration, compliance) without implementing actual test cases yet. This separates infrastructure setup from test implementation.

Output: Working test directory structure, GoogleTest integration, common fixtures, and stub files ready for unit tests (plan 07-02b) and other test suites (plans 07-03, 07-04).
</objective>

<execution_context>
@/home/kotdath/.claude/get-shit-done/workflows/execute-plan.md
@/home/kotdath/.claude/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/phases/07-build-validation/07-CONTEXT.md
@.planning/phases/07-build-validation/07-RESEARCH.md
@CMakeLists.txt
@.planning/phases/07-build-validation/07-01-SUMMARY.md
</context>

<tasks>

<task type="auto">
  <name>Create tests directory structure and CMakeLists.txt</name>
  <files>tests/CMakeLists.txt</files>
  <action>
Create tests/ directory and tests/CMakeLists.txt with:

```cmake
# mcpp test suite

# Use FetchContent for reproducible GoogleTest builds
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.14.0
)
# For Windows: Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Unit tests
add_executable(mcpp_unit_tests
    unit/test_json_rpc.cpp
    unit/test_request_tracker.cpp
    unit/test_timeout_manager.cpp
    unit/test_tool_registry.cpp
    unit/test_resource_registry.cpp
    unit/test_prompt_registry.cpp
    unit/test_pagination.cpp
)

target_link_libraries(mcpp_unit_tests
    PRIVATE
        mcpp
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(mcpp_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

gtest_discover_tests(mcpp_unit_tests
    LABELS unit
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Integration tests (empty for now, will be populated in 07-04)
add_executable(mcpp_integration_tests
    integration/test_client_server.cpp
)

target_link_libraries(mcpp_integration_tests
    PRIVATE
        mcpp
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(mcpp_integration_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

gtest_discover_tests(mcpp_integration_tests
    LABELS integration
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Compliance tests (empty for now, will be populated in 07-03)
add_executable(mcpp_compliance_tests
    compliance/test_jsonrpc_spec.cpp
)

target_link_libraries(mcpp_compliance_tests
    PRIVATE
        mcpp
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(mcpp_compliance_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(mcpp_compliance_tests PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

gtest_discover_tests(mcpp_compliance_tests
    LABELS compliance
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
```

Create stub directories:
```bash
mkdir -p tests/unit tests/integration tests/compliance tests/fixtures tests/data
```
  </action>
  <verify>Directory structure exists with tests/CMakeLists.txt</verify>
  <done>Test directory structure created with CMakeLists.txt configured for GoogleTest</done>
</task>

<task type="auto">
  <name>Create common test fixtures</name>
  <files>tests/fixtures/common.h</files>
  <action>
Create tests/fixtures/common.h with common test utilities:

```cpp
// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#ifndef MCPP_TESTS_FIXTURES_COMMON_H
#define MCPP_TESTS_FIXTURES_COMMON_H

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <chrono>

namespace mcpp::test {

// JSON test fixture with common helper methods
class JsonFixture : public ::testing::Test {
protected:
    // Parse JSON string, expect success
    nlohmann::json parse(const std::string& str) {
        try {
            return nlohmann::json::parse(str);
        } catch (const nlohmann::json::parse_error& e) {
            ADD_FAILURE() << "JSON parse failed: " << e.what();
            return nlohmann::json{};
        }
    }

    // Expect valid JSON-RPC request
    bool is_valid_request(const nlohmann::json& j) {
        return j.contains("jsonrpc") && j["jsonrpc"] == "2.0" &&
               (j.contains("id") || j.contains("method"));
    }

    // Expect valid JSON-RPC response
    bool is_valid_response(const nlohmann::json& j) {
        return j.contains("jsonrpc") && j["jsonrpc"] == "2.0" && j.contains("id");
    }

    // Expect valid JSON-RPC error
    bool is_valid_error(const nlohmann::json& j) {
        return j.contains("jsonrpc") && j["jsonrpc"] == "2.0" &&
               j.contains("error") && j["error"].is_object() &&
               j["error"].contains("code") && j["error"].contains("message");
    }
};

// Time-based test fixture for timeout tests
class TimeFixture : public ::testing::Test {
protected:
    void SetUp() override {
        start_time = std::chrono::steady_clock::now();
    }

    // Elapsed time since test start in milliseconds
    int64_t elapsed_ms() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
    }

    // Assert elapsed time is approximately expected (within tolerance ms)
    void assert_elapsed_approx(int64_t expected_ms, int64_t tolerance_ms = 50) {
        int64_t actual = elapsed_ms();
        EXPECT_GE(actual, expected_ms - tolerance_ms);
        EXPECT_LE(actual, expected_ms + tolerance_ms);
    }

private:
    std::chrono::steady_clock::time_point start_time;
};

} // namespace mcpp::test

#endif // MCPP_TESTS_FIXTURES_COMMON_H
```
  </action>
  <verify>File tests/fixtures/common.h exists with JsonFixture and TimeFixture classes</verify>
  <done>Common test fixtures created for JSON and time-based testing</done>
</task>

<task type="auto">
  <name>Create stub test files and wire tests into root CMakeLists.txt</name>
  <files>tests/integration/test_client_server.cpp, tests/compliance/test_jsonrpc_spec.cpp, CMakeLists.txt</files>
  <action>
1. Create stub test files that will be filled in later plans:

tests/integration/test_client_server.cpp:
```cpp
// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/client.h"
#include "mcpp/server/mcp_server.h"
#include <gtest/gtest.h>

// Placeholder for integration tests
// Will be populated in plan 07-04 (MCP Inspector integration)

TEST(ClientServerIntegration, Placeholder) {
    // Integration tests will verify:
    // - Full MCP handshake between client and server
    // - Tool call round-trip
    // - Resource read round-trip
    // - Prompt retrieval round-trip
    SUCCEED() << "Integration tests placeholder - to be implemented in 07-04";
}
```

tests/compliance/test_jsonrpc_spec.cpp:
```cpp
// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include <gtest/gtest.h>

// Placeholder for JSON-RPC spec compliance tests
// Will be populated in plan 07-03 (JSON-RPC compliance)

TEST(JsonRpcSpecCompliance, Placeholder) {
    // Compliance tests will verify:
    // - JSON-RPC 2.0 spec conformance
    // - MCP protocol compliance
    // - Error code correctness
    SUCCEED() << "Compliance tests placeholder - to be implemented in 07-03";
}
```

2. Update root CMakeLists.txt to conditionally include tests:

Find the "Testing" section (around line 180) and add:
```cmake
# Testing
option(MCPP_BUILD_TESTS "Build test suite" ON)
if(MCPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
```

Also add stub unit test files (empty, to be filled in 07-02b):
```bash
touch tests/unit/test_json_rpc.cpp
touch tests/unit/test_request_tracker.cpp
touch tests/unit/test_timeout_manager.cpp
touch tests/unit/test_tool_registry.cpp
touch tests/unit/test_resource_registry.cpp
touch tests/unit/test_prompt_registry.cpp
touch tests/unit/test_pagination.cpp
```

Each stub file should contain:
```cpp
// mcpp - MCP C++ library
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include <gtest/gtest.h>

// Placeholder - to be implemented in plan 07-02b
TEST(Placeholder, Placeholder) {
    SUCCEED() << "Unit test placeholder - to be implemented in 07-02b";
}
```
  </action>
  <verify>
- Stub files exist in tests/integration/ and tests/compliance/
- Root CMakeLists.txt has MCPP_BUILD_TESTS option
- Unit test stub files exist in tests/unit/
  </verify>
  <done>Stub test files created and tests wired into CMake build with MCPP_BUILD_TESTS option</done>
</task>

</tasks>

<verification>
After all tasks complete, verify:
1. tests/CMakeLists.txt configured with FetchContent for GoogleTest
2. Directory structure exists: tests/unit, tests/integration, tests/compliance, tests/fixtures, tests/data
3. tests/fixtures/common.h exists with JsonFixture and TimeFixture
4. Stub test files exist (unit, integration, compliance)
5. Root CMakeLists.txt has MCPP_BUILD_TESTS option
6. cmake -B build && cmake --build build compiles test executables (even if tests are placeholders)
</verification>

<success_criteria>
1. Test directory structure created
2. GoogleTest integration configured via FetchContent
3. Common test fixtures (JsonFixture, TimeFixture) available
4. Stub test files allow build to succeed
5. MCPP_BUILD_TESTS CMake option works (can disable tests with -DMCPP_BUILD_TESTS=OFF)
</success_criteria>

<output>
After completion, create `.planning/phases/07-build-validation/07-02a-SUMMARY.md` with:
- Test infrastructure setup details
- Directory structure created
- GoogleTest integration method (FetchContent)
- MCPP_BUILD_TESTS option added to CMakeLists.txt
</output>
