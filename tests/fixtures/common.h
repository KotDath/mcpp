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
