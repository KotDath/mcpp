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

#ifndef MCPP_API_ROLE_H
#define MCPP_API_ROLE_H

#include <concepts>

namespace mcpp::api {

/**
 * @brief Marker type for client role in role-based API
 *
 * RoleClient is an empty marker type used at compile time to distinguish
 * client-side APIs from server-side APIs. When used as a template parameter,
 * it enables static polymorphism for role-specific behavior.
 *
 * This follows the rust-sdk pattern where client and server roles are
 * distinct types with different associated types and capabilities.
 *
 * Example:
 *   Service<RoleClient> client_service;
 *   Peer<RoleClient> client_peer;
 */
struct RoleClient {
    // RoleClient is always on the client side
    static constexpr bool IS_CLIENT = true;
};

/**
 * @brief Marker type for server role in role-based API
 *
 * RoleServer is an empty marker type used at compile time to distinguish
 * server-side APIs from client-side APIs. When used as a template parameter,
 * it enables static polymorphism for role-specific behavior.
 *
 * This follows the rust-sdk pattern where client and server roles are
 * distinct types with different associated types and capabilities.
 *
 * Example:
 *   Service<RoleServer> server_service;
 *   Peer<RoleServer> server_peer;
 */
struct RoleServer {
    // RoleServer is always on the server side
    static constexpr bool IS_CLIENT = false;
};

/**
 * @brief Concept constraining template parameters to valid service roles
 *
 * ServiceRole concept ensures that a type is either RoleClient or RoleServer.
 * This provides compile-time type safety for role-based APIs, preventing
 * accidental use of incorrect types in Service and Peer templates.
 *
 * Example:
 *   template<ServiceRole Role>
 *   class Service { ... };
 *
 *   Service<RoleClient> client;  // OK
 *   Service<RoleServer> server;  // OK
 *   Service<int> invalid;        // Compile error
 */
template<typename Role>
concept ServiceRole = std::same_as<Role, RoleClient> || std::same_as<Role, RoleServer>;

} // namespace mcpp::api

#endif // MCPP_API_ROLE_H
