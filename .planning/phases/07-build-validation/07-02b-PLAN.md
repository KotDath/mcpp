---
phase: 07-build-validation
plan: 02b
type: execute
wave: 2
depends_on: [07-02a]
files_modified:
  - tests/unit/test_json_rpc.cpp
  - tests/unit/test_request_tracker.cpp
  - tests/unit/test_tool_registry.cpp
  - tests/unit/test_resource_registry.cpp
  - tests/unit/test_prompt_registry.cpp
  - tests/unit/test_pagination.cpp
  - tests/CMakeLists.txt
autonomous: true
user_setup: []

must_haves:
  truths:
    - "Library consumer can run unit tests using 'ctest -L unit'"
    - "Unit tests cover all major modules: JSON-RPC, request tracker, timeout manager, registries, pagination"
    - "Tests use common fixtures from tests/fixtures/common.h"
    - "All unit tests pass with no failures"
  artifacts:
    - path: "tests/unit/test_json_rpc.cpp"
      provides: "JSON-RPC parsing and serialization tests"
      min_lines: 200
    - path: "tests/unit/test_request_tracker.cpp"
      provides: "Request tracker and timeout manager tests"
      min_lines: 150
    - path: "tests/unit/test_tool_registry.cpp"
      provides: "Tool registry unit tests"
      min_lines: 50
    - path: "tests/unit/test_resource_registry.cpp"
      provides: "Resource registry unit tests"
      min_lines: 50
    - path: "tests/unit/test_prompt_registry.cpp"
      provides: "Prompt registry unit tests"
      min_lines: 50
    - path: "tests/unit/test_pagination.cpp"
      provides: "Pagination helper tests"
      min_lines: 60
  key_links:
    - from: "tests/unit/*.cpp"
      to: "tests/fixtures/common.h"
      via: "#include \"fixtures/common.h\""
      pattern: "#include.*fixtures/common"
    - from: "tests/unit/*.cpp"
      to: "mcpp library headers"
      via: "#include \"mcpp/...\""
      pattern: "#include.*mcpp/"
---

<objective>
Implement comprehensive unit tests for all major library modules: JSON-RPC core, request tracker, timeout manager, tool/resource/prompt registries, and pagination helpers.

Purpose: Deliver thorough unit test coverage for core functionality, enabling automated validation and regression prevention. Uses the test infrastructure set up in plan 07-02a.

Output: Complete unit test suite with 20+ test cases covering JSON-RPC types, request tracking, timeout management, server registries, and pagination utilities.
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
@.planning/phases/07-build-validation/07-02a-SUMMARY.md
@src/mcpp/core/json_rpc.h
@src/mcpp/core/request_tracker.h
@src/mcpp/server/tool_registry.h
@src/mcpp/server/resource_registry.h
@src/mcpp/server/prompt_registry.h
@src/mcpp/util/pagination.h
</context>

<tasks>

<task type="auto">
  <name>Create JSON-RPC unit tests</name>
  <files>tests/unit/test_json_rpc.cpp</files>
  <action>
Replace the placeholder in tests/unit/test_json_rpc.cpp with comprehensive JSON-RPC tests:

```cpp
// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/core/json_rpc.h"
#include "fixtures/common.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mcpp;
using namespace mcpp::core;
using namespace mcpp::test;

// ============================================================================
// JsonRpcRequest Tests
// ============================================================================

TEST(JsonRpcRequest, DefaultConstruction) {
    JsonRpcRequest req;
    EXPECT_EQ(req.method, "");
    EXPECT_TRUE(req.params.is_null());
    EXPECT_TRUE(req.id.is_null());
}

TEST(JsonRpcRequest, ParameterizedConstruction) {
    nlohmann::json id = 42;
    JsonRpcRequest req("test/method", nlohmann::json::object(), id);

    EXPECT_EQ(req.method, "test/method");
    EXPECT_TRUE(req.params.is_object());
    EXPECT_EQ(req.id, id);
}

TEST(JsonRpcRequest, ToJson_ValidRequest) {
    JsonRpcRequest req("test/method", {{"key", "value"}}, 1);
    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_EQ(j["method"], "test/method");
    EXPECT_EQ(j["params"]["key"], "value");
    EXPECT_EQ(j["id"], 1);
}

TEST(JsonRpcRequest, FromJson_ValidRequest) {
    nlohmann::json j = {
        {"jsonrpc", "2.0"},
        {"method", "test/method"},
        {"params", {{"arg1", "value1"}}},
        {"id", 123}
    };

    auto req = JsonRpcRequest::from_json(j);
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->method, "test/method");
    EXPECT_EQ(req->params["arg1"], "value1");
    EXPECT_EQ(req->id, 123);
}

TEST(JsonRpcRequest, FromJson_MissingJsonrpc_ReturnsNullopt) {
    nlohmann::json j = {
        {"method", "test/method"},
        {"id", 1}
    };

    auto req = JsonRpcRequest::from_json(j);
    EXPECT_FALSE(req.has_value());
}

TEST(JsonRpcRequest, FromJson_WrongJsonrpcVersion_ReturnsNullopt) {
    nlohmann::json j = {
        {"jsonrpc", "1.0"},
        {"method", "test/method"},
        {"id", 1}
    };

    auto req = JsonRpcRequest::from_json(j);
    EXPECT_FALSE(req.has_value());
}

// ============================================================================
// JsonRpcResponse Tests
// ============================================================================

TEST(JsonRpcResponse, SuccessResult) {
    nlohmann::json result = {{"status", "ok"}};
    JsonRpcResponse resp(result, 1);

    EXPECT_FALSE(resp.is_error());
    EXPECT_EQ(resp.result["status"], "ok");
    EXPECT_EQ(resp.id, 1);
}

TEST(JsonRpcResponse, ErrorResult) {
    JsonRpcError error(JsonRpcError::ErrorCode::InvalidParams, "Invalid parameters");
    JsonRpcResponse resp(error, 2);

    EXPECT_TRUE(resp.is_error());
    EXPECT_EQ(resp.error.code, static_cast<int>(JsonRpcError::ErrorCode::InvalidParams));
    EXPECT_EQ(resp.error.message, "Invalid parameters");
    EXPECT_EQ(resp.id, 2);
}

TEST(JsonRpcResponse, ToJson_SuccessResponse) {
    nlohmann::json result = {{"data", "value"}};
    JsonRpcResponse resp(result, 42);
    nlohmann::json j = resp.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_EQ(j["result"]["data"], "value");
    EXPECT_EQ(j["id"], 42);
    EXPECT_FALSE(j.contains("error"));
}

TEST(JsonRpcResponse, ToJson_ErrorResponse) {
    JsonRpcError error(JsonRpcError::ErrorCode::MethodNotFound, "Method not found", 42);
    JsonRpcResponse resp(error, 1);
    nlohmann::json j = resp.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_TRUE(j.contains("error"));
    EXPECT_EQ(j["error"]["code"], static_cast<int>(JsonRpcError::ErrorCode::MethodNotFound));
    EXPECT_EQ(j["error"]["message"], "Method not found");
    EXPECT_EQ(j["error"]["data"], 42);
    EXPECT_EQ(j["id"], 1);
    EXPECT_FALSE(j.contains("result"));
}

// ============================================================================
// JsonRpcError Tests
// ============================================================================

TEST(JsonRpcError, StandardErrorCodes) {
    JsonRpcError parse_err(JsonRpcError::ErrorCode::ParseError, "Parse error");
    EXPECT_EQ(parse_err.code, -32700);

    JsonRpcError req_err(JsonRpcError::ErrorCode::InvalidRequest, "Invalid request");
    EXPECT_EQ(req_err.code, -32600);

    JsonRpcError method_err(JsonRpcError::ErrorCode::MethodNotFound, "Method not found");
    EXPECT_EQ(method_err.code, -32601);

    JsonRpcError params_err(JsonRpcError::ErrorCode::InvalidParams, "Invalid params");
    EXPECT_EQ(params_err.code, -32602);

    JsonRpcError internal_err(JsonRpcError::ErrorCode::InternalError, "Internal error");
    EXPECT_EQ(internal_err.code, -32603);
}

TEST(JsonRpcError, FactoryMethods) {
    auto parse_err = JsonRpcError::parse_error("Failed to parse JSON");
    EXPECT_EQ(parse_err.code, -32700);
    EXPECT_EQ(parse_err.message, "Failed to parse JSON");

    auto invalid_req = JsonRpcError::invalid_request("Malformed JSON-RPC");
    EXPECT_EQ(invalid_req.code, -32600);

    auto method_not_found = JsonRpcError::method_notfound("unknown_method");
    EXPECT_EQ(method_not_found.code, -32601);

    auto invalid_params = JsonRpcError::invalid_params("Missing required field");
    EXPECT_EQ(invalid_params.code, -32602);

    auto internal_err = JsonRpcError::internal_error("Something went wrong");
    EXPECT_EQ(internal_err.code, -32603);
}

// ============================================================================
// JsonRpcNotification Tests
// ============================================================================

TEST(JsonRpcNotification, Construction) {
    JsonRpcNotification notif("notifications/message", {{"content", "hello"}});

    EXPECT_EQ(notif.method, "notifications/message");
    EXPECT_EQ(notif.params["content"], "hello");
}

TEST(JsonRpcNotification, ToJson) {
    JsonRpcNotification notif("test/notification", nlohmann::json::object());
    nlohmann::json j = notif.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_EQ(j["method"], "test/notification");
    EXPECT_FALSE(j.contains("id"));  // Notifications don't have IDs
}

TEST(JsonRpcNotification, FromJson_ValidNotification) {
    nlohmann::json j = {
        {"jsonrpc", "2.0"},
        {"method", "notifications/cancelled"},
        {"params", {{"reason", "user aborted"}}}
    };

    auto notif = JsonRpcNotification::from_json(j);
    ASSERT_TRUE(notif.has_value());
    EXPECT_EQ(notif->method, "notifications/cancelled");
    EXPECT_EQ(notif->params["reason"], "user aborted");
}

TEST(JsonRpcNotification, FromJson_Request_ReturnsNullopt) {
    // Requests have IDs, notifications don't
    nlohmann::json j = {
        {"jsonrpc", "2.0"},
        {"method", "test/method"},
        {"id", 1}
    };

    auto notif = JsonRpcNotification::from_json(j);
    EXPECT_FALSE(notif.has_value());
}
```
  </action>
  <verify>File tests/unit/test_json_rpc.cpp exists with comprehensive JSON-RPC tests</verify>
  <done>JSON-RPC unit tests created covering Request, Response, Error, and Notification types</done>
</task>

<task type="auto">
  <name>Create request tracker and timeout manager unit tests</name>
  <files>tests/unit/test_request_tracker.cpp</files>
  <action>
Replace the placeholder in tests/unit/test_request_tracker.cpp with request tracker and timeout manager tests:

```cpp
// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/core/request_tracker.h"
#include "fixtures/common.h"

#include <gtest/gtest.h>
#include <functional>
#include <thread>
#include <chrono>

using namespace mcpp;
using namespace mcpp::core;
using namespace mcpp::async;

using ResponseCallback = std::function<void(const nlohmann::json&)>;
using NotificationCallback = std::function<void(const nlohmann::json&)>;

// ============================================================================
// RequestTracker Tests
// ============================================================================

class RequestTrackerTest : public ::testing::Test {
protected:
    void SetUp() override {
        tracker = std::make_unique<RequestTracker>();
    }

    std::unique_ptr<RequestTracker> tracker;
};

TEST_F(RequestTrackerTest, GenerateId_Increments) {
    auto id1 = tracker->generate_id();
    auto id2 = tracker->generate_id();

    EXPECT_NE(id1, id2);
    EXPECT_GT(tracker->generate_id(), id2);
}

TEST_F(RequestTrackerTest, Register_CallbackStored) {
    bool called = false;
    ResponseCallback cb = [&called](const nlohmann::json&) { called = true; };

    RequestTracker::RequestId id = 42;
    tracker->register_request(id, cb);

    // Verify callback is stored (we can't directly inspect, but we can test via trigger)
}

TEST_F(RequestTrackerTest, Unregister_RemovesCallback) {
    bool called = false;
    ResponseCallback cb = [&called](const nlohmann::json&) { called = true; };

    RequestTracker::RequestId id = 1;
    tracker->register_request(id, cb);
    tracker->unregister(id);

    // After unregister, response shouldn't trigger callback
    nlohmann::json response = {{"result", "success"}, {"id", id}};
    tracker->handle_response(response);

    EXPECT_FALSE(called);
}

TEST_F(RequestTrackerTest, HandleResponse_TriggersCallback) {
    bool called = false;
    nlohmann::json received_result;

    ResponseCallback cb = [&called, &received_result](const nlohmann::json& resp) {
        called = true;
        received_result = resp;
    };

    RequestTracker::RequestId id = 99;
    tracker->register_request(id, cb);

    nlohmann::json response = {{"result", {{"data", "test"}}}, {"id", id}};
    tracker->handle_response(response);

    EXPECT_TRUE(called);
    EXPECT_EQ(received_result["result"]["data"], "test");
}

TEST_F(RequestTrackerTest, HandleResponse_MultipleRequests) {
    bool cb1_called = false, cb2_called = false;

    ResponseCallback cb1 = [&cb1_called](const nlohmann::json&) { cb1_called = true; };
    ResponseCallback cb2 = [&cb2_called](const nlohmann::json&) { cb2_called = true; };

    tracker->register_request(1, cb1);
    tracker->register_request(2, cb2);

    nlohmann::json response1 = {{"result", "ok"}, {"id", 1}};
    tracker->handle_response(response1);

    EXPECT_TRUE(cb1_called);
    EXPECT_FALSE(cb2_called);

    nlohmann::json response2 = {{"result", "ok"}, {"id", 2}};
    tracker->handle_response(response2);

    EXPECT_TRUE(cb2_called);
}

// ============================================================================
// TimeoutManager Tests
// ============================================================================

class TimeoutManagerTest : public TimeFixture {};

TEST_F(TimeoutManagerTest, Register_RequestTracked) {
    TimeoutManager mgr;
    TimeoutManager::RequestId id = 1;
    std::chrono::milliseconds timeout(100);

    mgr.register_request(id, timeout);

    // Request is now tracked for timeout
}

TEST_F(TimeoutManagerTest, Register_ThenUnregister_NotExpired) {
    TimeoutManager mgr;
    TimeoutManager::RequestId id = 1;
    std::chrono::milliseconds timeout(100);

    mgr.register_request(id, timeout);
    mgr.unregister(id);

    auto expired = mgr.check_expired();
    EXPECT_EQ(expired.size(), 0);
}

TEST_F(TimeoutManagerTest, CheckExpired_ReturnsTimedOutRequests) {
    TimeoutManager mgr;
    std::chrono::milliseconds timeout(50);

    mgr.register_request(1, timeout);
    mgr.register_request(2, timeout);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    auto expired = mgr.check_expired();
    EXPECT_GE(expired.size(), 1);
}

TEST_F(TimeoutManagerTest, CheckExpired_BeforeTimeout_NoExpiry) {
    TimeoutManager mgr;
    std::chrono::milliseconds timeout(1000);

    mgr.register_request(1, timeout);

    // Check immediately, should not be expired
    auto expired = mgr.check_expired();
    EXPECT_EQ(expired.size(), 0);
}
```
  </action>
  <verify>File tests/unit/test_request_tracker.cpp exists with request tracker and timeout manager tests</verify>
  <done>Request tracker unit tests created covering ID generation, callback registration, and timeout handling</done>
</task>

<task type="auto">
  <name>Create registry unit tests</name>
  <files>tests/unit/test_tool_registry.cpp, tests/unit/test_resource_registry.cpp, tests/unit/test_prompt_registry.cpp</files>
  <action>
Replace placeholders in the three registry test files:

tests/unit/test_tool_registry.cpp:
```cpp
// mcpp - MCP C++ library
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/tool_registry.h"
#include <gtest/gtest.h>

using namespace mcpp;
using namespace mcpp::server;

TEST(ToolRegistry, RegisterAndList) {
    ToolRegistry registry;

    registry.register_tool("test_tool", {{"description", "A test tool"}}, [](const nlohmann::json&) {
        return ToolResult{{"content", "executed"}};
    });

    auto tools = registry.list_tools();
    EXPECT_EQ(tools.size(), 1);
    EXPECT_EQ(tools[0]["name"], "test_tool");
}

TEST(ToolRegistry, CallTool_ExecutesHandler) {
    ToolRegistry registry;
    bool called = false;

    registry.register_tool("callable", {{"description", "Can be called"}}, [&called](const nlohmann::json&) {
        called = true;
        return ToolResult{{"content", "done"}};
    });

    auto result = registry.call_tool("callable", {{}});
    EXPECT_TRUE(called);
}

TEST(ToolRegistry, CallTool_UnknownTool_ReturnsError) {
    ToolRegistry registry;
    auto result = registry.call_tool("unknown", {{}});

    EXPECT_TRUE(result.contains("error"));
}

TEST(ToolRegistry, Unregister_RemovesTool) {
    ToolRegistry registry;

    registry.register_tool("to_remove", {{"description", "Will be removed"}}, [](const nlohmann::json&) {
        return ToolResult{{"content", "x"}};
    });

    EXPECT_EQ(registry.list_tools().size(), 1);

    registry.unregister("to_remove");
    EXPECT_EQ(registry.list_tools().size(), 0);
}
```

tests/unit/test_resource_registry.cpp:
```cpp
// mcpp - MCP C++ library
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/resource_registry.h"
#include <gtest/gtest.h>

using namespace mcpp;
using namespace mcpp::server;

TEST(ResourceRegistry, RegisterAndList) {
    ResourceRegistry registry;

    registry.register_resource("file://test.txt", "text/plain", [](const std::string&) {
        return ReadResourceResult{{"contents", "Hello"}};
    });

    auto resources = registry.list_resources();
    EXPECT_EQ(resources.size(), 1);
    EXPECT_EQ(resources[0]["uri"], "file://test.txt");
}

TEST(ResourceRegistry, ReadResource_ExecutesHandler) {
    ResourceRegistry registry;
    bool called = false;

    registry.register_resource("file://data", "text/plain", [&called](const std::string&) {
        called = true;
        return ReadResourceResult{{"contents", "data"}};
    });

    auto result = registry.read_resource("file://data");
    EXPECT_TRUE(called);
}

TEST(ResourceRegistry, Unregister_RemovesResource) {
    ResourceRegistry registry;

    registry.register_resource("file://tmp", "text/plain", [](const std::string&) {
        return ReadResourceResult{{"contents", "x"}};
    });

    EXPECT_EQ(registry.list_resources().size(), 1);

    registry.unregister("file://tmp");
    EXPECT_EQ(registry.list_resources().size(), 0);
}
```

tests/unit/test_prompt_registry.cpp:
```cpp
// mcpp - MCP C++ library
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/prompt_registry.h"
#include <gtest/gtest.h>

using namespace mcpp;
using namespace mcpp::server;

TEST(PromptRegistry, RegisterAndList) {
    PromptRegistry registry;

    registry.register_prompt("greeting", {{"description", "Say hello"}}, [](const nlohmann::json&) {
        return GetPromptResult{{"messages", {{{{{"role", "user"}, {"content", "Hello"}}}}}}};
    });

    auto prompts = registry.list_prompts();
    EXPECT_EQ(prompts.size(), 1);
    EXPECT_EQ(prompts[0]["name"], "greeting");
}

TEST(PromptRegistry, GetPrompt_ExecutesHandler) {
    PromptRegistry registry;
    bool called = false;

    registry.register_prompt("test", {{"description", "Test prompt"}}, [&called](const nlohmann::json&) {
        called = true;
        return GetPromptResult{{"messages", {}}};
    });

    auto result = registry.get_prompt("test", {{}});
    EXPECT_TRUE(called);
}

TEST(PromptRegistry, Unregister_RemovesPrompt) {
    PromptRegistry registry;

    registry.register_prompt("to_remove", {{"description", "Will be removed"}}, [](const nlohmann::json&) {
        return GetPromptResult{{"messages", {}}};
    });

    EXPECT_EQ(registry.list_prompts().size(), 1);

    registry.unregister("to_remove");
    EXPECT_EQ(registry.list_prompts().size(), 0);
}
```
  </action>
  <verify>All three registry test files exist with basic registration, listing, calling, and unregister tests</verify>
  <done>Registry unit tests created for tools, resources, and prompts</done>
</task>

<task type="auto">
  <name>Create pagination unit tests</name>
  <files>tests/unit/test_pagination.cpp</files>
  <action>
Replace the placeholder in tests/unit/test_pagination.cpp:

```cpp
// mcpp - MCP C++ library
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/util/pagination.h"
#include <gtest/gtest.h>

using namespace mcpp;
using namespace mcpp::util;

TEST(PaginatedResult, HasMore_WithCursor_ReturnsTrue) {
    std::vector<int> items = {1, 2, 3};
    PaginatedResult<int> result{items, "next_cursor", 10};

    EXPECT_TRUE(result.has_more());
    EXPECT_EQ(result.next_cursor, "next_cursor");
    EXPECT_EQ(result.total, 10);
}

TEST(PaginatedResult, HasMore_NoCursor_ReturnsFalse) {
    std::vector<int> items = {1, 2, 3};
    PaginatedResult<int> result{items, std::nullopt, 3};

    EXPECT_FALSE(result.has_more());
    EXPECT_EQ(result.total, 3);
}

TEST(PaginatedResult, ToJson_SerializesCorrectly) {
    std::vector<int> items = {1, 2};
    PaginatedResult<int> result{items, "abc", 5};

    nlohmann::json j = result.to_json([](const int& item) {
        return nlohmann::json{{"value", item}};
    });

    EXPECT_EQ(j["items"].size(), 2);
    EXPECT_EQ(j["items"][0]["value"], 1);
    EXPECT_EQ(j["nextCursor"], "abc");
    EXPECT_EQ(j["total"], 5);
}

TEST(ListAllTest, SinglePage) {
    // Mock function that returns a paginated result
    auto fetch_page = [](const std::string& cursor) -> PaginatedResult<int> {
        if (cursor.empty()) {
            return {{1, 2, 3}, std::nullopt, 3};
        }
        return {{}, std::nullopt, 0};
    };

    auto all_items = list_all<int>(fetch_page);
    EXPECT_EQ(all_items.size(), 3);
}

TEST(ListAllTest, MultiplePages) {
    int call_count = 0;
    auto fetch_page = [&call_count](const std::string& cursor) -> PaginatedResult<int> {
        call_count++;
        if (cursor.empty()) {
            return {{1, 2}, "page2", 5};
        } else if (cursor == "page2") {
            return {{3, 4}, "page3", 5};
        } else if (cursor == "page3") {
            return {{5}, std::nullopt, 5};
        }
        return {{}, std::nullopt, 0};
    };

    auto all_items = list_all<int>(fetch_page);
    EXPECT_EQ(all_items.size(), 5);
    EXPECT_EQ(call_count, 3);  // 3 pages fetched
}
```
  </action>
  <verify>File tests/unit/test_pagination.cpp exists with pagination helper tests</verify>
  <done>Pagination unit tests created covering PaginatedResult and list_all helper</done>
</task>

<task type="auto">
  <name>Build and run unit tests</name>
  <files>tests/CMakeLists.txt</files>
  <action>
Build and verify all unit tests:

1. Build tests:
   ```bash
   cd build
   cmake --build . --target mcpp_unit_tests
   ```

2. Run unit tests via CTest:
   ```bash
   ctest -L unit --output-on-failure
   ```

3. Run unit tests directly:
   ```bash
   ./tests/mcpp_unit_tests --gtest_filter="*"
   ```

4. Run specific test suites:
   ```bash
   ./tests/mcpp_unit_tests --gtest_filter="JsonRpc*"
   ./tests/mcpp_unit_tests --gtest_filter="ToolRegistry*"
   ./tests/mcpp_unit_tests --gtest_filter="ResourceRegistry*"
   ./tests/mcpp_unit_tests --gtest_filter="PromptRegistry*"
   ```

5. Verify test count:
   ```bash
   ./tests/mcpp_unit_tests --gtest_list_tests
   ```

Expected: At least 20 test cases covering JSON-RPC, registries, pagination, request tracking, and timeout management.

If any tests fail due to API mismatches with actual implementation, update the test code to match the actual API. The goal is to have all tests pass.

Do NOT commit - this is verification.
  </action>
  <verify>
Unit tests pass:
- ctest -L unit returns 0
- At least 20 test cases run
- Tests cover: JSON-RPC types, request tracker, timeout manager, registries, pagination
- No test failures or crashes
  </verify>
  <done>Complete unit test suite builds and runs successfully via CTest</done>
</task>

</tasks>

<verification>
After all tasks complete, verify:
1. tests/unit/test_json_rpc.cpp has comprehensive JSON-RPC tests (Request, Response, Error, Notification)
2. tests/unit/test_request_tracker.cpp has request tracker and timeout manager tests
3. Registry tests (tool, resource, prompt) exist with registration, listing, calling, unregister tests
4. tests/unit/test_pagination.cpp has PaginatedResult and list_all tests
5. All unit tests pass via ctest -L unit
6. TEST-01 (Google Test integration) requirement satisfied
</verification>

<success_criteria>
1. Unit test executable (mcpp_unit_tests) builds and links against mcpp library
2. gtest_discover_tests() automatically registers tests with CTest
3. Unit tests exist for all major modules (json_rpc, request_tracker, registries, pagination)
4. At least 20 test cases run and pass
5. Tests use common fixtures from tests/fixtures/common.h
</success_criteria>

<output>
After completion, create `.planning/phases/07-build-validation/07-02b-SUMMARY.md` with:
- Unit tests created per module
- Test coverage achieved (number of test cases)
- Any API mismatches found and fixed during implementation
- Test execution results
</output>
