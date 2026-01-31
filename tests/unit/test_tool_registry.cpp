// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/tool_registry.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mcpp;
using namespace mcpp::server;

// Mock transport for testing RequestContext
class MockTransport : public transport::Transport {
public:
    bool connected = false;
    std::string last_sent;

    bool connect() override { connected = true; return true; }
    void disconnect() override { connected = false; }
    bool is_connected() const override { return connected; }
    bool send(std::string_view message) override {
        last_sent = std::string(message);
        return true;
    }
    void set_message_callback(MessageCallback cb) override {}
    void set_error_callback(ErrorCallback cb) override {}
};

// ============================================================================
// ToolRegistry Tests
// ============================================================================

TEST(ToolRegistry, RegisterAndList) {
    ToolRegistry registry;

    nlohmann::json input_schema = nlohmann::json::parse(R"({
        "type": "object",
        "properties": {
            "message": {"type": "string"}
        }
    })");

    bool registered = registry.register_tool(
        "test_tool",
        "A test tool",
        input_schema,
        [](const std::string& name, const nlohmann::json& args, RequestContext& ctx) {
            return nlohmann::json{
                {"content", {{{"type", "text"}, {"text", "executed"}}}},
                {"isError", false}
            };
        }
    );

    EXPECT_TRUE(registered);
    EXPECT_EQ(registry.size(), 1);

    auto tools = registry.list_tools();
    EXPECT_EQ(tools.size(), 1);
    EXPECT_EQ(tools[0]["name"], "test_tool");
    EXPECT_EQ(tools[0]["description"], "A test tool");
    EXPECT_TRUE(tools[0].contains("inputSchema"));
}

TEST(ToolRegistry, RegisterDuplicate_ReturnsFalse) {
    ToolRegistry registry;

    nlohmann::json schema = nlohmann::json::parse(R"({"type": "object"})");

    bool first = registry.register_tool("echo", "Echo tool", schema,
        [](const std::string&, const nlohmann::json&, RequestContext&) {
            return nlohmann::json{{"content", nlohmann::json::array()}, {"isError", false}};
        }
    );

    bool second = registry.register_tool("echo", "Duplicate", schema,
        [](const std::string&, const nlohmann::json&, RequestContext&) {
            return nlohmann::json{{"content", nlohmann::json::array()}, {"isError", false}};
        }
    );

    EXPECT_TRUE(first);
    EXPECT_FALSE(second);
    EXPECT_EQ(registry.size(), 1);
}

TEST(ToolRegistry, CallTool_ExecutesHandler) {
    ToolRegistry registry;
    MockTransport transport;
    RequestContext ctx("req-1", transport);

    nlohmann::json schema = nlohmann::json::parse(R"({
        "type": "object",
        "properties": {"value": {"type": "number"}}
    })");

    bool called = false;
    std::string received_name;

    registry.register_tool("callable", "Can be called", schema,
        [&called, &received_name](const std::string& name, const nlohmann::json& args, RequestContext&) {
            called = true;
            received_name = name;
            return nlohmann::json{
                {"content", {{{"type", "text"}, {"text", "done"}}}},
                {"isError", false}
            };
        }
    );

    nlohmann::json args = {{"value", 42}};
    auto result = registry.call_tool("callable", args, ctx);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(called);
    EXPECT_EQ(received_name, "callable");
    EXPECT_EQ((*result)["content"][0]["text"], "done");
}

TEST(ToolRegistry, CallTool_UnknownTool_ReturnsNullopt) {
    ToolRegistry registry;
    MockTransport transport;
    RequestContext ctx("req-1", transport);

    auto result = registry.call_tool("unknown", nlohmann::json::object(), ctx);
    EXPECT_FALSE(result.has_value());
}

TEST(ToolRegistry, CallTool_WithAnnotations) {
    ToolRegistry registry;
    MockTransport transport;
    RequestContext ctx("req-1", transport);

    nlohmann::json schema = nlohmann::json::parse(R"({"type": "object"})");
    nlohmann::json output_schema = nlohmann::json::parse(R"({
        "type": "object",
        "properties": {"result": {"type": "string"}}
    })");

    ToolAnnotations annotations{
        .destructive = true,
        .read_only = false,
        .audience = "assistant",
        .priority = 10
    };

    bool registered = registry.register_tool(
        "delete_file",
        "Deletes a file",
        schema,
        output_schema,
        annotations,
        [](const std::string&, const nlohmann::json&, RequestContext&) {
            return nlohmann::json{
                {"content", nlohmann::json::array()},
                {"isError", false}
            };
        }
    );

    EXPECT_TRUE(registered);

    auto tools = registry.list_tools();
    EXPECT_EQ(tools[0]["annotations"]["destructive"], true);
    EXPECT_EQ(tools[0]["annotations"]["readOnly"], false);
    EXPECT_EQ(tools[0]["annotations"]["audience"], "assistant");
    EXPECT_EQ(tools[0]["annotations"]["priority"], 10);
    EXPECT_TRUE(tools[0].contains("outputSchema"));
}

TEST(ToolRegistry, HasTool_ChecksCorrectly) {
    ToolRegistry registry;

    nlohmann::json schema = nlohmann::json::parse(R"({"type": "object"})");

    EXPECT_FALSE(registry.has_tool("my_tool"));

    registry.register_tool("my_tool", "My tool", schema,
        [](const std::string&, const nlohmann::json&, RequestContext&) {
            return nlohmann::json{{"content", nlohmann::json::array()}, {"isError", false}};
        }
    );

    EXPECT_TRUE(registry.has_tool("my_tool"));
}

TEST(ToolRegistry, Empty_Accurate) {
    ToolRegistry registry;

    EXPECT_TRUE(registry.empty());

    nlohmann::json schema = nlohmann::json::parse(R"({"type": "object"})");
    registry.register_tool("tool", "Tool", schema,
        [](const std::string&, const nlohmann::json&, RequestContext&) {
            return nlohmann::json{{"content", nlohmann::json::array()}, {"isError", false}};
        }
    );

    EXPECT_FALSE(registry.empty());
}

TEST(ToolRegistry, Clear_RemovesAllTools) {
    ToolRegistry registry;

    nlohmann::json schema = nlohmann::json::parse(R"({"type": "object"})");

    registry.register_tool("tool1", "First", schema,
        [](const std::string&, const nlohmann::json&, RequestContext&) {
            return nlohmann::json{{"content", nlohmann::json::array()}, {"isError", false}};
        }
    );
    registry.register_tool("tool2", "Second", schema,
        [](const std::string&, const nlohmann::json&, RequestContext&) {
            return nlohmann::json{{"content", nlohmann::json::array()}, {"isError", false}};
        }
    );

    EXPECT_EQ(registry.size(), 2);

    registry.clear();

    EXPECT_EQ(registry.size(), 0);
    EXPECT_TRUE(registry.empty());
}

TEST(ToolRegistry, SetNotifyCallback_InvokedOnNotifyChanged) {
    ToolRegistry registry;

    bool callback_called = false;
    registry.set_notify_callback([&callback_called]() {
        callback_called = true;
    });

    nlohmann::json schema = nlohmann::json::parse(R"({"type": "object"})");
    registry.register_tool("tool", "Tool", schema,
        [](const std::string&, const nlohmann::json&, RequestContext&) {
            return nlohmann::json{{"content", nlohmann::json::array()}, {"isError", false}};
        }
    );

    // Notify changed should invoke callback
    registry.notify_changed();

    EXPECT_TRUE(callback_called);
}

TEST(ToolRegistry, ListToolsPaginated_NoCursor) {
    ToolRegistry registry;

    nlohmann::json schema = nlohmann::json::parse(R"({"type": "object"})");

    for (int i = 0; i < 3; ++i) {
        std::string name = "tool" + std::to_string(i);
        registry.register_tool(name, "Tool " + std::to_string(i), schema,
            [](const std::string&, const nlohmann::json&, RequestContext&) {
                return nlohmann::json{{"content", nlohmann::json::array()}, {"isError", false}};
            }
        );
    }

    auto page = registry.list_tools_paginated(std::nullopt);
    EXPECT_EQ(page.items.size(), 3);
    EXPECT_FALSE(page.has_more());
}

TEST(ToolRegistry, ListToolsPaginated_WithCursor) {
    ToolRegistry registry;

    nlohmann::json schema = nlohmann::json::parse(R"({"type": "object"})");

    // Register more tools than default page size (50)
    for (int i = 0; i < 60; ++i) {
        std::string name = "tool" + std::to_string(i);
        registry.register_tool(name, "Tool " + std::to_string(i), schema,
            [](const std::string&, const nlohmann::json&, RequestContext&) {
                return nlohmann::json{{"content", nlohmann::json::array()}, {"isError", false}};
            }
        );
    }

    auto page1 = registry.list_tools_paginated(std::nullopt);
    EXPECT_GT(page1.items.size(), 0);
    EXPECT_TRUE(page1.has_more());

    if (page1.has_more()) {
        auto page2 = registry.list_tools_paginated(page1.nextCursor);
        EXPECT_GT(page2.items.size(), 0);
    }
}

TEST(ToolRegistry, CallTool_PassesArgumentsToHandler) {
    ToolRegistry registry;
    MockTransport transport;
    RequestContext ctx("req-1", transport);

    nlohmann::json schema = nlohmann::json::parse(R"({
        "type": "object",
        "properties": {
            "x": {"type": "number"},
            "y": {"type": "number"}
        }
    })");

    int received_x = 0, received_y = 0;

    registry.register_tool("add", "Add numbers", schema,
        [&received_x, &received_y](const std::string&, const nlohmann::json& args, RequestContext&) {
            received_x = args["x"];
            received_y = args["y"];
            int sum = received_x + received_y;
            return nlohmann::json{
                {"content", {{{"type", "text"}, {"text", std::to_string(sum)}}}},
                {"isError", false}
            };
        }
    );

    nlohmann::json args = {{"x", 10}, {"y", 32}};
    auto result = registry.call_tool("add", args, ctx);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(received_x, 10);
    EXPECT_EQ(received_y, 32);
    EXPECT_EQ((*result)["content"][0]["text"], "42");
}
