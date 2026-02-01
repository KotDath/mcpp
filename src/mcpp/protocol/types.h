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

#ifndef MCPP_PROTOCOL_TYPES_H
#define MCPP_PROTOCOL_TYPES_H

#include <optional>
#include <string>

namespace mcpp::protocol {

/**
 * Implementation information for an MCP client or server.
 *
 * Corresponds to the "Implementation" type in MCP 2025-11-25 schema.
 * Used to identify the client/server in initialize messages.
 *
 * Example JSON:
 * {"name": "my-client", "version": "1.0.0"}
 */
struct Implementation {
    std::string name;
    std::string version;
};

/**
 * Metadata attached to requests.
 *
 * Corresponds to the "RequestMeta" type in MCP 2025-11-25 schema.
 * Contains optional metadata for requests like progress tokens and MIME types.
 *
 * Example JSON:
 * {"progressToken": "123", "mimeType": "application/json"}
 */
struct RequestMeta {
    std::optional<std::string> progressToken;
    std::optional<std::string> mimeType;
};

} // namespace mcpp::protocol

#endif // MCPP_PROTOCOL_TYPES_H
