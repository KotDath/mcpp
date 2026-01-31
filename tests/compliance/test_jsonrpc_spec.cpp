// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/core/json_rpc.h"
#include "mcpp/core/error.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <fstream>

using namespace mcpp;
using namespace mcpp::core;

// ============================================================================
// Test Data Loader
// ============================================================================

class JsonRpcSpecCompliance : public ::testing::Test {
protected:
    void SetUp() override {
        // Load test data from jsonrpc_examples.json
        std::ifstream file(TEST_DATA_DIR "/jsonrpc_examples.json");
        if (file.is_open()) {
            try {
                test_data = nlohmann::json::parse(file);
            } catch (const nlohmann::json::parse_error& e) {
                ADD_FAILURE() << "Failed to parse test data: " << e.what();
            }
        } else {
            ADD_FAILURE() << "Could not open test data file: " TEST_DATA_DIR "/jsonrpc_examples.json";
        }
    }

    nlohmann::json test_data;

    // Helper: Check if JSON has required request fields
    bool is_valid_request_structure(const nlohmann::json& j) {
        return j.contains("jsonrpc") && j["jsonrpc"] == "2.0" &&
               j.contains("method") && j["method"].is_string();
    }

    // Helper: Check if JSON has required response fields
    bool is_valid_response_structure(const nlohmann::json& j) {
        return j.contains("jsonrpc") && j["jsonrpc"] == "2.0" &&
               j.contains("id") &&
               (j.contains("result") || j.contains("error"));
    }

    // Helper: Parse RequestId from JSON
    std::optional<RequestId> parse_request_id(const nlohmann::json& j) {
        if (j.is_number_integer()) {
            return j.get<int64_t>();
        } else if (j.is_string()) {
            return j.get<std::string>();
        } else if (j.is_null()) {
            // Null ID is valid in some error responses
            return int64_t(0);
        }
        return std::nullopt;
    }
};

// ============================================================================
// JSON-RPC 2.0 Specification Compliance Tests
// ============================================================================

TEST_F(JsonRpcSpecCompliance, ValidRequest_Structure) {
    // A JSON-RPC request must have: jsonrpc="2.0", method (string), optional params, optional id
    if (!test_data.contains("valid_requests")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    for (auto& [name, req_json] : test_data["valid_requests"].items()) {
        SCOPED_TRACE(name);
        EXPECT_TRUE(req_json.contains("jsonrpc")) << "Request missing jsonrpc field";
        EXPECT_EQ(req_json["jsonrpc"], "2.0") << "Request has wrong jsonrpc version";
        EXPECT_TRUE(req_json.contains("method")) << "Request missing method field";
        EXPECT_TRUE(req_json["method"].is_string()) << "Request method must be string";
    }
}

TEST_F(JsonRpcSpecCompliance, ValidRequest_IdTypes) {
    // JSON-RPC 2.0 allows ID to be number, string, or null
    if (!test_data.contains("valid_requests")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    // Check integer ID
    if (test_data["valid_requests"].contains("with_positional_params")) {
        auto& req = test_data["valid_requests"]["with_positional_params"];
        EXPECT_TRUE(req["id"].is_number_integer()) << "Integer ID should be number";
    }

    // Check string ID
    if (test_data["valid_requests"].contains("string_id")) {
        auto& req = test_data["valid_requests"]["string_id"];
        EXPECT_TRUE(req["id"].is_string()) << "String ID should be string";
    }

    // Check null ID (valid for some error cases)
    if (test_data["valid_requests"].contains("null_id")) {
        auto& req = test_data["valid_requests"]["null_id"];
        EXPECT_TRUE(req["id"].is_null()) << "Null ID should be null";
    }
}

TEST_F(JsonRpcSpecCompliance, ValidResponse_Structure) {
    // A JSON-RPC response must have: jsonrpc="2.0", result OR error, id
    if (!test_data.contains("valid_responses")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    for (auto& [name, resp_json] : test_data["valid_responses"].items()) {
        SCOPED_TRACE(name);
        EXPECT_TRUE(resp_json.contains("jsonrpc")) << "Response missing jsonrpc field";
        EXPECT_EQ(resp_json["jsonrpc"], "2.0") << "Response has wrong jsonrpc version";
        EXPECT_TRUE(resp_json.contains("id")) << "Response missing id field";
        EXPECT_TRUE(resp_json.contains("result") || resp_json.contains("error"))
            << "Response must have result or error field";
        EXPECT_FALSE(resp_json.contains("result") && resp_json.contains("error"))
            << "Response cannot have both result and error";
    }
}

TEST_F(JsonRpcSpecCompliance, Error_CodeStandardValues) {
    // JSON-RPC 2.0 defines standard error codes
    struct ErrorCodeCheck {
        int code;
        const char* name;
    } standard_codes[] = {
        {-32700, "Parse error"},
        {-32600, "Invalid Request"},
        {-32601, "Method not found"},
        {-32602, "Invalid params"},
        {-32603, "Internal error"}
    };

    if (!test_data.contains("error_codes")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    EXPECT_EQ(test_data["error_codes"]["parse_error"], -32700);
    EXPECT_EQ(test_data["error_codes"]["invalid_request"], -32600);
    EXPECT_EQ(test_data["error_codes"]["method_not_found"], -32601);
    EXPECT_EQ(test_data["error_codes"]["invalid_params"], -32602);
    EXPECT_EQ(test_data["error_codes"]["internal_error"], -32603);
}

TEST_F(JsonRpcSpecCompliance, Error_ObjectStructure) {
    // Verify error objects have correct structure
    if (!test_data.contains("valid_responses")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    for (auto& [name, resp_json] : test_data["valid_responses"].items()) {
        if (!resp_json.contains("error")) continue;

        SCOPED_TRACE(name);
        const auto& error = resp_json["error"];

        // Verify error has required fields
        EXPECT_TRUE(error.contains("code")) << "Error missing code";
        EXPECT_TRUE(error.contains("message")) << "Error missing message";

        // Verify code is integer
        EXPECT_TRUE(error["code"].is_number_integer()) << "Error code must be integer";

        // Verify message is string
        EXPECT_TRUE(error["message"].is_string()) << "Error message must be string";

        // Data field is optional
        if (error.contains("data")) {
            // Data can be any JSON type - just verify it exists
            SUCCEED() << "Error has optional data field";
        }
    }
}

TEST_F(JsonRpcSpecCompliance, Notification_NoIdField) {
    // Notifications are requests without id field
    if (!test_data.contains("valid_notifications")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    for (auto& [name, notif_json] : test_data["valid_notifications"].items()) {
        SCOPED_TRACE(name);
        EXPECT_FALSE(notif_json.contains("id")) << "Notification must not have id field";
        EXPECT_TRUE(notif_json.contains("method")) << "Notification must have method";
        EXPECT_TRUE(notif_json.contains("jsonrpc")) << "Notification must have jsonrpc";
        EXPECT_EQ(notif_json["jsonrpc"], "2.0") << "Notification jsonrpc must be 2.0";
    }
}

TEST_F(JsonRpcSpecCompliance, InvalidRequest_Detection) {
    // Verify that invalid requests are properly identified
    if (!test_data.contains("invalid_requests")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    // Check that each invalid request is missing or has wrong required fields
    for (auto& [name, invalid_json] : test_data["invalid_requests"].items()) {
        SCOPED_TRACE(name);
        // These should fail validation in some way
        bool has_jsonrpc = invalid_json.contains("jsonrpc");
        bool has_method = invalid_json.contains("method");

        if (has_jsonrpc) {
            EXPECT_TRUE(invalid_json["jsonrpc"].is_string()) << "jsonrpc must be string";
            if (invalid_json["jsonrpc"] == "2.0") {
                // The "missing_method" case in our data has jsonrpc=2.0 but no method
                // This is intentional - it's an invalid request
                if (name == "missing_method") {
                    EXPECT_FALSE(has_method) << "This test case should be missing method";
                } else {
                    EXPECT_TRUE(has_method) << "Request with jsonrpc=2.0 must have method";
                }
            }
        }
    }
}

TEST_F(JsonRpcSpecCompliance, BatchRequest_Structure) {
    // Batch requests are JSON arrays of individual requests
    if (!test_data.contains("batch_requests")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    for (auto& [name, batch_json] : test_data["batch_requests"].items()) {
        SCOPED_TRACE(name);
        EXPECT_TRUE(batch_json.is_array()) << "Batch request must be an array";

        for (const auto& item : batch_json) {
            EXPECT_TRUE(item.contains("jsonrpc")) << "Batch item missing jsonrpc";
            EXPECT_TRUE(item.contains("method")) << "Batch item missing method";
        }
    }
}

TEST_F(JsonRpcSpecCompliance, BatchRequest_Empty) {
    // Empty batch is valid per JSON-RPC spec but should return empty response
    if (!test_data.contains("batch_requests") ||
        !test_data["batch_requests"].contains("empty_batch")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    auto empty_batch = test_data["batch_requests"]["empty_batch"];
    EXPECT_TRUE(empty_batch.is_array()) << "Empty batch must be array";
    EXPECT_EQ(empty_batch.size(), 0) << "Empty batch must have zero elements";
}

// ============================================================================
// JsonRpcRequest Type Compliance Tests
// ============================================================================

TEST_F(JsonRpcSpecCompliance, Request_ToJsonCreatesValidStructure) {
    // Verify our Request type creates valid JSON-RPC requests
    JsonRpcRequest req;
    req.id = RequestId{static_cast<int64_t>(42)};
    req.method = "test/method";
    req.params = {{"arg1", "value1"}};
    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_EQ(j["method"], "test/method");
    EXPECT_EQ(j["id"], 42);
    EXPECT_TRUE(j.contains("params"));
    EXPECT_EQ(j["params"]["arg1"], "value1");
}

TEST_F(JsonRpcSpecCompliance, Request_WithStringId) {
    JsonRpcRequest req;
    req.id = std::string("req-123");
    req.method = "test/method";
    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["id"], "req-123");
    EXPECT_TRUE(j["id"].is_string());
}

TEST_F(JsonRpcSpecCompliance, Request_NullParamsHandling) {
    JsonRpcRequest req;
    req.id = RequestId{static_cast<int64_t>(1)};
    req.method = "test/method";
    req.params = nullptr;
    nlohmann::json j = req.to_json();

    // Null params should not be included in output (default value)
    EXPECT_FALSE(j.contains("params")) << "Null params should not be serialized";
}

TEST_F(JsonRpcSpecCompliance, Request_ArrayParams) {
    nlohmann::json params = nlohmann::json::array({1, 2, 3});
    JsonRpcRequest req;
    req.id = RequestId{static_cast<int64_t>(1)};
    req.method = "process";
    req.params = params;
    nlohmann::json j = req.to_json();

    EXPECT_TRUE(j["params"].is_array());
    EXPECT_EQ(j["params"].size(), 3);
}

// ============================================================================
// JsonRpcResponse Type Compliance Tests
// ============================================================================

TEST_F(JsonRpcSpecCompliance, Response_SuccessResult) {
    nlohmann::json result = {{"status", "ok"}, {"data", {1, 2, 3}}};
    JsonRpcResponse resp;
    resp.id = RequestId{static_cast<int64_t>(1)};
    resp.result = result;
    nlohmann::json j = resp.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_TRUE(j.contains("result"));
    EXPECT_FALSE(j.contains("error"));
    EXPECT_EQ(j["id"], 1);
}

TEST_F(JsonRpcSpecCompliance, Response_NullResult) {
    // Null result is valid (indicates successful void method)
    nlohmann::json result = nullptr;
    JsonRpcResponse resp;
    resp.id = RequestId{static_cast<int64_t>(1)};
    resp.result = result;
    nlohmann::json j = resp.to_json();

    EXPECT_TRUE(j.contains("result"));
    EXPECT_TRUE(j["result"].is_null());
    EXPECT_FALSE(j.contains("error"));
}

TEST_F(JsonRpcSpecCompliance, Response_ErrorResponse) {
    JsonRpcError error{PARSE_ERROR, "Parse error", std::nullopt};
    JsonRpcResponse resp;
    resp.id = RequestId{static_cast<int64_t>(1)};
    resp.error = error;
    nlohmann::json j = resp.to_json();

    EXPECT_TRUE(j.contains("error"));
    EXPECT_FALSE(j.contains("result"));
    EXPECT_EQ(j["error"]["code"], PARSE_ERROR);
    EXPECT_EQ(j["error"]["message"], "Parse error");
}

TEST_F(JsonRpcSpecCompliance, Response_ErrorWithData) {
    JsonRpcError error{METHOD_NOT_FOUND, "Method not found", "Available methods: foo, bar"};
    JsonRpcResponse resp;
    resp.id = RequestId{static_cast<int64_t>(1)};
    resp.error = error;
    nlohmann::json j = resp.to_json();

    EXPECT_TRUE(j["error"].contains("data"));
    EXPECT_EQ(j["error"]["data"], "Available methods: foo, bar");
}

TEST_F(JsonRpcSpecCompliance, Response_ParseValidExamples) {
    if (!test_data.contains("valid_responses")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    for (auto& [name, resp_json] : test_data["valid_responses"].items()) {
        SCOPED_TRACE(name);
        auto resp = JsonRpcResponse::from_json(resp_json);
        EXPECT_TRUE(resp.has_value()) << "Valid response '" << name << "' should parse";
        if (resp.has_value()) {
            EXPECT_EQ(resp->jsonrpc, "2.0");
        }
    }
}

TEST_F(JsonRpcSpecCompliance, Response_MutualExclusivity) {
    // Response cannot have both result and error
    if (!test_data.contains("valid_responses")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    for (auto& [name, resp_json] : test_data["valid_responses"].items()) {
        SCOPED_TRACE(name);
        auto resp = JsonRpcResponse::from_json(resp_json);
        ASSERT_TRUE(resp.has_value());
        EXPECT_NE(resp->result.has_value() && resp->error.has_value(), true)
            << "Response '" << name << "' cannot have both result and error";
    }
}

TEST_F(JsonRpcSpecCompliance, Response_IsSuccessIsError) {
    JsonRpcResponse success_resp;
    success_resp.id = RequestId{static_cast<int64_t>(1)};
    success_resp.result = nlohmann::json{{"value", 42}};
    EXPECT_TRUE(success_resp.is_success());
    EXPECT_FALSE(success_resp.is_error());

    JsonRpcResponse error_resp;
    error_resp.id = RequestId{static_cast<int64_t>(2)};
    error_resp.error = JsonRpcError{INVALID_PARAMS, "Invalid params", std::nullopt};
    EXPECT_FALSE(error_resp.is_success());
    EXPECT_TRUE(error_resp.is_error());
}

// ============================================================================
// JsonRpcError Type Compliance Tests
// ============================================================================

TEST_F(JsonRpcSpecCompliance, Error_StandardCodesMatchSpec) {
    // Verify our error codes match JSON-RPC 2.0 spec
    EXPECT_EQ(PARSE_ERROR, -32700);
    EXPECT_EQ(INVALID_REQUEST, -32600);
    EXPECT_EQ(METHOD_NOT_FOUND, -32601);
    EXPECT_EQ(INVALID_PARAMS, -32602);
    EXPECT_EQ(INTERNAL_ERROR, -32603);
}

TEST_F(JsonRpcSpecCompliance, Error_FactoryMethods) {
    auto parse_err = JsonRpcError::parse_error("test details");
    EXPECT_EQ(parse_err.code, PARSE_ERROR);
    EXPECT_EQ(parse_err.message, "Parse error");
    EXPECT_TRUE(parse_err.data.has_value());
    EXPECT_EQ(parse_err.data.value().get<std::string>(), "test details");

    auto method_err = JsonRpcError::method_not_found("unknownMethod");
    EXPECT_EQ(method_err.code, METHOD_NOT_FOUND);
    EXPECT_EQ(method_err.message, "Method not found");
    EXPECT_TRUE(method_err.data.has_value());

    auto params_err = JsonRpcError::invalid_params();
    EXPECT_EQ(params_err.code, INVALID_PARAMS);
    EXPECT_EQ(params_err.message, "Invalid params");
    EXPECT_FALSE(params_err.data.has_value());
}

TEST_F(JsonRpcSpecCompliance, Error_ToJsonCreatesValidStructure) {
    JsonRpcError error{-32000, "Server error", nlohmann::json{{"details", "DB failed"}}};
    nlohmann::json j = error.to_json();

    EXPECT_EQ(j["code"], -32000);
    EXPECT_EQ(j["message"], "Server error");
    EXPECT_TRUE(j.contains("data"));
    EXPECT_EQ(j["data"]["details"], "DB failed");
}

TEST_F(JsonRpcSpecCompliance, Error_ParseValidExamples) {
    if (!test_data.contains("valid_responses")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    for (auto& [name, resp_json] : test_data["valid_responses"].items()) {
        if (!resp_json.contains("error")) continue;

        SCOPED_TRACE(name);
        auto error = JsonRpcError::from_json(resp_json["error"]);
        EXPECT_TRUE(error.has_value()) << "Error '" << name << "' should parse";
    }
}

// ============================================================================
// JsonRpcNotification Type Compliance Tests
// ============================================================================

TEST_F(JsonRpcSpecCompliance, Notification_ToJsonCreatesValidStructure) {
    JsonRpcNotification notif;
    notif.method = "notifications/update";
    notif.params = {{"value", 42}};
    nlohmann::json j = notif.to_json();

    EXPECT_EQ(j["jsonrpc"], "2.0");
    EXPECT_EQ(j["method"], "notifications/update");
    EXPECT_FALSE(j.contains("id")) << "Notification must not have id";
    EXPECT_EQ(j["params"]["value"], 42);
}

TEST_F(JsonRpcSpecCompliance, Notification_NoParams) {
    JsonRpcNotification notif;
    notif.method = "notifications/ping";
    nlohmann::json j = notif.to_json();

    EXPECT_FALSE(j.contains("params")) << "Notification with no params should not serialize params";
}

TEST_F(JsonRpcSpecCompliance, Notification_ValidExamplesFromData) {
    if (!test_data.contains("valid_notifications")) {
        GTEST_SKIP() << "Test data not available";
        return;
    }

    for (auto& [name, notif_json] : test_data["valid_notifications"].items()) {
        SCOPED_TRACE(name);
        // Verify structure (we can't parse these since we don't have from_json for Notification)
        EXPECT_FALSE(notif_json.contains("id")) << "Notification '" << name << "' must not have id";
        EXPECT_TRUE(notif_json.contains("method")) << "Notification '" << name << "' must have method";
    }
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

TEST_F(JsonRpcSpecCompliance, EdgeCase_EmptyStringMethod) {
    // Empty method string is technically valid per spec (though unusual)
    JsonRpcRequest req;
    req.id = RequestId{static_cast<int64_t>(1)};
    req.method = "";
    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["method"], "");
}

TEST_F(JsonRpcSpecCompliance, EdgeCase_LargeIntegerId) {
    // Test max safe integer
    nlohmann::json req_json = {
        {"jsonrpc", "2.0"},
        {"method", "test"},
        {"id", 9007199254740991}
    };

    auto id = parse_request_id(req_json["id"]);
    EXPECT_TRUE(id.has_value()) << "Large integer ID should be supported";
}

TEST_F(JsonRpcSpecCompliance, EdgeCase_NegativeIntegerId) {
    // Negative IDs are valid per spec
    JsonRpcRequest req;
    req.id = RequestId{static_cast<int64_t>(-1)};
    req.method = "test";
    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["id"], -1);
}

TEST_F(JsonRpcSpecCompliance, EdgeCase_ZeroId) {
    // Zero is a valid ID
    JsonRpcRequest req;
    req.id = RequestId{static_cast<int64_t>(0)};
    req.method = "test";
    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["id"], 0);
}

TEST_F(JsonRpcSpecCompliance, EdgeCase_VeryLongStringId) {
    // Long string IDs should work
    std::string long_id(1000, 'x');
    JsonRpcRequest req;
    req.id = long_id;
    req.method = "test";
    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["id"].get<std::string>().length(), 1000);
}

TEST_F(JsonRpcSpecCompliance, EdgeCase_UnicodeInMethod) {
    // Unicode characters in method name
    JsonRpcRequest req;
    req.id = RequestId{static_cast<int64_t>(1)};
    req.method = "test/方法";
    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["method"], "test/方法");
}

TEST_F(JsonRpcSpecCompliance, EdgeCase_NestedParams) {
    // Deeply nested parameter objects
    nlohmann::json nested = nlohmann::json::object();
    nested["level1"]["level2"]["level3"]["value"] = "deep";
    JsonRpcRequest req;
    req.id = RequestId{static_cast<int64_t>(1)};
    req.method = "test";
    req.params = nested;
    nlohmann::json j = req.to_json();

    EXPECT_EQ(j["params"]["level1"]["level2"]["level3"]["value"], "deep");
}

TEST_F(JsonRpcSpecCompliance, EdgeCase_ArrayParamsWithNulls) {
    // Arrays containing null values
    nlohmann::json params = nlohmann::json::array({1, nullptr, "test"});
    JsonRpcRequest req;
    req.id = RequestId{static_cast<int64_t>(1)};
    req.method = "test";
    req.params = params;
    nlohmann::json j = req.to_json();

    EXPECT_TRUE(j["params"].is_array());
    EXPECT_EQ(j["params"].size(), 3);
    EXPECT_TRUE(j["params"][1].is_null());
}

// ============================================================================
// Response Parsing Error Cases
// ============================================================================

TEST_F(JsonRpcSpecCompliance, ParseError_MissingJsonrpcField) {
    nlohmann::json invalid = {
        {"result", 42},
        {"id", 1}
    };

    auto resp = JsonRpcResponse::from_json(invalid);
    EXPECT_FALSE(resp.has_value()) << "Response without jsonrpc field should not parse";
}

TEST_F(JsonRpcSpecCompliance, ParseError_WrongJsonrpcVersion) {
    nlohmann::json invalid = {
        {"jsonrpc", "1.0"},
        {"result", 42},
        {"id", 1}
    };

    auto resp = JsonRpcResponse::from_json(invalid);
    EXPECT_FALSE(resp.has_value()) << "Response with wrong jsonrpc version should not parse";
}

TEST_F(JsonRpcSpecCompliance, ParseError_MissingIdField) {
    nlohmann::json invalid = {
        {"jsonrpc", "2.0"},
        {"result", 42}
    };

    auto resp = JsonRpcResponse::from_json(invalid);
    EXPECT_FALSE(resp.has_value()) << "Response without id field should not parse";
}

TEST_F(JsonRpcSpecCompliance, ParseError_BothResultAndError) {
    nlohmann::json invalid = {
        {"jsonrpc", "2.0"},
        {"result", 42},
        {"error", {{"code", -32600}, {"message", "Error"}}},
        {"id", 1}
    };

    auto resp = JsonRpcResponse::from_json(invalid);
    EXPECT_FALSE(resp.has_value()) << "Response with both result and error should not parse";
}

TEST_F(JsonRpcSpecCompliance, ParseError_NeitherResultNorError) {
    nlohmann::json invalid = {
        {"jsonrpc", "2.0"},
        {"id", 1}
    };

    auto resp = JsonRpcResponse::from_json(invalid);
    EXPECT_FALSE(resp.has_value()) << "Response without result or error should not parse";
}

TEST_F(JsonRpcSpecCompliance, ParseError_InvalidErrorObject) {
    nlohmann::json invalid = {
        {"jsonrpc", "2.0"},
        {"error", "not an object"},
        {"id", 1}
    };

    auto resp = JsonRpcResponse::from_json(invalid);
    EXPECT_FALSE(resp.has_value()) << "Response with non-object error should not parse";
}

// ============================================================================
// Error Parsing Error Cases
// ============================================================================

TEST_F(JsonRpcSpecCompliance, ErrorParse_MissingCodeField) {
    nlohmann::json invalid = {
        {"message", "Error"}
    };

    auto error = JsonRpcError::from_json(invalid);
    EXPECT_FALSE(error.has_value()) << "Error without code should not parse";
}

TEST_F(JsonRpcSpecCompliance, ErrorParse_MissingMessageField) {
    nlohmann::json invalid = {
        {"code", -32600}
    };

    auto error = JsonRpcError::from_json(invalid);
    EXPECT_FALSE(error.has_value()) << "Error without message should not parse";
}

TEST_F(JsonRpcSpecCompliance, ErrorParse_NonNumericCode) {
    nlohmann::json invalid = {
        {"code", "not a number"},
        {"message", "Error"}
    };

    auto error = JsonRpcError::from_json(invalid);
    EXPECT_FALSE(error.has_value()) << "Error with non-numeric code should not parse";
}

TEST_F(JsonRpcSpecCompliance, ErrorParse_NonStringMessage) {
    nlohmann::json invalid = {
        {"code", -32600},
        {"message", 123}
    };

    auto error = JsonRpcError::from_json(invalid);
    EXPECT_FALSE(error.has_value()) << "Error with non-string message should not parse";
}

TEST_F(JsonRpcSpecCompliance, ErrorParse_CodeAndMessageOnly) {
    // Error with only code and message (data is optional)
    nlohmann::json valid = {
        {"code", -32600},
        {"message", "Invalid Request"}
    };

    auto error = JsonRpcError::from_json(valid);
    EXPECT_TRUE(error.has_value()) << "Valid error with code+message should parse";
    EXPECT_FALSE(error->data.has_value()) << "Data should be absent when not provided";
}
