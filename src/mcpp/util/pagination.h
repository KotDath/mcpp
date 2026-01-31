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

#ifndef MCPP_UTIL_PAGINATION_H
#define MCPP_UTIL_PAGINATION_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "mcpp/content/pagination.h"

namespace mcpp::util {

/**
 * @brief Parameters for a paginated request
 *
 * Contains the cursor position and optional limit for pagination requests.
 * The cursor is an opaque token encoding the current position.
 */
struct PaginatedRequest {
    /// Opaque cursor for pagination (empty or nullopt for first page)
    std::optional<std::string> cursor;

    /// Optional limit on items per page
    std::optional<size_t> limit;
};

/**
 * @brief Helper function to automatically paginate through all results
 *
 * This function eliminates manual cursor tracking by calling the provided
 * list function repeatedly until all pages have been fetched.
 *
 * @tparam T The type of items in the paginated result
 * @tparam ListFn The type of the list function (deduced)
 * @param list_fn A callable that takes an optional<std::string> cursor
 *                and returns a PaginatedResult<T>
 * @return std::vector<T> All items accumulated across all pages
 *
 * @par Example
 * @code
 * auto all_tools = mcpp::util::list_all<mcpp::Tool>(
 *     [&client](std::optional<std::string> cursor) {
 *         return client.list_tools_paginated(cursor);
 *     }
 * );
 * @endcode
 *
 * @note The list function should return an empty next_cursor when
 *       there are no more pages.
 * @throw Any exception thrown by the list_fn propagates to the caller
 */
template <typename T, typename ListFn>
std::vector<T> list_all(ListFn&& list_fn) {
    std::vector<T> items;
    std::optional<std::string> cursor;

    do {
        auto page = list_fn(cursor);
        items.insert(
            items.end(),
            std::make_move_iterator(page.items.begin()),
            std::make_move_iterator(page.items.end())
        );
        cursor = page.nextCursor;
    } while (cursor);

    return items;
}

} // namespace mcpp::util

#endif // MCPP_UTIL_PAGINATION_H
