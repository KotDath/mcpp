// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef MCPP_CONTENT_PAGINATION_H
#define MCPP_CONTENT_PAGINATION_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mcpp::content {

/**
 * @brief Paginated result wrapper for list operations
 *
 * Provides cursor-based pagination for registry list operations.
 * The cursor is an opaque token encoding the current position;
 * clients should not interpret its contents.
 *
 * @tparam T Type of items in the result (e.g., nlohmann::json for tool metadata)
 *
 * Usage example:
 * ```cpp
 * auto page = registry.list_tools_paginated(std::nullopt);
 * // Process page.items...
 *
 * while (page.has_more()) {
 *     page = registry.list_tools_paginated(page.nextCursor);
 *     // Process next page...
 * }
 * ```
 */
template<typename T>
struct PaginatedResult {
    /// Items in this page
    std::vector<T> items;

    /// Opaque cursor for next page (present if more results exist)
    std::optional<std::string> nextCursor;

    /// Optional total count across all pages
    std::optional<uint64_t> total;

    /**
     * @brief Check if more pages exist
     *
     * @return true if there is a next page, false if this is the last page
     */
    bool has_more() const noexcept { return nextCursor.has_value(); }

    /**
     * @brief Default constructor
     */
    PaginatedResult() = default;

    /**
     * @brief Constructor with items only (no more pages)
     *
     * Creates a paginated result with all items and no next cursor.
     *
     * @param items Vector of items for this result
     */
    PaginatedResult(std::vector<T> items) : items(std::move(items)) {}

    /**
     * @brief Full constructor with cursor and optional total
     *
     * @param items Vector of items for this page
     * @param cursor Opaque cursor string for next page (empty if no more pages)
     * @param total Optional total count across all pages
     */
    PaginatedResult(std::vector<T> items,
                   std::optional<std::string> cursor,
                   std::optional<uint64_t> total = std::nullopt)
        : items(std::move(items)),
          nextCursor(std::move(cursor)),
          total(std::move(total)) {}
};

} // namespace mcpp::content

#endif // MCPP_CONTENT_PAGINATION_H
