// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/resource_registry.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mcpp;
using namespace mcpp::server;

// Mock transport for testing
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
// ResourceRegistry Tests
// ============================================================================

TEST(ResourceRegistry, RegisterAndList) {
    ResourceRegistry registry;

    bool registered = registry.register_resource(
        "file://test.txt",
        "Test File",
        "A test resource",
        "text/plain",
        [](const std::string& uri) -> ResourceContent {
            return ResourceContent{uri, "text/plain", true, "Hello, World!", ""};
        }
    );

    EXPECT_TRUE(registered);

    auto resources = registry.list_resources();
    EXPECT_EQ(resources.size(), 1);
    EXPECT_EQ(resources[0]["uri"], "file://test.txt");
    EXPECT_EQ(resources[0]["name"], "Test File");
    EXPECT_EQ(resources[0]["mimeType"], "text/plain");
}

TEST(ResourceRegistry, Register_WithOptionalDescription) {
    ResourceRegistry registry;

    bool registered = registry.register_resource(
        "file://config.json",
        "Config",
        std::nullopt,  // No description
        "application/json",
        [](const std::string& uri) -> ResourceContent {
            return ResourceContent{uri, "application/json", true, "{}", ""};
        }
    );

    EXPECT_TRUE(registered);

    auto resources = registry.list_resources();
    EXPECT_FALSE(resources[0].contains("description"));
}

TEST(ResourceRegistry, ReadResource_ExecutesHandler) {
    ResourceRegistry registry;

    bool called = false;
    std::string received_uri;

    registry.register_resource(
        "file://data",
        "Data",
        "Test data resource",
        "text/plain",
        [&called, &received_uri](const std::string& uri) -> ResourceContent {
            called = true;
            received_uri = uri;
            return ResourceContent{uri, "text/plain", true, "data content", ""};
        }
    );

    auto result = registry.read_resource("file://data");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(called);
    EXPECT_EQ(received_uri, "file://data");
    EXPECT_EQ((*result)["contents"][0]["text"], "data content");
}

TEST(ResourceRegistry, ReadResource_UnknownUri_ReturnsNullopt) {
    ResourceRegistry registry;

    auto result = registry.read_resource("file://unknown");
    EXPECT_FALSE(result.has_value());
}

TEST(ResourceRegistry, HasResource_ChecksCorrectly) {
    ResourceRegistry registry;

    EXPECT_FALSE(registry.has_resource("file://test.txt"));

    registry.register_resource(
        "file://test.txt",
        "Test",
        std::nullopt,
        "text/plain",
        [](const std::string&) -> ResourceContent {
            return ResourceContent{"", "text/plain", true, "", ""};
        }
    );

    EXPECT_TRUE(registry.has_resource("file://test.txt"));
}

TEST(ResourceRegistry, ReadResource_BinaryContent) {
    ResourceRegistry registry;

    registry.register_resource(
        "file://image.png",
        "Image",
        "A PNG image",
        "image/png",
        [](const std::string& uri) -> ResourceContent {
            // Binary content uses blob field with base64 data
            return ResourceContent{uri, "image/png", false, "", "iVBORw0KG..."};
        }
    );

    auto result = registry.read_resource("file://image.png");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE((*result)["contents"][0]["blob"].is_string());
}

TEST(ResourceRegistry, ReadResource_WithMimeTypeOverride) {
    ResourceRegistry registry;

    registry.register_resource(
        "file://data",
        "Data",
        std::nullopt,
        "text/plain",
        [](const std::string& uri) -> ResourceContent {
            ResourceContent content;
            content.uri = uri;
            content.mime_type = "application/json";  // Override MIME type
            content.is_text = true;
            content.text = "{\"key\": \"value\"}";
            return content;
        }
    );

    auto result = registry.read_resource("file://data");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)["contents"][0]["mimeType"], "application/json");
}

TEST(ResourceRegistry, RegisterTemplate_Basic) {
    ResourceRegistry registry;

    bool registered = registry.register_template(
        "file://{path}",
        "File Reader",
        "Read any file",
        "text/plain",
        [](const std::string& uri, const nlohmann::json& params) -> ResourceContent {
            std::string path = params["path"];
            return ResourceContent{uri, "text/plain", true, "Content of " + path, ""};
        }
    );

    EXPECT_TRUE(registered);

    auto resources = registry.list_resources();
    bool found_template = false;
    for (const auto& r : resources) {
        if (r.contains("template") && r["template"] == "file://{path}") {
            found_template = true;
            break;
        }
    }
    EXPECT_TRUE(found_template);
}

TEST(ResourceRegistry, ReadResource_TemplateMatch) {
    ResourceRegistry registry;

    registry.register_template(
        "config://{section}/{key}",
        "Config",
        "Read config values",
        "text/plain",
        [](const std::string& uri, const nlohmann::json& params) -> ResourceContent {
            std::string section = params["section"];
            std::string key = params["key"];
            return ResourceContent{uri, "text/plain", true, section + ":" + key + "=value", ""};
        }
    );

    // Try to read a URI matching the template
    auto result = registry.read_resource("config://database/host");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->contains("contents"));
}

TEST(ResourceRegistry, Subscribe_Unsubscribe) {
    ResourceRegistry registry;

    registry.register_resource(
        "file://watch.txt",
        "Watched File",
        std::nullopt,
        "text/plain",
        [](const std::string&) -> ResourceContent {
            return ResourceContent{"", "text/plain", true, "", ""};
        }
    );

    // Subscribe
    bool subscribed = registry.subscribe("file://watch.txt", "client-1");
    EXPECT_TRUE(subscribed);

    // Unsubscribe
    bool unsubscribed = registry.unsubscribe("file://watch.txt", "client-1");
    EXPECT_TRUE(unsubscribed);

    // Unsubscribe again should return false
    unsubscribed = registry.unsubscribe("file://watch.txt", "client-1");
    EXPECT_FALSE(unsubscribed);
}

TEST(ResourceRegistry, SetNotifyCallback_InvokedOnNotifyChanged) {
    ResourceRegistry registry;

    bool callback_called = false;
    registry.set_notify_callback([&callback_called]() {
        callback_called = true;
    });

    registry.notify_changed();

    EXPECT_TRUE(callback_called);
}

TEST(ResourceRegistry, ListResourcesPaginated_NoCursor) {
    ResourceRegistry registry;

    for (int i = 0; i < 3; ++i) {
        std::string uri = "file://file" + std::to_string(i) + ".txt";
        registry.register_resource(
            uri,
            "File " + std::to_string(i),
            std::nullopt,
            "text/plain",
            [](const std::string&) -> ResourceContent {
                return ResourceContent{"", "text/plain", true, "", ""};
            }
        );
    }

    auto page = registry.list_resources_paginated(std::nullopt);
    EXPECT_EQ(page.items.size(), 3);
    EXPECT_FALSE(page.has_more());
}

TEST(ResourceRegistry, ListResourcesPaginated_WithCursor) {
    ResourceRegistry registry;

    // Register more resources than default page size (50)
    for (int i = 0; i < 60; ++i) {
        std::string uri = "file://file" + std::to_string(i) + ".txt";
        registry.register_resource(
            uri,
            "File " + std::to_string(i),
            std::nullopt,
            "text/plain",
            [](const std::string&) -> ResourceContent {
                return ResourceContent{"", "text/plain", true, "", ""};
            }
        );
    }

    auto page1 = registry.list_resources_paginated(std::nullopt);
    EXPECT_GT(page1.items.size(), 0);

    if (page1.has_more()) {
        auto page2 = registry.list_resources_paginated(page1.nextCursor);
        EXPECT_GT(page2.items.size(), 0);
    }
}

TEST(ResourceRegistry, CompletionHandler_SetAndGet) {
    ResourceRegistry registry;

    registry.set_completion_handler(
        "my_resource",
        [](const std::string& arg_name, const nlohmann::json& current_value,
           const std::optional<nlohmann::json>& ref) -> std::vector<Completion> {
            return {
                Completion{"value1", "First completion"},
                Completion{"value2", "Second completion"}
            };
        }
    );

    auto completions = registry.get_completion("my_resource", "path", "file", std::nullopt);

    ASSERT_TRUE(completions.has_value());
    EXPECT_EQ(completions->size(), 2);
    EXPECT_EQ((*completions)[0].value, "value1");
    EXPECT_EQ((*completions)[1].value, "value2");
}

TEST(ResourceRegistry, CompletionHandler_NoHandler_ReturnsNullopt) {
    ResourceRegistry registry;

    auto completions = registry.get_completion("unknown", "arg", "value", std::nullopt);
    EXPECT_FALSE(completions.has_value());
}

TEST(ResourceRegistry, MultipleSubscribers) {
    ResourceRegistry registry;

    registry.register_resource(
        "file://shared.txt",
        "Shared",
        std::nullopt,
        "text/plain",
        [](const std::string&) -> ResourceContent {
            return ResourceContent{"", "text/plain", true, "", ""};
        }
    );

    // Multiple subscribers
    EXPECT_TRUE(registry.subscribe("file://shared.txt", "client-1"));
    EXPECT_TRUE(registry.subscribe("file://shared.txt", "client-2"));

    // Unsubscribe one - other should still be subscribed
    EXPECT_TRUE(registry.unsubscribe("file://shared.txt", "client-1"));

    // Can still subscribe again
    EXPECT_TRUE(registry.subscribe("file://shared.txt", "client-3"));
}

TEST(ResourceRegistry, SetTransport_NotificationUsesTransport) {
    ResourceRegistry registry;
    MockTransport transport;

    registry.register_resource(
        "file://test.txt",
        "Test",
        std::nullopt,
        "text/plain",
        [](const std::string&) -> ResourceContent {
            return ResourceContent{"", "text/plain", true, "", ""};
        }
    );

    registry.set_transport(transport);
    registry.subscribe("file://test.txt", "client-1");

    // Notify updated should use transport
    registry.notify_updated("file://test.txt");

    // Transport should have received the notification
    EXPECT_FALSE(transport.last_sent.empty());
}
