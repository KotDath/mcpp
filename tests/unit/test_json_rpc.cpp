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
    EXPECT_EQ(req.jsonrpc, "2.0");
    EXPECT_EQ(req.method, "");
    EXPECT_TRUE(req.params.is_null());
}

TEST(JsonRpcRequest, ParameterizedConstruction_WithId) {
    JsonRpcRequest req;
    req.method = "test/method";
    req.params = nlohmann::json::object();
    req.id = int64_t{42};

    EXPECT_EQ(req.jsonrpc, "2.0");
    EXPECT_EQ(req.method, "test/method");
    EXPECT_TRUE(req.params.is_object());
    EXPECT_EQ(std::get<int64_t>(req.id), 42);
}

TEST(JsonRpcRequest, ParameterizedConstruction_WithStringId) {
    JsonRpcRequest req;
    req.method = "test/method";
    req.params = nlohmann::json::object();
    req.id = std::string("req-123");

    EXPECT_EQ(req.method, "test/method");
    EXPECT_EQ(std::get<std::string>(req.id), "req-123");
}

TEST(JsonRpcRequest, ToJson_ValidRequest) {
    JsonRpcRequest req;
    req.method = "test/method";
    req.params = {{"key", "value"}};
    req.id = int64_t{1};

    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_EQ(j["method"], "test/method");
    EXPECT_EQ(j["params"]["key"], "value");
    EXPECT_EQ(j["id"], 1);
}

TEST(JsonRpcRequest, ToJson_WithNullParams) {
    JsonRpcRequest req;
    req.method = "method/no_params";
    req.params = nullptr;
    req.id = int64_t{1};

    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["method"], "method/no_params");
    EXPECT_FALSE(j.contains("params"));  // null params not included
}

TEST(JsonRpcRequest, ToJson_WithArrayParams) {
    JsonRpcRequest req;
    req.method = "subtract";
    req.params = nlohmann::json::array({42, 23});
    req.id = int64_t{1};

    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["method"], "subtract");
    EXPECT_TRUE(j["params"].is_array());
    EXPECT_EQ(j["params"][0], 42);
    EXPECT_EQ(j["params"][1], 23);
}

TEST(JsonRpcRequest, ToJson_StringId) {
    JsonRpcRequest req;
    req.method = "test/method";
    req.params = nlohmann::json::object();
    req.id = std::string("abc");

    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["id"], "abc");
}

TEST(JsonRpcRequest, ToString_SerializesCorrectly) {
    JsonRpcRequest req;
    req.method = "test/method";
    req.params = {{"arg", "value"}};
    req.id = int64_t{1};

    std::string str = req.to_string();

    EXPECT_TRUE(str.find("\"jsonrpc\":\"2.0\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"method\":\"test/method\"") != std::string::npos);
}

// ============================================================================
// JsonRpcResponse Tests
// ============================================================================

TEST(JsonRpcResponse, SuccessResult) {
    nlohmann::json result = {{"status", "ok"}};
    JsonRpcResponse resp;
    resp.result = result;
    resp.id = 1;

    EXPECT_FALSE(resp.is_error());
    EXPECT_TRUE(resp.is_success());
    EXPECT_EQ((*resp.result)["status"], "ok");
}

TEST(JsonRpcResponse, ErrorResult) {
    JsonRpcError error(INVALID_PARAMS, "Invalid parameters");
    JsonRpcResponse resp;
    resp.error = error;
    resp.id = 2;

    EXPECT_TRUE(resp.is_error());
    EXPECT_FALSE(resp.is_success());
    EXPECT_EQ((*resp.error).code, -32602);
    EXPECT_EQ((*resp.error).message, "Invalid parameters");
}

TEST(JsonRpcResponse, ToJson_SuccessResponse) {
    nlohmann::json result = {{"data", "value"}};
    JsonRpcResponse resp;
    resp.result = result;
    resp.id = 42;
    nlohmann::json j = resp.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_EQ(j["result"]["data"], "value");
    EXPECT_EQ(j["id"], 42);
    EXPECT_FALSE(j.contains("error"));
}

TEST(JsonRpcResponse, ToJson_ErrorResponse) {
    JsonRpcError error(METHOD_NOT_FOUND, "Method not found");
    error.data = 42;
    JsonRpcResponse resp;
    resp.error = error;
    resp.id = 1;
    nlohmann::json j = resp.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_TRUE(j.contains("error"));
    EXPECT_EQ(j["error"]["code"], -32601);
    EXPECT_EQ(j["error"]["message"], "Method not found");
    EXPECT_EQ(j["error"]["data"], 42);
    EXPECT_EQ(j["id"], 1);
    EXPECT_FALSE(j.contains("result"));
}

TEST(JsonRpcResponse, ToString_SerializesCorrectly) {
    nlohmann::json result = {{"answer", 42}};
    JsonRpcResponse resp;
    resp.result = result;
    resp.id = 1;

    std::string str = resp.to_string();
    EXPECT_TRUE(str.find("\"result\":") != std::string::npos);
    EXPECT_TRUE(str.find("\"answer\":42") != std::string::npos);
}

// ============================================================================
// JsonRpcError Tests
// ============================================================================

TEST(JsonRpcError, StandardErrorCodes) {
    JsonRpcError parse_err(core::PARSE_ERROR, "Parse error");
    EXPECT_EQ(parse_err.code, -32700);

    JsonRpcError req_err(core::INVALID_REQUEST, "Invalid request");
    EXPECT_EQ(req_err.code, -32600);

    JsonRpcError method_err(core::METHOD_NOT_FOUND, "Method not found");
    EXPECT_EQ(method_err.code, -32601);

    JsonRpcError params_err(core::INVALID_PARAMS, "Invalid params");
    EXPECT_EQ(params_err.code, -32602);

    JsonRpcError internal_err(core::INTERNAL_ERROR, "Internal error");
    EXPECT_EQ(internal_err.code, -32603);
}

TEST(JsonRpcError, FactoryMethods) {
    auto parse_err = JsonRpcError::parse_error("Failed to parse JSON");
    EXPECT_EQ(parse_err.code, -32700);
    EXPECT_EQ(parse_err.message, "Parse error");
    EXPECT_TRUE(parse_err.data.has_value());
    EXPECT_EQ(parse_err.data.value(), "Failed to parse JSON");

    auto invalid_req = JsonRpcError::invalid_request("Malformed JSON-RPC");
    EXPECT_EQ(invalid_req.code, -32600);
    EXPECT_EQ(invalid_req.message, "Invalid Request");

    auto method_not_found = JsonRpcError::method_not_found("unknown_method");
    EXPECT_EQ(method_not_found.code, -32601);
    EXPECT_TRUE(method_not_found.data.has_value());

    auto invalid_params = JsonRpcError::invalid_params("Missing required field");
    EXPECT_EQ(invalid_params.code, -32602);
    EXPECT_EQ(invalid_params.message, "Invalid params");

    auto internal_err = JsonRpcError::internal_error("Something went wrong");
    EXPECT_EQ(internal_err.code, -32603);
    EXPECT_EQ(internal_err.message, "Internal error");
}

TEST(JsonRpcError, ToJson_SerializesCorrectly) {
    JsonRpcError err(core::METHOD_NOT_FOUND, "Method not found");
    err.data = nlohmann::json{{"available", std::vector<std::string>{"foo", "bar"}}};

    nlohmann::json j = err.to_json();

    EXPECT_EQ(j["code"], -32601);
    EXPECT_EQ(j["message"], "Method not found");
    EXPECT_TRUE(j.contains("data"));
    EXPECT_EQ(j["data"]["available"][0], "foo");
}

TEST(JsonRpcError, ToString_SerializesCorrectly) {
    JsonRpcError err(core::INVALID_PARAMS, "Bad params");
    std::string str = err.to_string();

    EXPECT_TRUE(str.find("\"code\":-32602") != std::string::npos);
    EXPECT_TRUE(str.find("\"message\":\"Bad params\"") != std::string::npos);
}

TEST(JsonRpcError, FactoryWithoutDetails) {
    auto err = JsonRpcError::parse_error();
    EXPECT_EQ(err.code, -32700);
    EXPECT_FALSE(err.data.has_value());
}

// ============================================================================
// JsonRpcNotification Tests
// ============================================================================

TEST(JsonRpcNotification, Construction) {
    JsonRpcNotification notif;
    notif.method = "notifications/message";
    notif.params = {{"content", "hello"}};

    EXPECT_EQ(notif.jsonrpc, "2.0");
    EXPECT_EQ(notif.method, "notifications/message");
    EXPECT_EQ(notif.params["content"], "hello");
}

TEST(JsonRpcNotification, Construction_WithNullParams) {
    JsonRpcNotification notif;
    notif.method = "test/notification";
    notif.params = nullptr;

    EXPECT_EQ(notif.method, "test/notification");
    EXPECT_TRUE(notif.params.is_null());
}

TEST(JsonRpcNotification, ToJson) {
    JsonRpcNotification notif;
    notif.method = "test/notification";
    notif.params = nlohmann::json::object();

    nlohmann::json j = notif.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_EQ(j["method"], "test/notification");
    EXPECT_FALSE(j.contains("id"));  // Notifications don't have IDs
}

TEST(JsonRpcNotification, ToJson_WithParams) {
    JsonRpcNotification notif;
    notif.method = "notifications/cancelled";
    notif.params = {{"reason", "user aborted"}, {"requestId", 42}};

    nlohmann::json j = notif.to_json();

    EXPECT_EQ(j["method"], "notifications/cancelled");
    EXPECT_EQ(j["params"]["reason"], "user aborted");
    EXPECT_EQ(j["params"]["requestId"], 42);
    EXPECT_FALSE(j.contains("id"));
}

TEST(JsonRpcNotification, ToString_SerializesCorrectly) {
    JsonRpcNotification notif;
    notif.method = "notify/update";
    notif.params = {{"value", 100}};

    std::string str = notif.to_string();

    EXPECT_TRUE(str.find("\"jsonrpc\":\"2.0\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"method\":\"notify/update\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"value\":100") != std::string::npos);
}

// ============================================================================
// RequestId Variant Tests
// ============================================================================

TEST(RequestId, IntId_Visit) {
    RequestId id = 42;
    std::string result = std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int64_t>) {
            return "int:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "string:" + v;
        }
        return "unknown";
    }, id);

    EXPECT_EQ(result, "int:42");
}

TEST(RequestId, StringId_Visit) {
    RequestId id = std::string("abc-123");
    std::string result = std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int64_t>) {
            return "int:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "string:" + v;
        }
        return "unknown";
    }, id);

    EXPECT_EQ(result, "string:abc-123");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(JsonRpcIntegration, RequestResponseRoundTrip) {
    // Create a request
    JsonRpcRequest req;
    req.method = "tools/call";
    req.params = {{"name", "echo"}, {"arguments", {{"message", "hello"}}}};
    req.id = int64_t{1};
    nlohmann::json req_json = req.to_json();

    // Simulate a response
    nlohmann::json result = {
        {"content", {{{"type", "text"}, {"text", "hello"}}}},
        {"isError", false}
    };
    JsonRpcResponse resp;
    resp.result = result;
    resp.id = req.id;
    nlohmann::json resp_json = resp.to_json();

    // Verify response matches request ID
    EXPECT_EQ(req_json["id"], resp_json["id"]);
    EXPECT_TRUE(resp_json.contains("result"));
    EXPECT_FALSE(resp_json.contains("error"));
}

TEST(JsonRpcIntegration, ErrorResponseRoundTrip) {
    // Create a request
    JsonRpcRequest req;
    req.method = "unknown/method";
    req.params = nullptr;
    req.id = int64_t{99};

    // Simulate an error response
    JsonRpcResponse resp;
    resp.error = JsonRpcError::method_not_found("unknown/method");
    resp.id = req.id;
    nlohmann::json resp_json = resp.to_json();

    EXPECT_EQ(resp_json["id"], 99);
    EXPECT_TRUE(resp_json.contains("error"));
    EXPECT_EQ(resp_json["error"]["code"], -32601);
    EXPECT_FALSE(resp_json.contains("result"));
}

TEST(JsonRpcIntegration, NotificationNoId) {
    JsonRpcNotification notif;
    notif.method = "notifications/progress";
    notif.params = {{"progress", 50.0}, {"message", "Processing..."}};

    nlohmann::json j = notif.to_json();

    EXPECT_TRUE(j.contains("method"));
    EXPECT_FALSE(j.contains("id"));  // Notifications must not have id
    EXPECT_EQ(j["params"]["progress"], 50.0);
}
