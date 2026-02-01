# Technology Stack

**Domain:** C++ MCP (Model Context Protocol) Library
**Researched:** 2025-01-31
**Updated:** 2026-02-01 (v1.1 Inspector Integration)
**Overall Confidence:** HIGH

## Executive Summary

The recommended stack for building a C++17 MCP library prioritizes modern, header-only libraries where possible to minimize build complexity while maintaining performance. For v1.1 Inspector integration:

- **No new language dependencies** required - MCP Inspector connects via existing stdio transport
- **Bats-core** recommended for automated CLI testing - TAP-compliant bash testing framework
- **Enhanced logging** using existing Logger for JSON-RPC parse error debugging
- **Example server** with `mcp.json` configuration for Inspector/Cursor/Claude Code compatibility

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| **nlohmann/json** | 3.12.0 | JSON parsing/serialization | De facto standard, header-only, C++17 optimized, 45.8k GitHub stars, actively maintained (latest: Apr 2025) |
| **Asio (standalone)** | 1.36.0 | Async networking I/O | Modern C++ design, cross-platform, no Boost dependency required, excellent C++17/20 coroutine support |
| **llhttp** | 9.3.0 | HTTP/1.x parsing | Node.js's HTTP parser, production-hardened, security-patched, supports all HTTP methods including QUERY |
| **OpenSSL** | 1.1+ / 3.0+ | TLS/SSL for secure transports | Industry standard, required for HTTPS+WSS transports, widest compatibility |

### v1.1 Additions: Inspector Integration & Testing

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| **MCP Inspector (npx)** | latest | Integration testing | Official MCP testing tool; `--cli` mode enables automation; connects via stdio |
| **Bats-core** | 1.11.0+ | CLI test automation | TAP-compliant bash testing; simple syntax; ideal for testing stdio servers |
| **jq** | 1.6+ | JSON parsing in tests | Validate JSON-RPC response structure in bash scripts |

### Transport Layer

| Technology | Version | Purpose | When to Use |
|------------|---------|---------|-------------|
| **WebSocket++** | 0.8.2 | WebSocket transport | Required for WebSocket transport; header-only, integrates with Asio, mature despite 2020 last release |
| **stdio** | (standard) | stdio transport | Use platform stdin/stdout - no library needed; Inspector uses this |
| **Custom SSE** | (build) | Server-Sent Events | Build custom parser using llhttp; no quality C++ SSE library exists (verified via search) |

### Build System

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| **CMake** | 3.16+ | Build configuration | C++17 standard, FetchContent support, cross-platform, gcc-11 compatible |

### Testing & Development

| Technology | Version | Purpose | When to Use |
|------------|---------|---------|-------------|
| **GoogleTest** | 1.17.0+ | Unit testing framework | C++17 now required (aligns with project), actively maintained, extensive matcher library |
| **MCP Inspector** | (from spec) | Integration testing | Official MCP testing utility for protocol validation |
| **Bats-core** | 1.11.0+ | CLI automation | NEW: For automated testing of stdio server via Inspector CLI |

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

### v1.1: Inspector CLI & Bats Installation

```bash
# MCP Inspector - via npx (no install needed)
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server

# Bats-core for CLI testing
# macOS
brew install bats-core

# Ubuntu/Debian
sudo apt install bats

# Or from source
git clone https://github.com/bats-core/bats-core.git /tmp/bats
cd /tmp/bats
sudo ./install.sh /usr/local

# Verify
bats --version  # Should show 1.11.0+

# jq for JSON parsing in tests
sudo apt install jq   # Ubuntu
brew install jq       # macOS
```

### System Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev cmake g++-11 bats jq

# macOS
brew install openssl cmake bats jq

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

## MCP Inspector CLI Integration

### Basic Usage

```bash
# Direct stdio connection
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server

# With configuration file
npx @modelcontextprotocol/inspector --cli --config mcp.json --server mcpp-inspector

# Test specific method
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server \
    --method tools/list

# Call a tool
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server \
    --method tools/call --tool-name calculate --tool-arg operation=add --tool-arg a=5 --tool-arg b=3
```

### Example mcp.json Configuration

```json
{
  "mcpServers": {
    "mcpp-inspector": {
      "command": "/path/to/build/examples/inspector_server",
      "args": [],
      "env": {
        "MCPP_LOG_LEVEL": "debug",
        "MCPP_LOG_PAYLOAD": "true"
      }
    }
  }
}
```

## Automated CLI Testing with Bats

### Test Structure

```
tests/cli/
  bats/                      # Bats test files
    test_inpector_startup.bats
    test_tools_call.bats
    test_resources_list.bats
    test_prompts_list.bats
    test_jsonrpc_errors.bats
  helpers/                    # Test fixtures and helpers
    setup_mcpp_server.bash
    inspect_output.bash
```

### Example Bats Test

```bash
#!/usr/bin/env bats

setup() {
    export MCPP_SERVER="./build/examples/inspector_server"
    [ -f "$MCPP_SERVER" ] || skip "Server not built"
}

@test "tools/list returns valid JSON-RPC" {
    run npx @modelcontextprotocol/inspector --cli "$MCPP_SERVER" --method tools/list

    [ "$status" -eq 0 ]
    # Verify output is valid JSON array
    echo "$output" | jq -e '.result.tools | type == "array"'
}

@test "calculate tool adds numbers" {
    run npx @modelcontextprotocol/inspector --cli "$MCPP_SERVER" \
        --method tools/call --tool-name calculate \
        --tool-arg operation=add --tool-arg a=5 --tool-arg b=3

    [ "$status" -eq 0 ]
    # Verify result contains "8"
    echo "$output" | jq -e '.result.content[0].text | test("8")'
}

@test "malformed JSON returns -32700 error" {
    # This tests the parse error handling
    echo '{"jsonrpc": "2.0", "method": "invalid"' | \
        "$MCPP_SERVER" 2>/dev/null | \
        jq -e '.error.code == -32700'
}
```

### CMake Integration for CLI Tests

```cmake
# Add to tests/CMakeLists.txt
find_program(BATS_EXECUTABLE bats)
if(BATS_EXECUTABLE)
    add_custom_target(cli-tests
        COMMAND ${BATS_EXECUTABLE} tests/cli/bats
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running CLI tests with Bats"
    )
    add_dependencies(test cli-tests)
else()
    message(WARNING "bats not found - CLI tests will be skipped")
endif()
```

## JSON-RPC Parse Error Debugging

### Enhanced Logging Strategy

The existing `Logger` class already supports:
- `set_level(Level)` - Configure log verbosity
- `enable_payload_logging(bool, size_t)` - Log JSON payloads with size limits
- Thread-safe stderr output (not stdout - critical for stdio MCP)

### Example Server Debug Mode

```cpp
// Add to inspector_server.cpp main()
bool debug_mode = false;
for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--debug" || std::string(argv[i]) == "-d") {
        debug_mode = true;
    }
}

if (debug_mode) {
    mcpp::util::logger().set_level(mcpp::util::Logger::Level::Trace);
    mcpp::util::logger().enable_payload_logging(true, 2048);
    std::cerr << "=== DEBUG MODE ENABLED ===" << std::endl;
}
```

### Common Parse Error -32700 Causes

| Cause | Detection | Prevention |
|-------|-----------|-------------|
| **Non-JSON output to stdout** | Inspector shows "Parse error" immediately | Use `std::cerr` for all logging, never `std::cout` |
| **Incomplete JSON lines** | Error shows truncated JSON | Always terminate with `std::endl` or `\n` |
| **Invalid JSON syntax** | nlohmann::json::exception | Use `json::accept()` for validation |
| **Encoding issues** | Garbled characters in output | Ensure UTF-8 encoding only |

### Debug Workflow

```bash
# 1. Enable debug logging
MCPP_LOG_LEVEL=trace MCPP_LOG_PAYLOAD=true \
    ./build/examples/inspector_server 2>debug.log

# 2. In another terminal, run Inspector
npx @modelcontextprotocol/inspector --cli ./build/examples/inspector_server

# 3. Review debug.log for exact bytes sent/received
# 4. Check for any stdout pollution (grep for non-JSON lines)
```

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| JSON | nlohmann/json | json-rpc-cxx | Not a full JSON library; JSON-RPC specific, limited JSON manipulation |
| JSON | nlohmann/json | Boost.JSON | More verbose API, less intuitive; nlohmann is community preference |
| JSON | nlohmann/json | Glaze | Faster but less mature; smaller community, fewer examples |
| CLI Testing | Bats-core | Python pytest + subprocess | Adds Python dependency; bash sufficient for CLI testing |
| CLI Testing | Bats-core | ShellSpec | Newer, less mature than Bats-core |
| CLI Testing | Bats-core | GoogleTest CLI wrapper | Overkill; adds C++ compilation for test changes |
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
| **New JSON library** | nlohmann/json is standard, well-maintained, and already integrated | Use existing nlohmann/json |
| **RPC framework (gRPC, Thrift)** | MCP uses JSON-RPC 2.0 over stdio; no binary RPC needed | Custom JSON-RPC on nlohmann/json |
| **HTTP server library (cpp-httplib)** | Only needed if hosting Inspector UI locally; npx handles this | Use npx Inspector |
| **Python testing framework** | Adds dependency; Bats-core is sufficient for CLI testing | Bats-core for bash tests |
| **spdlog as required dependency** | Currently optional; keep optional but recommend for debug builds | Use existing Logger (stderr fallback) |
| **WebSocket library** | MCP uses stdio/SSE/HTTP for Inspector; WebSocket not needed for v1.1 | Existing stdio transport |

## Stack Patterns by Variant

### If building ONLY stdio transport:
- Use **nlohmann/json** + **GoogleTest** + **Bats-core**
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
| Bats-core 1.11.0+ | Bash 3.2+ | TAP-compliant; integrates with CI/CD |
| MCP Inspector | Node.js 22.7.5+ | Via npx; no install required |

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
- [MCP Inspector GitHub](https://github.com/modelcontextprotocol/inspector) - Official Inspector CLI documentation
- [MCP Inspector Docs](https://modelcontextprotocol.io/docs/tools/inspector) - Official usage guide
- [Bats-core GitHub](https://github.com/bats-core/bats-core) - Bash Automated Testing System
- [Bats-core Documentation](https://bats-core.readthedocs.io/) - Official docs

### MEDIUM Confidence (WebSearch + Official Verification)
- [json-rpc-cxx GitHub](https://github.com/jsonrpcx/json-rpc-cxx) - Evaluated as JSON-RPC alternative
- [Reddit: Asio in 2024/2025](https://www.reddit.com/r/cpp_questions/comments/1fi7yfn/asio_is_great_its_2024_why_should_i_use_the_rest_of_boost/) - Community sentiment on Asio vs Boost
- [Asio vs libevent performance comparison (2024)](https://juejin.cn/post/7314485430385295400) - Benchmark: Asio ~800K QPS vs libevent ~500K QPS
- [C++ networking library comparison (2025)](https://blog.csdn.net/tyhenry/article/details/146341229) - Comprehensive benchmark including muduo, Asio, libevent
- [MCP CLI Updates (January 2026)](https://www.philschmid.de/mcp-cli) - v0.3.0 features
- [Understanding MCP Through Raw STDIO](https://foojay.io/today/understanding-mcp-through-raw-stdio-communication/) - stdio debugging patterns
- [MCP Debugging Best Practices](https://www.mcpevals.io/blog/debugging-mcp-servers-tips-and-best-practices) - Parse error causes
- [StackOverflow: MCP parse errors](https://stackoverflow.com/questions/79550897/mcp-server-always-get-initialization-error) - Common stdio issues

### LOW CONFIDENCE (WebSearch Only - Requires Verification)
- SSE library searches yielded no C++-specific solutions - build custom parser recommended
- Some alternative libraries (Glaze, rapidjson alternatives) mentioned in community discussions but not officially verified for this use case

### Reference Implementation Analysis
- [gopher-mcp CMakeLists.txt](https://github.com/gopher-mcp/gopher-mcp) - Analyzed for dependency choices (uses libevent, nlohmann/json 3.11.3, llhttp 9.1.3, nghttp2, yaml-cpp)
- Note: gopher-mcp uses libevent; this research recommends Asio instead for modern C++17 codebases

---
*Stack research for: C++ MCP Library (mcpp)*
*Researched: 2025-01-31*
*Updated: 2026-02-01 (v1.1 Inspector Integration)*
