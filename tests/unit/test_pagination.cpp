// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/util/pagination.h"
#include "mcpp/content/pagination.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mcpp;
using namespace mcpp::util;
using namespace mcpp::content;

// ============================================================================
// PaginatedResult Tests
// ============================================================================

TEST(PaginatedResult, DefaultConstruction) {
    PaginatedResult<int> result;

    EXPECT_TRUE(result.items.empty());
    EXPECT_FALSE(result.nextCursor.has_value());
    EXPECT_FALSE(result.total.has_value());
}

TEST(PaginatedResult, Construction_WithItemsOnly) {
    std::vector<int> items = {1, 2, 3};
    PaginatedResult<int> result(items);

    EXPECT_EQ(result.items.size(), 3);
    EXPECT_EQ(result.items[0], 1);
    EXPECT_FALSE(result.nextCursor.has_value());
}

TEST(PaginatedResult, Construction_WithCursor) {
    std::vector<int> items = {1, 2, 3};
    PaginatedResult<int> result(items, "next_cursor", 10);

    EXPECT_EQ(result.items.size(), 3);
    EXPECT_TRUE(result.nextCursor.has_value());
    EXPECT_EQ(*result.nextCursor, "next_cursor");
    EXPECT_TRUE(result.total.has_value());
    EXPECT_EQ(*result.total, 10);
}

TEST(PaginatedResult, HasMore_WithCursor_ReturnsTrue) {
    std::vector<int> items = {1, 2, 3};
    PaginatedResult<int> result(items, "next_cursor", 10);

    EXPECT_TRUE(result.has_more());
    EXPECT_EQ(result.nextCursor, "next_cursor");
    EXPECT_EQ(result.total, 10);
}

TEST(PaginatedResult, HasMore_NoCursor_ReturnsFalse) {
    std::vector<int> items = {1, 2, 3};
    PaginatedResult<int> result(items, std::nullopt, 3);

    EXPECT_FALSE(result.has_more());
    EXPECT_EQ(result.total, 3);
}

TEST(PaginatedResult, HasMore_EmptyCursor_ReturnsFalse) {
    std::vector<int> items = {1, 2, 3};
    PaginatedResult<int> result(items, "", 3);

    // Empty string cursor still counts as having a value
    // but semantically indicates no more pages
    EXPECT_TRUE(result.nextCursor.has_value());
    EXPECT_EQ(*result.nextCursor, "");
}

TEST(PaginatedResult, OptionalTotal_NotProvided) {
    std::vector<int> items = {1, 2};
    PaginatedResult<int> result(items, "cursor");

    EXPECT_TRUE(result.has_more());
    EXPECT_FALSE(result.total.has_value());
}

TEST(PaginatedResult, MoveConstruction) {
    std::vector<int> items = {1, 2, 3};
    PaginatedResult<int> result1(items, "cursor", 10);

    PaginatedResult<int> result2(std::move(result1));

    EXPECT_EQ(result2.items.size(), 3);
    EXPECT_EQ(*result2.nextCursor, "cursor");
    EXPECT_EQ(*result2.total, 10);
}

TEST(PaginatedResult, JsonIntItems) {
    std::vector<int> items = {1, 2, 3};
    PaginatedResult<int> result(items, "abc", 5);

    // Test with a simple serialization function
    nlohmann::json j = nlohmann::json{
        {"items", result.items},
        {"nextCursor", result.nextCursor},
        {"total", result.total}
    };

    EXPECT_EQ(j["items"].size(), 3);
    EXPECT_EQ(j["items"][0], 1);
    EXPECT_EQ(j["nextCursor"], "abc");
    EXPECT_EQ(j["total"], 5);
}

TEST(PaginatedResult, JsonStringItems) {
    std::vector<std::string> items = {"apple", "banana", "cherry"};
    PaginatedResult<std::string> result(items, "page2", std::nullopt);

    EXPECT_EQ(result.items[0], "apple");
    EXPECT_EQ(result.items[1], "banana");
    EXPECT_EQ(result.items[2], "cherry");
}

// ============================================================================
// list_all Tests
// ============================================================================

TEST(ListAll, SinglePage) {
    // Mock function that returns a paginated result without more pages
    auto fetch_page = [](const std::string& cursor) -> PaginatedResult<int> {
        if (cursor.empty()) {
            return {{1, 2, 3}, std::nullopt, 3};
        }
        return {{}, std::nullopt, 0};
    };

    auto all_items = list_all<int>(fetch_page);
    EXPECT_EQ(all_items.size(), 3);
    EXPECT_EQ(all_items[0], 1);
    EXPECT_EQ(all_items[1], 2);
    EXPECT_EQ(all_items[2], 3);
}

TEST(ListAll, MultiplePages) {
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
    EXPECT_EQ(all_items[0], 1);
    EXPECT_EQ(all_items[1], 2);
    EXPECT_EQ(all_items[2], 3);
    EXPECT_EQ(all_items[3], 4);
    EXPECT_EQ(all_items[4], 5);
    EXPECT_EQ(call_count, 3);  // 3 pages fetched
}

TEST(ListAll, EmptyResult) {
    auto fetch_page = [](const std::string& cursor) -> PaginatedResult<int> {
        return {{}, std::nullopt, 0};
    };

    auto all_items = list_all<int>(fetch_page);

    EXPECT_EQ(all_items.size(), 0);
    EXPECT_TRUE(all_items.empty());
}

TEST(ListAll, LargeDataset) {
    int total_items = 0;

    auto fetch_page = [&total_items](const std::string& cursor) -> PaginatedResult<int> {
        if (cursor.empty()) {
            total_items += 50;
            std::vector<int> page;
            for (int i = 0; i < 50; ++i) page.push_back(i);
            return {page, "page2", 150};
        } else if (cursor == "page2") {
            total_items += 50;
            std::vector<int> page;
            for (int i = 50; i < 100; ++i) page.push_back(i);
            return {page, "page3", 150};
        } else if (cursor == "page3") {
            total_items += 50;
            std::vector<int> page;
            for (int i = 100; i < 150; ++i) page.push_back(i);
            return {page, std::nullopt, 150};
        }
        return {{}, std::nullopt, 0};
    };

    auto all_items = list_all<int>(fetch_page);

    EXPECT_EQ(all_items.size(), 150);
    EXPECT_EQ(all_items[0], 0);
    EXPECT_EQ(all_items[149], 149);
}

TEST(ListAll, StringItems) {
    auto fetch_page = [](const std::string& cursor) -> PaginatedResult<std::string> {
        if (cursor.empty()) {
            return {{"alpha", "beta"}, "page2", 4};
        } else if (cursor == "page2") {
            return {{"gamma", "delta"}, std::nullopt, 4};
        }
        return {{}, std::nullopt, 0};
    };

    auto all_items = list_all<std::string>(fetch_page);

    EXPECT_EQ(all_items.size(), 4);
    EXPECT_EQ(all_items[0], "alpha");
    EXPECT_EQ(all_items[1], "beta");
    EXPECT_EQ(all_items[2], "gamma");
    EXPECT_EQ(all_items[3], "delta");
}

TEST(ListAll, JsonItems) {
    auto fetch_page = [](const std::string& cursor) -> PaginatedResult<nlohmann::json> {
        if (cursor.empty()) {
            return {
                {{{"id", 1}, {"name", "one"}}, {{"id", 2}, {"name", "two"}}},
                "page2",
                4
            };
        } else if (cursor == "page2") {
            return {
                {{{"id", 3}, {"name", "three"}}, {{"id", 4}, {"name", "four"}}},
                std::nullopt,
                4
            };
        }
        return {{}, std::nullopt, 0};
    };

    auto all_items = list_all<nlohmann::json>(fetch_page);

    EXPECT_EQ(all_items.size(), 4);
    EXPECT_EQ(all_items[0]["id"], 1);
    EXPECT_EQ(all_items[0]["name"], "one");
    EXPECT_EQ(all_items[3]["id"], 4);
    EXPECT_EQ(all_items[3]["name"], "four");
}

// ============================================================================
// PaginatedRequest Tests
// ============================================================================

TEST(PaginatedRequest, DefaultConstruction) {
    PaginatedRequest req;

    EXPECT_FALSE(req.cursor.has_value());
    EXPECT_FALSE(req.limit.has_value());
}

TEST(PaginatedRequest, WithCursor) {
    PaginatedRequest req;
    req.cursor = "some_cursor";

    EXPECT_TRUE(req.cursor.has_value());
    EXPECT_EQ(*req.cursor, "some_cursor");
}

TEST(PaginatedRequest, WithLimit) {
    PaginatedRequest req;
    req.limit = 100;

    EXPECT_TRUE(req.limit.has_value());
    EXPECT_EQ(*req.limit, 100);
}

TEST(PaginatedRequest, WithCursorAndLimit) {
    PaginatedRequest req{"cursor_123", 50};

    EXPECT_EQ(req.cursor, "cursor_123");
    EXPECT_EQ(req.limit, 50);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(PaginationIntegration, RegistryStylePagination) {
    // Simulate a registry-style pagination pattern
    struct Item {
        std::string name;
        int value;
    };

    std::vector<Item> all_db_items = {
        {"item1", 1}, {"item2", 2}, {"item3", 3},
        {"item4", 4}, {"item5", 5}, {"item6", 6}
    };

    auto fetch_page = [&all_db_items](const std::string& cursor) -> PaginatedResult<Item> {
        int offset = 0;
        if (!cursor.empty()) {
            offset = std::stoi(cursor);
        }

        const size_t page_size = 2;
        std::vector<Item> page_items;

        for (size_t i = offset; i < std::min(offset + page_size, all_db_items.size()); ++i) {
            page_items.push_back(all_db_items[i]);
        }

        std::optional<std::string> next_cursor;
        if (offset + page_size < all_db_items.size()) {
            next_cursor = std::to_string(offset + page_size);
        }

        return {page_items, next_cursor, static_cast<uint64_t>(all_db_items.size())};
    };

    auto all_items = list_all<Item>(fetch_page);

    EXPECT_EQ(all_items.size(), 6);
    EXPECT_EQ(all_items[0].name, "item1");
    EXPECT_EQ(all_items[5].name, "item6");
}

TEST(PaginationIntegration, EmptyPages) {
    // Test handling of empty pages in the middle of pagination
    int page_num = 0;

    auto fetch_page = [&page_num](const std::string& cursor) -> PaginatedResult<int> {
        if (cursor.empty()) {
            page_num = 1;
            return {{1}, "page2", 3};
        } else if (cursor == "page2") {
            page_num = 2;
            return {{}, "page3", 3};  // Empty page
        } else if (cursor == "page3") {
            page_num = 3;
            return {{2, 3}, std::nullopt, 3};
        }
        return {{}, std::nullopt, 0};
    };

    auto all_items = list_all<int>(fetch_page);

    EXPECT_EQ(all_items.size(), 3);
    EXPECT_EQ(all_items[0], 1);
    EXPECT_EQ(all_items[1], 2);
    EXPECT_EQ(all_items[2], 3);
}

TEST(PaginationIntegration, ConsistentTotal) {
    // Test that total is consistent across pages
    auto fetch_page = [](const std::string& cursor) -> PaginatedResult<int> {
        if (cursor.empty()) {
            return {{1, 2}, "page2", 100};
        } else if (cursor == "page2") {
            return {{3, 4}, "page3", 100};
        }
        return {{5}, std::nullopt, 100};
    };

    // Fetch individual pages
    auto page1 = fetch_page("");
    auto page2 = fetch_page(*page1.nextCursor);
    auto page3 = fetch_page(*page2.nextCursor);

    EXPECT_EQ(page1.total, 100);
    EXPECT_EQ(page2.total, 100);
    EXPECT_EQ(page3.total, 100);
}
