# Technology Stack

**Domain:** C++ MCP (Model Context Protocol) Library
**Researched:** 2025-01-31
**Overall Confidence:** HIGH

## Executive Summary

The recommended stack for building a C++17 MCP library prioritizes modern, header-only libraries where possible to minimize build complexity while maintaining performance. Key insights:

- **nlohmann/json** is the de facto standard for C++ JSON (v3.12.0, April 2025)
- **Asio standalone** (v1.36.0) is preferred over libevent for modern C++17 development
- **WebSocket++** remains the standard for WebSocket despite last release in 2020
- **GoogleTest 1.17.0+** now requires C++17, aligning with project requirements
- For SSE: Build custom parser using llhttp (v9.3.0) rather than relying on dated libraries

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| **nlohmann/json** | 3.12.0 | JSON parsing/serialization | De facto standard, header-only, C++17 optimized, 45.8k GitHub stars, actively maintained (latest: Apr 2025) |
| **Asio (standalone)** | 1.36.0 | Async networking I/O | Modern C++ design, cross-platform, no Boost dependency required, excellent C++17/20 coroutine support |
| **llhttp** | 9.3.0 | HTTP/1.x parsing | Node.js's HTTP parser, production-hardened, security-patched, supports all HTTP methods including QUERY |
| **OpenSSL** | 1.1+ / 3.0+ | TLS/SSL for secure transports | Industry standard, required for HTTPS+WSS transports, widest compatibility |

### Transport Layer

| Technology | Version | Purpose | When to Use |
| Library | Version | Purpose | When to Use |
|------------|---------|---------|-------------|
| **WebSocket++** | 0.8.2 | WebSocket transport | Required for WebSocket transport; header-only, integrates with Asio, mature despite 2020 last release |
| **stdio** | (standard) | stdio transport | Use platform stdin/stdout - no library needed |
| **Custom SSE** | (build) | Server-Sent Events | Build custom parser using llhttp; no quality C++ SSE library exists (verified via search) |

### Build System

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| **CMake** | 3.16+ | Build configuration | C++17 standard, FetchContent support, cross-platform, gcc-11 compatible |

### Testing & Development

| Technology | Version | Purpose | When to Use |
| Library | Version | Purpose | When to Use |
|------------|---------|---------|-------------|
| **GoogleTest** | 1.17.0+ | Unit testing framework | C++17 now required (aligns with project), actively maintained, extensive matcher library |
| **MCP Inspector** | (from spec) | Integration testing | Official MCP testing utility for protocol validation |

## Installation

### Core Dependencies via CMake FetchContent

```cmake
# JSON library
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

# HTTP parser (for SSE)
FetchContent_Declare(
    llhttp
    GIT_REPOSITORY https://github.com/nodejs/llhttp.git
    GIT_TAG v9.3.0
)

# WebSocket library
FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
    GIT_TAG 0.8.2
)

# Testing framework
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.17.0
)
```

### System Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev cmake g++-11

# macOS
brew install openssl cmake

# OpenSSL 3.0+ preferred for new projects
```

### Asio Integration

```cmake
# Option 1: Standalone (recommended)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1.36.0
)

# Option 2: System package
# sudo apt-get install libasio-dev  # Version may vary
```

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| JSON | nlohmann/json | json-rpc-cxx | Not a full JSON library; JSON-RPC specific, limited JSON manipulation |
| JSON | nlohmann/json | Boost.JSON | More verbose API, less intuitive; nlohmann is community preference |
| JSON | nlohmann/json | Glaze | Faster but less mature; smaller community, fewer examples |
| Networking | Asio | libevent | Older C-style API; less type-safe; ~40% slower benchmarks in 2024 tests |
| Networking | Asio | libuv | C API; broader scope includes file I/O (not needed); async model less ergonomic in C++ |
| Networking | Asio | Boost.Asio | Full Boost dependency unnecessary; standalone version provides same functionality |
| WebSocket | WebSocket++ | uWebSockets | C++17 required but heavier, more complex; WebSocket++ is header-only and simpler |
| WebSocket | WebSocket++ | Boost.Beast | Powerful but verbose; requires learning curve; WebSocket++ has simpler API |
| HTTP Parser | llhttp | http-parser | Original llhttp replaces it; TypeScript-derived, better maintained |
| HTTP Parser | llhttp | curl | Too heavy; full-featured HTTP client not needed for parsing |
| SSE Parser | Custom (llhttp) | httplib | Full HTTP client library overkill; need just SSE parsing |
| SSE Parser | Custom (llhttp) | cpp-httplib | Same as httplib; parser not cleanly separable |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| **json-rpc-cxx** | JSON-RPC specific library, not general JSON; limits flexibility | nlohmann/json + custom JSON-RPC layer |
| **RapidJSON** | Lower-level API; more verbose; memory management more complex | nlohmann/json (header-only, safer) |
| **Boost.Serialization** | Overkill; complex template metaprogramming; slower compile times | nlohmann/json for MCP protocol |
| **libevent** | Older C API; less type-safe; performance inferior to Asio (2024 benchmarks) | Asio standalone |
| **gSOAP** | Heavyweight; overkill for JSON-RPC; XML-focused originally | Custom JSON-RPC on nlohmann/json |
| **Pistache** | Full HTTP server; overkill; library provides HTTP, users bring server | Asio for transport only |
| **Drogon** | Full web framework; too heavy for a protocol library | Asio + individual components |
| **POCO** | Heavy framework; slower compile; more dependencies than needed | Modular: nlohmann + Asio + OpenSSL |
| **cpp-httplib** | Includes HTTP server; library shouldn't provide HTTP server per constraints | Build HTTP parsing with llhttp, users bring HTTP server |
| **cpr** | HTTP client library; not needed for transport-agnostic design | Use llhttp for parsing, user provides HTTP server |

## Stack Patterns by Variant

### If building ONLY stdio transport:
- Use **nlohmann/json** + **GoogleTest**
- No networking libraries needed
- Minimal dependencies: fastest compile, simplest integration

### If building stdio + SSE:
- Add **llhttp** for HTTP parsing
- Add **OpenSSL** for TLS support
- Build custom SSE parser on top of llhttp

### If building stdio + SSE + WebSocket (full stack):
- All of the above
- Add **WebSocket++** for WebSocket protocol
- Add **Asio** for async networking
- This is the recommended production configuration

## Version Compatibility

| Package A | Compatible With | Notes |
|-----------|-----------------|-------|
| nlohmann/json 3.12.0 | gcc-11, C++17 | Verified in CI; includes std::optional support |
| Asio 1.36.0 | gcc-11, C++17 | Standalone version; no Boost required |
| WebSocket++ 0.8.2 | Asio 1.36.0, C++11 | C++11 minimum, works fine with C++17 |
| llhttp 9.3.0 | C99, C++17 via C wrapper | C library; wrap in C++ interface |
| GoogleTest 1.17.0+ | C++17 required | Breaking change: C++17 now minimum |
| OpenSSL 1.1+ / 3.0+ | All above | Both versions supported; 3.0+ preferred for new projects |

## Async/Non-Blocking Strategy for C++17

Since C++20 coroutines are unavailable, the library should provide:

1. **Callback-based core API** - For streaming and real-time responses
2. **std::future<> wrappers** - For simple request/response patterns
3. **Thread-safe dispatchers** - Using Asio's io_context per thread

```cpp
// Core callback pattern (required for streaming)
template<typename Callback>
void callToolStreaming(const std::string& name, const json& args, Callback cb);

// Future wrapper (convenience for simple RPC)
std::future<json> callTool(const std::string& name, const json& args);
```

## JSON-RPC Layer Strategy

**Do NOT use json-rpc-cxx.** Reasons:
- MCP requires specific JSON-RPC 2.0 extensions (progress tokens, streaming)
- Need tight integration with MCP types
- Want control over serialization for performance

**Build custom JSON-RPC layer** using:
- nlohmann/json for message serialization
- Custom request/response dispatcher
- Direct mapping to MCP schema types

## SSE Parsing Strategy

No quality C++ SSE library exists in 2025. Build custom parser:

```
SSE Format: "data: {}\n\n"
- Use llhttp to parse HTTP headers (Content-Type: text/event-stream)
- Parse SSE events with simple state machine on line boundaries
- Handle: data:, event:, id:, retry: lines
- Respect reconnection with Last-Event-ID
```

Reference implementation can draw from:
- gopher-mcp's sse_codec_filter.cc
- MDN SSE specification (for protocol correctness)

## Sources

### HIGH Confidence (Official Documentation)
- [nlohmann/json Releases](https://github.com/nlohmann/json/releases) - v3.12.0 release (Apr 11, 2025)
- [WebSocket++ Repository](https://github.com/zaphoyd/websocketpp) - v0.8.2 documentation
- [llhttp Releases](https://github.com/nodejs/llhttp/releases) - v9.3.0 release (May 23, 2025)
- [Asio C++ Library](https://think-async.com/) - Official site
- [Asio SourceForge](https://sourceforge.net/projects/asio/) - v1.36.0 downloads
- [GoogleTest Repository](https://github.com/google/googletest) - v1.17.0+ requires C++17

### MEDIUM Confidence (WebSearch + Official Verification)
- [json-rpc-cxx GitHub](https://github.com/jsonrpcx/json-rpc-cxx) - Evaluated as JSON-RPC alternative
- [Reddit: Asio in 2024/2025](https://www.reddit.com/r/cpp_questions/comments/1fi7yfn/asio_is_great_its_2024_why_should_i_use_the_rest_of_boost/) - Community sentiment on Asio vs Boost
- [Asio vs libevent performance comparison (2024)](https://juejin.cn/post/7314485430385295400) - Benchmark: Asio ~800K QPS vs libevent ~500K QPS
- [C++ networking library comparison (2025)](https://blog.csdn.net/tyhenry/article/details/146341229) - Comprehensive benchmark including muduo, Asio, libevent

### LOW CONFIDENCE (WebSearch Only - Requires Verification)
- SSE library searches yielded no C++-specific solutions - build custom parser recommended
- Some alternative libraries (Glaze, rapidjson alternatives) mentioned in community discussions but not officially verified for this use case

### Reference Implementation Analysis
- [gopher-mcp CMakeLists.txt](https://github.com/gopher-mcp/gopher-mcp) - Analyzed for dependency choices (uses libevent, nlohmann/json 3.11.3, llhttp 9.1.3, nghttp2, yaml-cpp)
- Note: gopher-mcp uses libevent; this research recommends Asio instead for modern C++17 codebases

---
*Stack research for: C++ MCP Library (mcpp)*
*Researched: 2025-01-31*
