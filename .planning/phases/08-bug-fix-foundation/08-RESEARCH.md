# Phase 8: Bug Fix Foundation - Research

**Researched:** 2026-02-01
**Domain:** JSON-RPC 2.0 validation, MCP stdio protocol compliance, C++ library design
**Confidence:** HIGH

## Summary

This phase addresses three critical library-layer fixes for MCP (Model Context Protocol) stdio compliance: (1) proper JSON-RPC request validation via `JsonRpcRequest::from_json()`, (2) newline-delimited JSON output helpers for stdio protocol compliance, and (3) debug output routing to stderr only. The fixes are library-layer only—no external dependencies, no application code changes. The example server (Phase 9) will consume these fixes.

**Primary recommendation:** Mirror the existing `JsonRpcResponse::from_json()` pattern for `JsonRpcRequest::from_json()`, add `to_string_delimited()` helpers to all JSON-RPC types, and create `MCPP_DEBUG_LOG` macro for stderr-only debug output.

## Standard Stack

The library uses existing dependencies only—no new libraries required for this phase.

### Core (Existing)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| nlohmann/json | 3.11+ | JSON parsing/dumping | Header-only, fast, widely adopted, already in use |
| C++20 stdlib | C++20 | std::optional, std::variant, std::fprintf | Required for `from_json()` return type and variant ID handling |

### Supporting (Existing)
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| spdlog | 1.8+ (optional) | Structured logging backend | Already optional with stderr fallback—configure to stderr-only |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| std::optional | Exceptions, tl::expected | std::optional matches existing `JsonRpcResponse::from_json()` pattern; exceptions add overhead, tl::expected is external dependency |
| `MCPP_DEBUG_LOG` macro | Direct std::fprintf | Macro centralizes debug output, allows future feature flagging without code changes |

**Installation:** No new dependencies—use existing stack.

## Architecture Patterns

### Recommended Project Structure
```
src/mcpp/core/
├── json_rpc.h          # Add JsonRpcRequest::from_json(), to_string_delimited() to all types
├── json_rpc.cpp        # Implement from_json(), ParseError diagnostics
└── error.h             # Add JsonRpcRequest::ParseError struct

src/mcpp/util/
├── logger.h            # Add MCPP_DEBUG_LOG macro declaration
└── logger.cpp          # Already routes to stderr (verified line 264)
```

### Pattern 1: Validation via Static from_json() Method

**What:** Mirror `JsonRpcResponse::from_json()` to add `JsonRpcRequest::from_json()` as static public method.

**When to use:** Anytime incoming JSON needs validation before processing.

**Example:**
```cpp
// Source: Existing pattern in src/mcpp/core/json_rpc.cpp lines 53-115
static std::optional<JsonRpcResponse> JsonRpcResponse::from_json(const JsonValue& j) {
    try {
        // Check for jsonrpc field
        if (!j.contains("jsonrpc") || !j["jsonrpc"].is_string()) {
            return std::nullopt;
        }
        std::string jsonrpc_version = j["jsonrpc"].get<std::string>();
        if (jsonrpc_version != "2.0") {
            return std::nullopt;
        }
        // ... rest of validation
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

// NEW: Add to JsonRpcRequest following same pattern
static std::optional<JsonRpcRequest> JsonRpcRequest::from_json(const JsonValue& j);
```

**Validation checklist for `JsonRpcRequest::from_json()`:**
1. `jsonrpc` field exists and equals `"2.0"`
2. `id` field exists (string, number, or null per spec)
3. `method` field exists and is string
4. `params` field (if present) is object or array, not null/string/number
5. Return `std::nullopt` on any validation failure (no exceptions)

### Pattern 2: ParseError Struct for Diagnostics

**What:** Lightweight diagnostics struct for validation failures without echoing malicious input.

**When to use:** When `from_json()` returns `nullopt` and caller needs error details for logging/debugging.

**Example:**
```cpp
// Source: Designed for this phase
struct JsonRpcRequest::ParseError {
    enum class Code {
        MissingJsonrpc,
        InvalidJsonrpcVersion,
        MissingId,
        InvalidIdType,
        MissingMethod,
        InvalidMethodType,
        InvalidParamsType,
        MalformedJson
    };

    Code code;
    std::string message;           // Human-readable, NO raw JSON content
    std::optional<RequestId> id;   // Extracted if available for error response

    // Factory methods for common errors
    static ParseError missing_jsonrpc() {
        return {Code::MissingJsonrpc, "Missing required 'jsonrpc' field", std::nullopt};
    }
    static ParseError invalid_params_type() {
        return {Code::InvalidParamsType, "'params' must be object or array", std::nullopt};
    }
};
```

**Security consideration:** Never include raw JSON content in `message`—avoid echoing potentially malicious input in error responses or logs.

### Pattern 3: Centralized Newline Delimiter Helper

**What:** Add `to_string_delimited()` method to all JSON-RPC types that returns `.dump() + "\n"`.

**When to use:** All stdio transport output. Keep `to_string()` for non-stdio use cases (HTTP, testing).

**Example:**
```cpp
// Source: Designed for this phase
// In JsonRpcRequest, JsonRpcResponse, JsonRpcNotification
std::string to_string_delimited() const {
    return to_json().dump() + "\n";
}

// Usage in example server (Phase 9):
// std::cout << response.to_string_delimited();  // Includes newline
// Instead of: std::cout << response.to_string() << std::endl;
```

**Why not flush in helper:** Let caller control flushing—some use cases may want to buffer multiple writes. Example server can call `std::cout << ... << std::flush` when needed.

### Pattern 4: Debug Logging Macro

**What:** Centralized macro `MCPP_DEBUG_LOG(fmt, ...)` that expands to `std::fprintf(stderr, ...)`.

**When to use:** Temporary debug output, diagnostics in library code paths. Use existing `Logger` for structured logging.

**Example:**
```cpp
// Source: Designed for this phase
// In src/mcpp/util/logger.h or new debug.h
#ifndef MCPP_DEBUG_LOG
#define MCPP_DEBUG_LOG(fmt, ...) std::fprintf(stderr, "[MCPP_DEBUG] " fmt "\n", ##__VA_ARGS__)
#endif

// Usage:
MCPP_DEBUG_LOG("Request validation failed: code=%d", static_cast<int>(error.code));
```

**Alternative naming considered:**
- `MCPP_LOG_DEBUG`—more consistent with `Logger::debug()`
- `MCPP_TRACE`—implies lower severity than debug
- `MCPP_DEBUG_LOG`—selected for clarity

**Decision:** Use `MCPP_DEBUG_LOG` (Claude's discretion). Planner should lock this choice.

### Anti-Patterns to Avoid
- **Echoing raw JSON in errors:** Security risk—malformed input could be malicious. Use error codes and sanitized messages only.
- **Mixed stdout usage:** Never write debug to `std::cout` in library code—pollutes stdio protocol. Use stderr only.
- **Inconsistent delimiters:** Don't mix `"\n"`, `"\r\n"`, `std::endl`—MCP spec requires newline (`"\n"`) only.
- **Exception-based validation:** Existing codebase uses `std::optional` for validation errors (`JsonRpcResponse::from_json()`)—follow that pattern.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| JSON validation | Custom field checks, string parsing | nlohmann/json `contains()`, `is_string()`, `is_number()` | Handles type checking, exception safety, nested validation |
| Newline delimiting | Custom string concatenation in every call site | `to_string_delimited()` helper | Centralized, testable, ensures consistency across all JSON-RPC types |
| Request ID extraction | Manual string parsing (like inspector_server.cpp lines 367-401) | `JsonRpcRequest::from_json()` with ParseError | Type-safe, handles all valid ID formats (string, number, null) |
| Debug output routing | `std::cout` with conditional compilation | `MCPP_DEBUG_LOG` macro or `Logger` | Guarantees stderr-only, no stdout pollution possible |

**Key insight:** The example server's manual ID extraction (lines 367-401) is fragile—a proper `from_json()` validates structure and extracts ID in one pass.

## Common Pitfalls

### Pitfall 1: Stdout Protocol Pollution

**What goes wrong:** Debug or diagnostic output written to `stdout` gets parsed as JSON-RPC messages by MCP clients, causing protocol errors.

**Why it happens:** `std::cout` is the default for many C++ logging patterns; developers forget stdio transport reserves `stdout` exclusively for JSON-RPC messages.

**How to avoid:**
1. Use `MCPP_DEBUG_LOG` macro (writes to stderr)
2. Use existing `Logger` class (already routes to stderr, verified line 264)
3. For third-party libraries (spdlog), configure sinks to stderr or file only

**Warning signs:** MCP clients report "parse error" on messages that look like debug text, or clients hang waiting for valid JSON-RPC response.

### Pitfall 2: Missing Newline Delimiters

**What goes wrong:** JSON-RPC messages without trailing newline cause clients to hang waiting for complete message—stdio transport reads line-by-line.

**Why it happens:** `nlohmann::json::dump()` does NOT add trailing newline; developer must add it explicitly.

**How to avoid:**
1. Use `to_string_delimited()` for all stdio output
2. Verify with `str.ends_with('\n')` in tests
3. Set `std::cout` to unbuffered mode if needed (rare)

**MCP specification requirement (HIGH confidence):**
> "Messages are delimited by newlines, and MUST NOT contain embedded newlines."
> — [MCP Transports Specification](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports)

**Warning signs:** Inspector or other MCP clients appear to "hang" after sending request.

### Pitfall 3: Insufficient Request Validation

**What goes wrong:** Malformed requests cause crashes, undefined behavior, or generic error messages that don't help debugging.

**Why it happens:** Code assumes valid JSON-RPC structure without checking field presence/types before accessing.

**How to avoid:**
1. Always use `JsonRpcRequest::from_json()` before processing incoming requests
2. Validate: `jsonrpc == "2.0"`, `id` present, `method` present and is string, `params` is object/array if present
3. Return `std::nullopt` on validation failure (don't throw)

**Warning signs:** Segmentation faults on malformed input, `nlohmann::json::exception` in logs.

### Pitfall 4: Extracting Request ID from Malformed JSON

**What goes wrong:** Parse error responses have `id: null` because extracting ID from malformed JSON is fragile.

**Why it happens:** When JSON parsing fails, you can't use `nlohmann::json` to access fields—must fall back to string manipulation (as inspector_server.cpp does).

**How to avoid:**
1. `from_json()` attempts full validation first
2. If validation fails, try to extract `id` field using same nlohmann::json before throwing
3. If nlohmann::json throws, fall back to `std::nullopt` for ID
4. ParseError includes extracted ID (if any) for error response construction

**Example of fragile pattern to avoid:** inspector_server.cpp lines 367-401 (manual string search for `"id"`). The library's `from_json()` should be more robust.

### Pitfall 5: spdlog Writing to Stdout

**What goes wrong:** If spdlog is configured with default console sink, it may write to `stdout`, polluting the stdio protocol.

**Why it happens:** spdlog's default sink pattern varies; not all configurations specify stderr.

**How to avoid:**
1. Configure spdlog sinks explicitly to stderr or file
2. Or use library's built-in Logger (already has stderr fallback at line 264)
3. Test with `MCPP_DEBUG_LOG` to verify stderr-only output

**Verification:** Run library with debug enabled and check `/proc/self/fd/1` (stdout) vs `/proc/self/fd/2` (stderr).

## Code Examples

Verified patterns from official sources and existing codebase:

### Existing Pattern: JsonRpcResponse::from_json()

```cpp
// Source: src/mcpp/core/json_rpc.cpp lines 53-115
std::optional<JsonRpcResponse> JsonRpcResponse::from_json(const JsonValue& j) {
    try {
        // Check for jsonrpc field
        if (!j.contains("jsonrpc") || !j["jsonrpc"].is_string()) {
            return std::nullopt;
        }
        std::string jsonrpc_version = j["jsonrpc"].get<std::string>();
        if (jsonrpc_version != "2.0") {
            return std::nullopt;
        }

        // Check for id field
        if (!j.contains("id")) {
            return std::nullopt;
        }
        auto id_opt = detail::parse_request_id(j["id"]);
        if (!id_opt) {
            return std::nullopt;
        }

        JsonRpcResponse response;
        response.jsonrpc = jsonrpc_version;
        response.id = *id_opt;

        // Check for result OR error (mutually exclusive per spec)
        bool has_result = j.contains("result");
        bool has_error = j.contains("error");

        if (has_result && has_error) {
            return std::nullopt;  // Both present - invalid per spec
        }

        if (has_result) {
            response.result = j["result"];
        } else if (has_error) {
            auto error_opt = JsonRpcError::from_json(j["error"]);
            if (!error_opt) {
                return std::nullopt;
            }
            response.error = *error_opt;
        } else {
            return std::nullopt;  // Neither result nor error
        }

        return response;

    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}
```

### New Pattern: JsonRpcRequest::from_json() (to implement)

```cpp
// Designed for this phase, mirrors JsonRpcResponse::from_json()
static std::optional<JsonRpcRequest> JsonRpcRequest::from_json(const JsonValue& j) {
    try {
        // Check for jsonrpc field
        if (!j.contains("jsonrpc") || !j["jsonrpc"].is_string()) {
            return std::nullopt;
        }
        std::string jsonrpc_version = j["jsonrpc"].get<std::string>();
        if (jsonrpc_version != "2.0") {
            return std::nullopt;
        }

        // Check for id field (required for requests)
        if (!j.contains("id")) {
            return std::nullopt;
        }
        auto id_opt = detail::parse_request_id(j["id"]);
        if (!id_opt) {
            return std::nullopt;
        }

        // Check for method field
        if (!j.contains("method") || !j["method"].is_string()) {
            return std::nullopt;
        }

        JsonRpcRequest request;
        request.jsonrpc = jsonrpc_version;
        request.id = *id_opt;
        request.method = j["method"].get<std::string>();

        // Check for params (optional, but must be object or array if present)
        if (j.contains("params")) {
            const JsonValue& params = j["params"];
            if (!params.is_object() && !params.is_array()) {
                return std::nullopt;
            }
            request.params = params;
        }

        return request;

    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}
```

### New Pattern: to_string_delimited() (to implement)

```cpp
// Designed for this phase
// In JsonRpcRequest, JsonRpcResponse, JsonRpcNotification
std::string to_string_delimited() const {
    return to_json().dump() + "\n";
}
```

### Existing Pattern: Stderr Routing (already correct)

```cpp
// Source: src/mcpp/util/logger.cpp line 264
void Logger::log_impl(Level level, const std::string& formatted_message) {
#if MCPP_HAS_SPDLOG
    try {
        // ... spdlog logging ...
        return;
    } catch (...) {
        // Fall through to stderr on spdlog error
    }
#endif

    // Fallback to stderr
    std::cerr << formatted_message << std::endl;
}
```

### Existing Pattern: Stdio Transport Newline Handling

```cpp
// Source: src/mcpp/transport/stdio_transport.cpp lines 101-114
bool StdioTransport::send(std::string_view message) {
    if (!pipe_ || !running_) {
        return false;
    }

    // Append newline delimiter per MCP spec
    std::string full_message(message);
    full_message += '\n';

    size_t written = fwrite(full_message.data(), 1, full_message.size(), pipe_);
    fflush(pipe_);

    return written == full_message.size();
}
```

**Note:** The transport layer adds newlines, but JSON-RPC types should also provide `to_string_delimited()` for direct use (e.g., example server writing to `std::cout` without going through `StdioTransport`).

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual JSON string parsing | nlohmann/json with type-safe validation | Library inception (v1.0) | Type safety, exception handling |
| `std::cout << response << std::endl` | `to_string_delimited()` helper | This phase (v1.1) | Centralized delimiter handling |
| Fragile manual ID extraction | `from_json()` with ParseError diagnostics | This phase (v1.1) | Robust validation, better error messages |
| Debug output to stdout | `MCPP_DEBUG_LOG` to stderr | This phase (v1.1) | Protocol compliance |

**Deprecated/outdated:**
- **Example server's manual ID extraction** (inspector_server.cpp lines 367-401): Replaced by `JsonRpcRequest::from_json()` + ParseError in Phase 9
- **Direct `std::cout` usage for JSON output**: Use `to_string_delimited()` instead

## Open Questions

1. **ParseError factory method granularity**
   - What we know: Need factory methods for common validation failures
   - What's unclear: Should we have one factory per validation rule, or group related failures?
   - Recommendation: Start with one factory per validation rule (missing_jsonrpc, invalid_method, invalid_params, etc.)—caller can group if needed. Add more in Phase 9 if real-world usage shows gaps.

2. **Should `to_string_delimited()` also flush?**
   - What we know: `StdioTransport::send()` calls `fflush()` after write
   - What's unclear: Should the string helper also flush, or leave that to caller?
   - Recommendation: Don't flush in helper. Let caller decide—some use cases (batching, testing) don't want immediate flush. Example server can do `std::cout << msg.to_string_delimited() << std::flush` if needed.

3. **Macro naming: `MCPP_DEBUG_LOG` vs `MCPP_LOG_DEBUG`**
   - What we know: Need macro for stderr debug output
   - What's unclear: Which naming style is more consistent with codebase conventions?
   - Recommendation: Use `MCPP_DEBUG_LOG` (selected by discretion). If planner disagrees, can change to `MCPP_LOG_DEBUG` to match `Logger::debug()` pattern.

## Sources

### Primary (HIGH confidence)
- [MCP Transports Specification (2025-11-25)](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports) - Stdio protocol requirements, newline delimiter specification, stderr allowance for logging
- [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification) - Request validation rules, error response format
- `src/mcpp/core/json_rpc.cpp` (lines 53-115) - Existing `JsonRpcResponse::from_json()` pattern to mirror
- `src/mcpp/util/logger.cpp` (line 264) - Verified stderr fallback implementation
- `src/mcpp/transport/stdio_transport.cpp` (lines 101-114) - Existing newline delimiter handling

### Secondary (MEDIUM confidence)
- [Understanding MCP Through Raw STDIO Communication](https://foojay.io/today/understanding-mcp-through-raw-stdio-communication/) - Confirms newline-delimited JSON-RPC over stdio
- [MCP Server Development Guide](https://github.com/cyanheads/model-context-protocol-resources/blob/main/guides/mcp-server-development-guide.md) - Stdio transport best practices
- [JSON-RPC Best Practices](https://json-rpc.dev/learn/best-practices) - Error handling, validation patterns

### Tertiary (LOW confidence)
- [Building a Modern C++23 JSON-RPC 2.0 Library](https://medium.com/@pooriayousefi/building-a-modern-c-23-json-rpc-2-0-library-b62c4826769d) - C++-specific JSON-RPC implementation insights (verified against existing codebase patterns)
- `examples/inspector_server.cpp` (lines 367-401) - Example of what NOT to do (fragile manual ID extraction)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - No new dependencies, existing codebase patterns verified
- Architecture: HIGH - Based on existing `JsonRpcResponse::from_json()` pattern, official MCP spec
- Pitfalls: HIGH - Official MCP spec explicitly requires newline delimiters and stderr-only logging, existing code shows stdout pollution risk

**Research date:** 2026-02-01
**Valid until:** 2026-03-01 (30 days - stable domain, MCP spec is authoritative)
