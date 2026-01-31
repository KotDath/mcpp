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

#ifndef MCPP_UTIL_URI_TEMPLATE_H
#define MCPP_UTIL_URI_TEMPLATE_H

#include <nlohmann/json.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstdlib>

namespace mcpp {
namespace util {

/**
 * @brief RFC 6570 Level 1-2 URI template expansion
 *
 * Provides simple URI template expansion for MCP resource templates.
 * Supports Level 1-2 of RFC 6570:
 * - Level 1: Simple variable expansion {var}
 * - Level 2: Reserved expansion, query parameters {?var*}
 *
 * This is a minimal implementation sufficient for MCP resource templates.
 * It does not implement the full RFC 6570 specification (Level 3-4).
 *
 * @see https://datatracker.ietf.org/doc/html/rfc6570
 * @see https://modelcontextprotocol.io/specification/2025-11-25/server/resources
 */
class UriTemplate {
public:
    /**
     * @brief Expand a URI template with provided parameters
     *
     * Supports:
     * - {var} - Path-style variable substitution from params["var"]
     * - {?var*} - Query parameter expansion from params["var"] object
     *
     * Path variables use RFC 6570 reserve-style expansion (preserves /, ?, etc.).
     * Query parameters use percent-encoding per RFC 3986.
     *
     * @param template_str The URI template (e.g., "file://{path}")
     * @param params JSON object containing template variables
     * @return Expanded URI string
     *
     * Examples:
     *   expand("file://{path}", {{"path": "/etc/config"}})
     *     -> "file:///etc/config"
     *
     *   expand("http://example.com/api{?params*}", {{"params": {{"a","1"},{"b","2"}}}})
     *     -> "http://example.com/api?a=1&b=2"
     *
     *   expand("users/{id}", {{"id": "123"}})
     *     -> "users/123"
     */
    static std::string expand(const std::string& template_str,
                             const nlohmann::json& params) {
        std::string result = template_str;
        size_t pos = 0;

        // Process query parameter expansion {?var*}
        while ((pos = result.find("{?", pos)) != std::string::npos) {
            size_t end = result.find("}", pos);
            if (end == std::string::npos) break;

            std::string var_name = result.substr(pos + 2, end - pos - 2);
            // Remove * suffix if present
            if (!var_name.empty() && var_name.back() == '*') {
                var_name = var_name.substr(0, var_name.length() - 1);
            }

            std::string query_str = build_query_string(params, var_name);
            result.replace(pos, end - pos + 1, query_str);
            pos = pos + query_str.length();
        }

        // Process simple variable expansion {var}
        pos = 0;
        while ((pos = result.find("{", pos)) != std::string::npos) {
            // Skip if this is a query template already processed
            if (pos + 1 < result.length() && result[pos + 1] == '?') {
                pos++;
                continue;
            }

            size_t end = result.find("}", pos);
            if (end == std::string::npos) break;

            std::string var_name = result.substr(pos + 1, end - pos - 1);
            // Remove * suffix if present (Level 2 expansion modifier)
            if (!var_name.empty() && var_name.back() == '*') {
                var_name = var_name.substr(0, var_name.length() - 1);
            }

            std::string value;
            if (params.contains(var_name)) {
                const auto& param = params[var_name];
                if (param.is_string()) {
                    value = param.get<std::string>();
                } else if (param.is_number()) {
                    value = param.dump();
                } else {
                    value = param.dump();
                }
            }

            // Percent-encode for path component (preserves / and other path-safe chars)
            value = percent_encode_path(value);
            result.replace(pos, end - pos + 1, value);
            pos = pos + value.length();
        }

        return result;
    }

private:
    /**
     * @brief Build a query string from an object parameter
     *
     * Converts {"a": "1", "b": "2"} to "a=1&b=2"
     * Values are percent-encoded for query component.
     */
    static std::string build_query_string(const nlohmann::json& params,
                                          const std::string& var_name) {
        if (!params.contains(var_name)) {
            return "";
        }

        const auto& param = params[var_name];
        if (!param.is_object()) {
            return "";
        }

        std::ostringstream oss;
        bool first = true;

        for (auto it = param.begin(); it != param.end(); ++it) {
            if (!first) {
                oss << "&";
            }
            first = false;

            std::string key = it.key();
            std::string value;

            if (it.value().is_string()) {
                value = it.value().get<std::string>();
            } else {
                value = it.value().dump();
            }

            // Percent-encode both key and value for query component
            oss << percent_encode(key) << "=" << percent_encode(value);
        }

        std::string result = oss.str();
        if (!result.empty()) {
            return "?" + result;
        }
        return "";
    }

    /**
     * @brief Percent-encode a string for path components
     *
     * Uses RFC 6570 reserve-style encoding which preserves path segment
     * characters like / and ?. Similar to {+var} expansion in RFC 6570.
     * Encodes: space, ", #, <, >, ?, `, {, }, |, \, ^, [, ], control chars
     * Preserves: unreserved chars + /, : @, $ & , + = ; !
     */
    static std::string percent_encode_path(const std::string& input) {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0');

        for (unsigned char c : input) {
            // Unreserved characters per RFC 3986
            if (is_unreserved(c)) {
                oss << c;
            } else if (c == '/' || c == ':' || c == '@' || c == '$' ||
                       c == '&' || c == ',' || c == '+' || c == '=' ||
                       c == ';' || c == '!') {
                // Path-safe characters (RFC 6570 reserve expansion)
                oss << c;
            } else {
                oss << '%' << std::setw(2) << static_cast<int>(c);
            }
        }

        return oss.str();
    }

    /**
     * @brief Percent-encode a string for URI components
     *
     * Encodes reserved characters per RFC 3986.
     * Used for query parameter encoding.
     */
    static std::string percent_encode(const std::string& input) {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0');

        for (unsigned char c : input) {
            // Unreserved characters per RFC 3986
            if (is_unreserved(c)) {
                oss << c;
            } else {
                oss << '%' << std::setw(2) << static_cast<int>(c);
            }
        }

        return oss.str();
    }

    /**
     * @brief Check if a character is unreserved (doesn't need encoding)
     *
     * RFC 3986 unreserved characters: A-Z a-z 0-9 - . _ ~
     */
    static bool is_unreserved(unsigned char c) {
        return std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~';
    }

    // Static utility class - no instantiation
    UriTemplate() = delete;
    UriTemplate(const UriTemplate&) = delete;
    UriTemplate& operator=(const UriTemplate&) = delete;
};

} // namespace util
} // namespace mcpp

#endif // MCPP_UTIL_URI_TEMPLATE_H
