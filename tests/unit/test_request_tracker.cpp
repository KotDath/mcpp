// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/core/request_tracker.h"
#include "mcpp/async/timeout.h"
#include "fixtures/common.h"

#include <gtest/gtest.h>
#include <functional>
#include <thread>
#include <chrono>
#include <atomic>

using namespace mcpp;
using namespace mcpp::core;
using namespace mcpp::async;
using namespace mcpp::test;

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

TEST_F(RequestTrackerTest, NextId_Increments) {
    auto id1 = tracker->next_id();
    auto id2 = tracker->next_id();

    EXPECT_NE(id1, id2);
    EXPECT_GT(tracker->next_id(), id2);
}

TEST_F(RequestTrackerTest, NextId_ReturnsInt64) {
    auto id = tracker->next_id();
    ASSERT_TRUE(std::holds_alternative<int64_t>(id));
    EXPECT_GE(std::get<int64_t>(id), 0);
}

TEST_F(RequestTrackerTest, RegisterPending_CallbackStored) {
    bool called = false;
    nlohmann::json received_result;

    ResponseCallback on_success = [&called, &received_result](const nlohmann::json& result) {
        called = true;
        received_result = result;
    };

    std::function<void(const JsonRpcError&)> on_error = [](const JsonRpcError&) {
        // Error handler
    };

    RequestId id = 42;
    tracker->register_pending(id, on_success, on_error);

    // Verify callback is stored by completing the request
    auto pending = tracker->complete(id);
    ASSERT_TRUE(pending.has_value());

    // Invoke the callback
    nlohmann::json test_result = {{"data", "test_value"}};
    pending->on_success(test_result);

    EXPECT_TRUE(called);
    EXPECT_EQ(received_result["data"], "test_value");
}

TEST_F(RequestTrackerTest, Unregister_RemovesPendingRequest) {
    bool called = false;
    nlohmann::json received_result;

    ResponseCallback on_success = [&called, &received_result](const nlohmann::json& result) {
        called = true;
        received_result = result;
    };

    std::function<void(const JsonRpcError&)> on_error = [](const JsonRpcError&) {};

    RequestId id = 1;
    tracker->register_pending(id, on_success, on_error);

    // Cancel the request
    tracker->cancel(id);

    // After cancel, should not be able to complete
    auto pending = tracker->complete(id);
    EXPECT_FALSE(pending.has_value());

    // Callback should not have been called
    EXPECT_FALSE(called);
}

TEST_F(RequestTrackerTest, Complete_InvokesCallback) {
    bool success_called = false;
    bool error_called = false;
    nlohmann::json received_result;

    ResponseCallback on_success = [&success_called, &received_result](const nlohmann::json& result) {
        success_called = true;
        received_result = result;
    };

    std::function<void(const JsonRpcError&)> on_error = [&error_called](const JsonRpcError&) {
        error_called = true;
    };

    RequestId id = 99;
    tracker->register_pending(id, on_success, on_error);

    // Complete the request
    auto pending = tracker->complete(id);
    ASSERT_TRUE(pending.has_value());

    // Invoke success callback
    nlohmann::json test_result = {{"status", "ok"}, {"value", 42}};
    pending->on_success(test_result);

    EXPECT_TRUE(success_called);
    EXPECT_FALSE(error_called);
    EXPECT_EQ(received_result["value"], 42);
}

TEST_F(RequestTrackerTest, Complete_NonExistentId_ReturnsNullopt) {
    auto pending = tracker->complete(999);
    EXPECT_FALSE(pending.has_value());
}

TEST_F(RequestTrackerTest, Complete_MultipleRequests_TrackedSeparately) {
    bool cb1_called = false, cb2_called = false;
    nlohmann::json result1, result2;

    ResponseCallback on_success1 = [&cb1_called, &result1](const nlohmann::json& r) {
        cb1_called = true;
        result1 = r;
    };

    ResponseCallback on_success2 = [&cb2_called, &result2](const nlohmann::json& r) {
        cb2_called = true;
        result2 = r;
    };

    std::function<void(const JsonRpcError&)> on_error = [](const JsonRpcError&) {};

    tracker->register_pending(1, on_success1, on_error);
    tracker->register_pending(2, on_success2, on_error);

    // Complete first request
    auto pending1 = tracker->complete(1);
    ASSERT_TRUE(pending1.has_value());
    pending1->on_success({{"result", "first"}});

    EXPECT_TRUE(cb1_called);
    EXPECT_FALSE(cb2_called);

    // Complete second request
    auto pending2 = tracker->complete(2);
    ASSERT_TRUE(pending2.has_value());
    pending2->on_success({{"result", "second"}});

    EXPECT_TRUE(cb2_called);
    EXPECT_EQ(result1["result"], "first");
    EXPECT_EQ(result2["result"], "second");
}

TEST_F(RequestTrackerTest, PendingCount_Accurate) {
    std::function<void(const nlohmann::json&)> on_success = [](const nlohmann::json&) {};
    std::function<void(const JsonRpcError&)> on_error = [](const JsonRpcError&) {};

    EXPECT_EQ(tracker->pending_count(), 0);

    tracker->register_pending(1, on_success, on_error);
    EXPECT_EQ(tracker->pending_count(), 1);

    tracker->register_pending(2, on_success, on_error);
    EXPECT_EQ(tracker->pending_count(), 2);

    tracker->complete(1);
    EXPECT_EQ(tracker->pending_count(), 1);

    tracker->cancel(2);
    EXPECT_EQ(tracker->pending_count(), 0);
}

TEST_F(RequestTrackerTest, Cancel_NonExistentId_NoEffect) {
    // Should not throw or crash
    EXPECT_NO_THROW(tracker->cancel(999));
    EXPECT_EQ(tracker->pending_count(), 0);
}

TEST_F(RequestTrackerTest, ErrorCallback_StoredCorrectly) {
    bool error_called = false;
    int error_code = 0;

    ResponseCallback on_success = [](const nlohmann::json&) {};
    std::function<void(const JsonRpcError&)> on_error = [&error_called, &error_code](const JsonRpcError& e) {
        error_called = true;
        error_code = e.code;
    };

    tracker->register_pending(1, on_success, on_error);

    auto pending = tracker->complete(1);
    ASSERT_TRUE(pending.has_value());

    // Invoke error callback
    pending->on_error(JsonRpcError::method_not_found("test_method"));

    EXPECT_TRUE(error_called);
    EXPECT_EQ(error_code, -32601);
}

TEST_F(RequestTrackerTest, StringId_Supported) {
    bool called = false;
    ResponseCallback on_success = [&called](const nlohmann::json&) { called = true; };
    std::function<void(const JsonRpcError&)> on_error = [](const JsonRpcError&) {};

    RequestId id = std::string("req-abc-123");
    tracker->register_pending(id, on_success, on_error);

    auto pending = tracker->complete(id);
    ASSERT_TRUE(pending.has_value());
    pending->on_success({{"result", "ok"}});

    EXPECT_TRUE(called);
}

// ============================================================================
// TimeoutManager Tests
// ============================================================================

class TimeoutManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a short default timeout for tests
        mgr = std::make_unique<TimeoutManager>(std::chrono::milliseconds(100));
    }

    std::unique_ptr<TimeoutManager> mgr;
};

TEST_F(TimeoutManagerTest, SetTimeout_RequestTracked) {
    RequestId id = 1;
    std::chrono::milliseconds timeout(50);
    bool timeout_called = false;

    mgr->set_timeout(id, timeout, [&timeout_called](RequestId) {
        timeout_called = true;
    });

    EXPECT_TRUE(mgr->has_timeout(id));
    EXPECT_EQ(mgr->pending_count(), 1);
}

TEST_F(TimeoutManagerTest, SetTimeout_ThenCancel_NotExpired) {
    RequestId id = 1;
    std::chrono::milliseconds timeout(100);
    bool timeout_called = false;

    mgr->set_timeout(id, timeout, [&timeout_called](RequestId) {
        timeout_called = true;
    });

    mgr->cancel(id);

    EXPECT_FALSE(mgr->has_timeout(id));
    EXPECT_EQ(mgr->pending_count(), 0);

    // Check timeouts - should not invoke callback
    auto expired = mgr->check_timeouts();
    EXPECT_EQ(expired.size(), 0);
    EXPECT_FALSE(timeout_called);
}

TEST_F(TimeoutManagerTest, Cancel_NonExistentId_NoEffect) {
    EXPECT_NO_THROW(mgr->cancel(999));
    EXPECT_EQ(mgr->pending_count(), 0);
}

TEST_F(TimeoutManagerTest, CheckTimeouts_BeforeTimeout_NoExpiry) {
    RequestId id = 1;
    std::chrono::milliseconds timeout(1000);
    bool timeout_called = false;

    mgr->set_timeout(id, timeout, [&timeout_called](RequestId) {
        timeout_called = true;
    });

    // Check immediately, should not be expired
    auto expired = mgr->check_timeouts();
    EXPECT_EQ(expired.size(), 0);
    EXPECT_FALSE(timeout_called);
}

TEST_F(TimeoutManagerTest, CheckTimeouts_AfterTimeout_ReturnsExpiredId) {
    RequestId id = 1;
    std::chrono::milliseconds timeout(30);
    bool timeout_called = false;

    mgr->set_timeout(id, timeout, [&timeout_called](RequestId expired_id) {
        timeout_called = true;
        EXPECT_EQ(expired_id, id);
    });

    // Wait for timeout to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto expired = mgr->check_timeouts();
    EXPECT_GE(expired.size(), 1);
    EXPECT_TRUE(timeout_called);
}

TEST_F(TimeoutManagerTest, CheckTimeouts_MultipleRequests_SomeExpired) {
    std::chrono::milliseconds short_timeout(30);
    std::chrono::milliseconds long_timeout(1000);

    std::atomic<int> timeout_count{0};

    // Register requests with different timeouts
    mgr->set_timeout(1, short_timeout, [&timeout_count](RequestId) { timeout_count++; });
    mgr->set_timeout(2, long_timeout, [&timeout_count](RequestId) { timeout_count++; });
    mgr->set_timeout(3, short_timeout, [&timeout_count](RequestId) { timeout_count++; });

    // Wait for short timeout to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto expired = mgr->check_timeouts();
    EXPECT_GE(expired.size(), 2);  // At least requests 1 and 3
    EXPECT_GE(timeout_count, 2);
}

TEST_F(TimeoutManagerTest, HasTimeout_ChecksCorrectly) {
    RequestId id = 1;

    EXPECT_FALSE(mgr->has_timeout(id));

    mgr->set_timeout(id, std::chrono::milliseconds(100), [](RequestId) {});

    EXPECT_TRUE(mgr->has_timeout(id));

    mgr->cancel(id);

    EXPECT_FALSE(mgr->has_timeout(id));
}

TEST_F(TimeoutManagerTest, PendingCount_Accurate) {
    EXPECT_EQ(mgr->pending_count(), 0);

    mgr->set_timeout(1, std::chrono::milliseconds(100), [](RequestId) {});
    EXPECT_EQ(mgr->pending_count(), 1);

    mgr->set_timeout(2, std::chrono::milliseconds(100), [](RequestId) {});
    EXPECT_EQ(mgr->pending_count(), 2);

    mgr->cancel(1);
    EXPECT_EQ(mgr->pending_count(), 1);
}

TEST_F(TimeoutManagerTest, DefaultTimeout_ConstructorSetsValue) {
    auto custom_mgr = std::make_unique<TimeoutManager>(std::chrono::milliseconds(500));
    EXPECT_EQ(custom_mgr->default_timeout(), std::chrono::milliseconds(500));
}

TEST_F(TimeoutManagerTest, SetTimeout_ReplacesExistingTimeout) {
    bool first_called = false;
    bool second_called = false;

    RequestId id = 1;

    // Set first timeout
    mgr->set_timeout(id, std::chrono::milliseconds(30),
        [&first_called](RequestId) { first_called = true; });

    // Replace with second timeout
    mgr->set_timeout(id, std::chrono::milliseconds(100),
        [&second_called](RequestId) { second_called = true; });

    EXPECT_EQ(mgr->pending_count(), 1);

    // Wait for first timeout period
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto expired = mgr->check_timeouts();

    // First callback should not have been called (replaced)
    EXPECT_FALSE(first_called);
    // Second callback not called yet (longer timeout)
    EXPECT_FALSE(second_called);
}

TEST_F(TimeoutManagerTest, StringId_Supported) {
    RequestId id = std::string("req-string-123");
    bool timeout_called = false;

    mgr->set_timeout(id, std::chrono::milliseconds(30),
        [&timeout_called, &id](RequestId expired_id) {
            timeout_called = true;
            EXPECT_EQ(expired_id, id);
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto expired = mgr->check_timeouts();
    EXPECT_GE(expired.size(), 1);
    EXPECT_TRUE(timeout_called);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(RequestTrackerTimeoutIntegration, CompleteBeforeTimeout) {
    RequestTracker tracker;
    TimeoutManager timeout_mgr(std::chrono::milliseconds(100));

    RequestId id = 1;
    bool response_received = false;
    bool timeout_called = false;

    ResponseCallback on_success = [&response_received](const nlohmann::json&) {
        response_received = true;
    };

    std::function<void(const JsonRpcError&)> on_error = [](const JsonRpcError&) {};

    tracker.register_pending(id, on_success, on_error);
    timeout_mgr.set_timeout(id, std::chrono::milliseconds(100),
        [&timeout_called](RequestId) { timeout_called = true; });

    // Simulate response arriving before timeout
    auto pending = tracker.complete(id);
    ASSERT_TRUE(pending.has_value());
    pending->on_success({{"result", "success"}});

    // Cancel timeout
    timeout_mgr.cancel(id);

    EXPECT_TRUE(response_received);
    EXPECT_FALSE(timeout_called);
}

TEST(RequestTrackerTimeoutIntegration, TimeoutBeforeResponse) {
    RequestTracker tracker;
    TimeoutManager timeout_mgr(std::chrono::milliseconds(30));

    RequestId id = 1;
    bool response_received = false;
    bool timeout_called = false;

    ResponseCallback on_success = [&response_received](const nlohmann::json&) {
        response_received = true;
    };

    std::function<void(const JsonRpcError&)> on_error = [](const JsonRpcError&) {};

    tracker.register_pending(id, on_success, on_error);
    timeout_mgr.set_timeout(id, std::chrono::milliseconds(30),
        [&timeout_called](RequestId) { timeout_called = true; });

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto expired = timeout_mgr.check_timeouts();
    EXPECT_GE(expired.size(), 1);
    EXPECT_TRUE(timeout_called);

    // Clean up request tracker
    tracker.cancel(id);

    EXPECT_FALSE(response_received);
}
