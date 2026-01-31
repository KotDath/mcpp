// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/prompt_registry.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mcpp;
using namespace mcpp::server;

// ============================================================================
// PromptRegistry Tests
// ============================================================================

TEST(PromptRegistry, RegisterAndList) {
    PromptRegistry registry;

    std::vector<PromptArgument> args = {
        PromptArgument{"topic", "The topic to write about", true}
    };

    bool registered = registry.register_prompt(
        "greeting",
        "Say hello",
        args,
        [](const std::string& name, const nlohmann::json& arguments) {
            return std::vector<PromptMessage>{
                PromptMessage{"user", {
                    {"type", "text"},
                    {"text", "Hello!"}
                }}
            };
        }
    );

    EXPECT_TRUE(registered);

    auto prompts = registry.list_prompts();
    EXPECT_EQ(prompts.size(), 1);
    EXPECT_EQ(prompts[0]["name"], "greeting");
    EXPECT_EQ(prompts[0]["description"], "Say hello");
    EXPECT_EQ(prompts[0]["arguments"][0]["name"], "topic");
}

TEST(PromptRegistry, RegisterDuplicate_ReturnsFalse) {
    PromptRegistry registry;

    bool first = registry.register_prompt(
        "echo",
        "Echo prompt",
        {},
        [](const std::string&, const nlohmann::json&) {
            return std::vector<PromptMessage>{};
        }
    );

    bool second = registry.register_prompt(
        "echo",
        "Duplicate",
        {},
        [](const std::string&, const nlohmann::json&) {
            return std::vector<PromptMessage>{};
        }
    );

    EXPECT_TRUE(first);
    EXPECT_FALSE(second);
}

TEST(PromptRegistry, GetPrompt_ExecutesHandler) {
    PromptRegistry registry;

    bool called = false;
    std::string received_name;

    registry.register_prompt(
        "test",
        "Test prompt",
        {},
        [&called, &received_name](const std::string& name, const nlohmann::json& arguments) {
            called = true;
            received_name = name;
            return std::vector<PromptMessage>{
                PromptMessage{"user", {
                    {"type", "text"},
                    {"text", "Test content"}
                }}
            };
        }
    );

    auto result = registry.get_prompt("test", nlohmann::json::object());

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(called);
    EXPECT_EQ(received_name, "test");
    EXPECT_EQ((*result)["messages"][0]["role"], "user");
    EXPECT_EQ((*result)["messages"][0]["content"]["text"], "Test content");
}

TEST(PromptRegistry, GetPrompt_UnknownPrompt_ReturnsNullopt) {
    PromptRegistry registry;

    auto result = registry.get_prompt("unknown", nlohmann::json::object());
    EXPECT_FALSE(result.has_value());
}

TEST(PromptRegistry, GetPrompt_WithArguments) {
    PromptRegistry registry;

    std::vector<PromptArgument> args = {
        PromptArgument{"name", "The person to greet", true},
        PromptArgument{"greeting", "The greeting word", false}
    };

    registry.register_prompt(
        "personal_greeting",
        "Greet someone personally",
        args,
        [](const std::string&, const nlohmann::json& arguments) {
            std::string name = arguments.value("name", "World");
            std::string greeting = arguments.value("greeting", "Hello");

            std::string text = greeting + ", " + name + "!";

            return std::vector<PromptMessage>{
                PromptMessage{"user", {
                    {"type", "text"},
                    {"text", text}
                }}
            };
        }
    );

    nlohmann::json args = {
        {"name", "Alice"},
        {"greeting", "Hi"}
    };

    auto result = registry.get_prompt("personal_greeting", args);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)["messages"][0]["content"]["text"], "Hi, Alice!");
}

TEST(PromptRegistry, HasPrompt_ChecksCorrectly) {
    PromptRegistry registry;

    EXPECT_FALSE(registry.has_prompt("my_prompt"));

    registry.register_prompt(
        "my_prompt",
        "My prompt",
        {},
        [](const std::string&, const nlohmann::json&) {
            return std::vector<PromptMessage>{};
        }
    );

    EXPECT_TRUE(registry.has_prompt("my_prompt"));
}

TEST(PromptRegistry, Register_WithNoArguments) {
    PromptRegistry registry;

    bool registered = registry.register_prompt(
        "simple",
        "A simple prompt",
        {},  // No arguments
        [](const std::string&, const nlohmann::json&) {
            return std::vector<PromptMessage>{
                PromptMessage{"user", {
                    {"type", "text"},
                    {"text", "Simple content"}
                }}
            };
        }
    );

    EXPECT_TRUE(registered);

    auto prompts = registry.list_prompts();
    EXPECT_TRUE(prompts[0]["arguments"].is_null() || prompts[0]["arguments"].empty());
}

TEST(PromptRegistry, GetPrompt_MultipleMessages) {
    PromptRegistry registry;

    registry.register_prompt(
        "conversation",
        "A conversation starter",
        {},
        [](const std::string&, const nlohmann::json&) {
            return std::vector<PromptMessage>{
                PromptMessage{"user", {
                    {"type", "text"},
                    {"text", "Let's discuss a topic."}
                }},
                PromptMessage{"assistant", {
                    {"type", "text"},
                    {"text", "Sure, what would you like to discuss?"}
                }}
            };
        }
    );

    auto result = registry.get_prompt("conversation", nlohmann::json::object());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)["messages"].size(), 2);
    EXPECT_EQ((*result)["messages"][0]["role"], "user");
    EXPECT_EQ((*result)["messages"][1]["role"], "assistant");
}

TEST(PromptRegistry, GetPrompt_WithOptionalArguments) {
    PromptRegistry registry;

    std::vector<PromptArgument> args = {
        PromptArgument{"required_arg", "Required", true},
        PromptArgument{"optional_arg", "Optional", false}
    };

    int call_count = 0;

    registry.register_prompt(
        "mixed_args",
        "Mixed arguments",
        args,
        [&call_count](const std::string&, const nlohmann::json& arguments) {
            call_count++;
            return std::vector<PromptMessage>{
                PromptMessage{"user", {
                    {"type", "text"},
                    {"text", "Called " + std::to_string(call_count) + " times"}
                }}
            };
        }
    );

    // Call with only required argument
    nlohmann::json partial_args = {{"required_arg", "value"}};
    auto result1 = registry.get_prompt("mixed_args", partial_args);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(call_count, 1);

    // Call with both arguments
    nlohmann::json full_args = {
        {"required_arg", "value"},
        {"optional_arg", "optional_value"}
    };
    auto result2 = registry.get_prompt("mixed_args", full_args);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(call_count, 2);
}

TEST(PromptRegistry, SetNotifyCallback_InvokedOnNotifyChanged) {
    PromptRegistry registry;

    bool callback_called = false;
    registry.set_notify_callback([&callback_called]() {
        callback_called = true;
    });

    registry.notify_changed();

    EXPECT_TRUE(callback_called);
}

TEST(PromptRegistry, ListPromptsPaginated_NoCursor) {
    PromptRegistry registry;

    for (int i = 0; i < 3; ++i) {
        std::string name = "prompt" + std::to_string(i);
        registry.register_prompt(
            name,
            "Prompt " + std::to_string(i),
            {},
            [](const std::string&, const nlohmann::json&) {
                return std::vector<PromptMessage>{};
            }
        );
    }

    auto page = registry.list_prompts_paginated(std::nullopt);
    EXPECT_EQ(page.items.size(), 3);
    EXPECT_FALSE(page.has_more());
}

TEST(PromptRegistry, ListPromptsPaginated_WithCursor) {
    PromptRegistry registry;

    // Register more prompts than default page size (50)
    for (int i = 0; i < 60; ++i) {
        std::string name = "prompt" + std::to_string(i);
        registry.register_prompt(
            name,
            "Prompt " + std::to_string(i),
            {},
            [](const std::string&, const nlohmann::json&) {
                return std::vector<PromptMessage>{};
            }
        );
    }

    auto page1 = registry.list_prompts_paginated(std::nullopt);
    EXPECT_GT(page1.items.size(), 0);

    if (page1.has_more()) {
        auto page2 = registry.list_prompts_paginated(page1.nextCursor);
        EXPECT_GT(page2.items.size(), 0);
    }
}

TEST(PromptRegistry, CompletionHandler_SetAndGet) {
    PromptRegistry registry;

    registry.set_completion_handler(
        "my_prompt",
        [](const std::string& arg_name, const nlohmann::json& current_value,
           const std::optional<nlohmann::json>& ref) -> std::vector<Completion> {
            return {
                Completion{"option1", "First option"},
                Completion{"option2", "Second option"}
            };
        }
    );

    auto completions = registry.get_completion("my_prompt", "style", "code", std::nullopt);

    ASSERT_TRUE(completions.has_value());
    EXPECT_EQ(completions->size(), 2);
    EXPECT_EQ((*completions)[0].value, "option1");
}

TEST(PromptRegistry, CompletionHandler_NoHandler_ReturnsNullopt) {
    PromptRegistry registry;

    auto completions = registry.get_completion("unknown", "arg", "value", std::nullopt);
    EXPECT_FALSE(completions.has_value());
}

TEST(PromptRegistry, ArgumentWithDescription) {
    PromptRegistry registry;

    std::vector<PromptArgument> args = {
        PromptArgument{"style", "Writing style (formal/casual)", true},
        PromptArgument{"length", "Response length (short/long)", false}
    };

    registry.register_prompt(
        "writer",
        "Writing assistant",
        args,
        [](const std::string&, const nlohmann::json&) {
            return std::vector<PromptMessage>{
                PromptMessage{"user", {
                    {"type", "text"},
                    {"text", "Ready to write"}
                }}
            };
        }
    );

    auto prompts = registry.list_prompts();
    EXPECT_EQ(prompts[0]["arguments"][0]["name"], "style");
    EXPECT_EQ(prompts[0]["arguments"][0]["description"], "Writing style (formal/casual)");
    EXPECT_EQ(prompts[0]["arguments"][0]["required"], true);
    EXPECT_EQ(prompts[0]["arguments"][1]["required"], false);
}

TEST(PromptRegistry, PromptContent_WithStructuredContent) {
    PromptRegistry registry;

    registry.register_prompt(
        "image_prompt",
        "Image generation prompt",
        {},
        [](const std::string&, const nlohmann::json&) {
            // Content can be structured (e.g., for image content)
            return std::vector<PromptMessage>{
                PromptMessage{"user", nlohmann::json{
                    {"type", "text"},
                    {"text", "Generate a sunset"}
                }}
            };
        }
    );

    auto result = registry.get_prompt("image_prompt", nlohmann::json::object());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)["messages"][0]["role"], "user");
    EXPECT_EQ((*result)["messages"][0]["content"]["type"], "text");
}
