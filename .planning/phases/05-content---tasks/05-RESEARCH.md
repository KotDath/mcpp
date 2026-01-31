# Phase 5: Content & Tasks - Research

**Researched:** 2026-01-31
**Domain:** MCP 2025-11-25 content types, pagination, and task API
**Confidence:** HIGH

## Summary

This phase implements rich content type handling (text, image, audio, resource links, embedded resources) with annotations, cursor-based pagination for all list operations, list changed notifications, and the experimental tasks API for long-running operations.

The MCP 2025-11-25 specification defines a comprehensive content type system where `ContentBlock` is a variant type supporting text, images, audio, resource references, and embedded resources. All content types can optionally include `Annotations` for metadata (audience, priority, lastModified). Pagination uses opaque cursor-based semantics where servers provide `nextCursor` tokens for result continuation. Tasks provide a requestor-driven polling model for deferred execution with status tracking, TTL management, and result retrieval.

**Primary recommendation:** Extend the existing `ContentBlock` in `client/sampling.h` to a shared namespace, add content types following the TypeScript schema structure, implement cursor-based pagination in list methods, use the callback pattern from `RootsManager` for list_changed notifications, and implement `TaskManager` class following the registry pattern for task lifecycle management.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| nlohmann/json | 3.11+ | JSON serialization/deserialization | Already used throughout mcpp |
| nlohmann/json-schema-validator | Latest | JSON Schema validation | Already used for tool input/output validation |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| std::variant | C++17 | ContentBlock type discrimination | Type-safe content handling |
| std::optional | C++17 | Optional fields (cursor, annotations, ttl) | Clean API for optional values |
| std::string_view | C++17 | Opaque cursor passing | Zero-copy cursor handling |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| std::variant | std::shared_ptr<ContentBase> | Variant has better performance, no heap allocation |
| std::string | Custom Cursor type | Opaque string matches spec, simpler |

**Installation:**
No new dependencies. All required libraries are already in use.

## Architecture Patterns

### Recommended Project Structure
```
src/mcpp/
├── content/
│   ├── content.h          # Content types (TextContent, ImageContent, etc.)
│   └── annotations.h      # Annotations struct
├── server/
│   ├── tool_registry.h    # Extended with list_changed support
│   ├── resource_registry.h # Extended with list_changed support
│   ├── prompt_registry.h   # Extended with list_changed support
│   └── task_manager.h      # NEW: Task lifecycle management
└── protocol/
    └── capabilities.h      # Extended with tasks capability
```

### Pattern 1: ContentBlock Variant Extension

**What:** Extend the existing `ContentBlock` variant from `client/sampling.h` to a shared namespace with additional content types.

**When to use:** For all content handling in tools, resources, prompts, and sampling.

**Example:**
```cpp
// Source: MCP 2025-11-25 schema, lines 1741-1831
namespace mcpp::content {

struct TextContent {
    std::string type = "text";
    std::string text;
    std::optional<Annotations> annotations;
};

struct ImageContent {
    std::string type = "image";
    std::string data;      // base64-encoded
    std::string mimeType;  // e.g., "image/png", "image/jpeg"
    std::optional<Annotations> annotations;
};

struct AudioContent {
    std::string type = "audio";
    std::string data;      // base64-encoded
    std::string mimeType;  // e.g., "audio/mp3", "audio/wav"
    std::optional<Annotations> annotations;
};

struct ResourceLink {
    std::string type = "resource";
    std::string uri;
    std::optional<Annotations> annotations;
};

struct EmbeddedResource {
    std::string type = "embedded";
    ResourceContent resource;  // Text or Blob contents
    std::optional<Annotations> annotations;
};

using ContentBlock = std::variant<TextContent, ImageContent, AudioContent,
                                   ResourceLink, EmbeddedResource>;

} // namespace mcpp::content
```

### Pattern 2: Cursor-Based Pagination

**What:** Wrap list results with pagination metadata including `nextCursor` and optional `total`.

**When to use:** For all list operations (tools/list, resources/list, prompts/list, tasks/list).

**Example:**
```cpp
// Paginated list result wrapper
template<typename T>
struct PaginatedResult {
    std::vector<T> items;
    std::optional<std::string> nextCursor;  // Present if more results exist
    std::optional<uint64_t> total;          // Optional total count

    // Helper to check if more pages exist
    bool has_more() const noexcept { return nextCursor.has_value(); }
};

// Registry methods accept optional cursor
class ToolRegistry {
public:
    // Existing method (non-paginated)
    std::vector<nlohmann::json> list_tools() const;

    // NEW: Paginated version
    PaginatedResult<nlohmann::json> list_tools(
        const std::optional<std::string>& cursor
    ) const;
};
```

### Pattern 3: List Changed Notifications (RootsManager Pattern)

**What:** Registries send notifications when their contents change, following the existing `RootsManager` callback pattern.

**When to use:** When tools, resources, or prompts are registered/unregistered/modified.

**Example:**
```cpp
// Source: src/mcpp/client/roots.h, lines 129-202
class ToolRegistry {
public:
    using NotifyCallback = std::function<void()>;

    // Set callback for sending notifications
    void set_notify_callback(NotifyCallback cb);

    // Send notification when list changes
    void notify_changed();

private:
    NotifyCallback notify_cb_;

    // Automatically call notify_changed() after modifications
    bool register_tool(...) {
        bool result = tools_.emplace(...).second;
        if (result) notify_changed();
        return result;
    }
};
```

### Pattern 4: Task Lifecycle Management

**What:** A `TaskManager` class that tracks task state, handles TTL expiration, and provides polling/results.

**When to use:** For any request-augmented operations that support task execution.

**Example:**
```cpp
// Source: MCP 2025-11-25 tasks spec, Rust SDK patterns
namespace mcpp::server {

enum class TaskStatus {
    Working,
    InputRequired,
    Completed,
    Failed,
    Cancelled
};

struct Task {
    std::string task_id;
    TaskStatus status;
    std::optional<std::string> status_message;
    std::string created_at;        // ISO 8601
    std::string last_updated_at;   // ISO 8601
    std::optional<uint64_t> ttl;   // Milliseconds, null = unlimited
    std::optional<uint64_t> poll_interval;
};

class TaskManager {
public:
    // Create a new task
    std::string create_task(uint64_t ttl_ms);

    // Get task status
    std::optional<Task> get_task(const std::string& task_id) const;

    // Update task status
    bool update_status(const std::string& task_id, TaskStatus status,
                       const std::optional<std::string>& message);

    // Set/get result for completed task
    bool set_result(const std::string& task_id, const nlohmann::json& result);
    std::optional<nlohmann::json> get_result(const std::string& task_id) const;

    // Cancel a task
    bool cancel_task(const std::string& task_id);

    // List tasks with pagination
    PaginatedResult<Task> list_tasks(
        const std::optional<std::string>& cursor = std::nullopt
    ) const;

    // Clean up expired tasks (call periodically)
    void cleanup_expired();

private:
    std::unordered_map<std::string, Task> tasks_;
    std::unordered_map<std::string, nlohmann::json> results_;
    mutable std::mutex mutex_;  // Thread-safe task access
};

} // namespace mcpp::server
```

### Anti-Patterns to Avoid

- **Binary content as raw bytes**: Always use base64-encoded strings for image/audio data. The spec requires base64 (`@format byte` in schema).
- **Parsing cursors**: Treat cursors as opaque tokens. Don't attempt to decode offset/ID from cursor strings.
- **Blocking in task creation**: Return `CreateTaskResult` immediately, don't wait for task completion.
- **Assuming notification delivery**: Clients may not receive status notifications; implement polling fallback.
- **Hardcoded task IDs**: Use UUID v4 or cryptographically random strings for task IDs.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Base64 encoding | Custom base64 functions | OpenSSL's BIO_f_base64 or lightweight implementation | Proper padding, error handling |
| UUID generation | Random string + dash format | Standard UUID library or format | RFC 4122 compliance |
| ISO 8601 timestamps | String concatenation | std::strftime with format string | Timezone handling, standard format |
| Thread-safe task map | Custom locking | std::unordered_map + std::mutex | Simple, proven pattern |
| JSON schema for content | Hand-coded validation | nlohmann/json already validates | Consistency with existing code |

**Key insight:** The library already uses nlohmann/json for all JSON operations. Extend this pattern rather than introducing new serialization libraries.

## Common Pitfalls

### Pitfall 1: Content Type Discrimination

**What goes wrong:** Forgetting to check the `type` field before accessing content-specific fields.

**Why it happens:** `std::variant` requires correct type indexing or std::visit usage.

**How to avoid:** Always use helper functions for content serialization/deserialization:

```cpp
// WRONG - direct access without type check
json j = content["data"];  // May not exist for TextContent

// RIGHT - use std::visit
std::visit([](const auto& c) {
    if constexpr (std::is_same_v<std::decay_t<decltype(c)>, ImageContent>) {
        // Safe to access c.data
    }
}, content_block);
```

**Warning signs:** Compiler errors about accessing non-existent members, runtime errors from missing JSON keys.

### Pitfall 2: Cursor Reuse Across Sessions

**What goes wrong:** Client saves a cursor from one session and uses it in another.

**Why it happens:** Cursors look like stable tokens but may encode session state.

**How to avoid:** Document that cursors are session-bound. Consider encoding session ID in server cursors.

**Warning signs:** Invalid cursor errors after reconnection, pagination getting stuck.

### Pitfall 3: Task Result Lifetime

**What goes wrong:** Client requests result after TTL expiration, gets "task not found" error.

**Why it happens:** Server is compliant to delete expired tasks.

**How to avoid:** Clients should retrieve results immediately after task completion. Consider storing important results client-side.

**Warning signs:** Intermittent "task not found" errors, especially for long-running tasks.

### Pitfall 4: List Changed Notification Spam

**What goes wrong:** Notification sent on every registration, flooding clients during bulk setup.

**Why it happens:** Registry calls `notify_changed()` after each `register_*()` call.

**How to avoid:** Add `bool notify = true` parameter to registration methods, or provide batch registration:

```cpp
// Suppress notifications during bulk setup
registry.set_notifications_enabled(false);
for (auto& tool : tools) {
    registry.register_tool(tool);
}
registry.set_notifications_enabled(true);
registry.notify_changed();  // Single notification
```

**Warning signs:** Clients receiving excessive list_changed notifications during initialization.

### Pitfall 5: Task Status Transition Violations

**What goes wrong:** Task transitions from `completed` back to `working`, or from `cancelled` to any other state.

**Why it happens:** Improper state machine implementation.

**How to avoid:** Enforce valid state transitions:

```cpp
bool TaskManager::update_status(const std::string& id, TaskStatus new_status) {
    Task& task = tasks_.at(id);

    // Validate state transitions
    switch (task.status) {
        case TaskStatus::Working:
            if (new_status == TaskStatus::Completed ||
                new_status == TaskStatus::Failed ||
                new_status == TaskStatus::Cancelled ||
                new_status == TaskStatus::InputRequired) {
                break;  // Valid
            }
            return false;
        case TaskStatus::InputRequired:
            if (new_status == TaskStatus::Working ||
                new_status == TaskStatus::Completed ||
                new_status == TaskStatus::Failed ||
                new_status == TaskStatus::Cancelled) {
                break;  // Valid
            }
            return false;
        case TaskStatus::Completed:
        case TaskStatus::Failed:
        case TaskStatus::Cancelled:
            // Terminal states - no transitions allowed
            return false;
    }

    task.status = new_status;
    task.last_updated_at = get_timestamp();
    return true;
}
```

**Warning signs:** Client confusion about task state, unexpected task behavior.

## Code Examples

### Content Serialization/Deserialization

```cpp
// Source: Based on MCP 2025-11-25 schema and existing sampling.h patterns
namespace mcpp::content {

inline nlohmann::json content_to_json(const ContentBlock& content) {
    return std::visit([](const auto& c) -> nlohmann::json {
        nlohmann::json j;
        j["type"] = c.type;

        if constexpr (std::is_same_v<std::decay_t<decltype(c)>, TextContent>) {
            j["text"] = c.text;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(c)>, ImageContent>) {
            j["data"] = c.data;
            j["mimeType"] = c.mimeType;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(c)>, AudioContent>) {
            j["data"] = c.data;
            j["mimeType"] = c.mimeType;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(c)>, ResourceLink>) {
            j["uri"] = c.uri;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(c)>, EmbeddedResource>) {
            j["resource"] = resource_content_to_json(c.resource);
        }

        if (c.annotations.has_value()) {
            j["annotations"] = annotations_to_json(*c.annotations);
        }

        return j;
    }, content);
}

} // namespace mcpp::content
```

### Pagination in List Operations

```cpp
// Server-side pagination for tools/list
PaginatedResult<nlohmann::json> ToolRegistry::list_tools_paginated(
    const std::optional<std::string>& cursor
) const {
    const size_t PAGE_SIZE = 50;  // Server-determined page size

    // Decode cursor (server's internal format)
    size_t start_index = 0;
    if (cursor.has_value()) {
        // Decode cursor - example: offset-based encoding
        // Server decides encoding; client treats as opaque
        try {
            start_index = std::stoull(*cursor);
        } catch (...) {
            // Invalid cursor - start from beginning or return error
            start_index = 0;
        }
    }

    PaginatedResult<nlohmann::json> result;
    size_t count = 0;

    for (const auto& [name, registration] : tools_) {
        if (count++ < start_index) continue;
        if (result.items.size() >= PAGE_SIZE) {
            // More results exist
            result.nextCursor = std::to_string(start_index + PAGE_SIZE);
            break;
        }

        result.items.push_back(tool_to_json(registration));
    }

    result.total = tools_.size();
    return result;
}
```

### Task-Augmented Tool Call

```cpp
// In McpServer, handling tools/call with task augmentation
nlohmann::json McpServer::handle_tools_call(
    const nlohmann::json& params,
    const nlohmann::json& _meta
) {
    std::string name = params["name"];
    nlohmann::json arguments = params.value("arguments", nlohmann::json::object());

    // Check for task augmentation
    bool is_task_augmented = params.contains("task");

    if (is_task_augmented) {
        // Extract task metadata
        uint64_t ttl = params["task"].value("ttl", 300000);  // Default 5 min

        // Create task
        std::string task_id = task_manager_.create_task(ttl);

        // Start async execution (e.g., via thread pool)
        execute_tool_async(name, arguments, task_id);

        // Return immediate response with task info
        return nlohmann::json{
            {"task", task_to_json(task_manager_.get_task(task_id).value())}
        };
    } else {
        // Synchronous execution (existing behavior)
        RequestContext ctx(/* ... */);
        return tool_registry_.call_tool(name, arguments, ctx);
    }
}
```

### List Changed Notification Sending

```cpp
// In McpServer, after capability negotiation
void McpServer::on_client_initialized(const ClientCapabilities& client_caps) {
    // Enable list_changed notifications if client supports them
    if (client_caps.tools && client_caps.tools->listChanged == true) {
        tool_registry_.set_notify_callback([this]() {
            send_notification("notifications/tools/list_changed", nullptr);
        });
    }

    if (client_caps.resources && client_caps.resources->listChanged == true) {
        resource_registry_.set_notify_callback([this]() {
            send_notification("notifications/resources/list_changed", nullptr);
        });
    }

    if (client_caps.prompts && client_caps.prompts->listChanged == true) {
        prompt_registry_.set_notify_callback([this]() {
            send_notification("notifications/prompts/list_changed", nullptr);
        });
    }
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Text-only content | Rich content (text, image, audio, resources) | MCP 2025-11-25 | Multi-modal LLM support |
| No pagination | Cursor-based pagination | MCP 2025-11-25 | Scalable list operations |
| No list changed notifications | Optional list_changed per registry | MCP 2025-11-25 | Dynamic resource discovery |
| No long-running operations | Task API with polling | MCP 2025-11-25 | Async request handling |
| No content metadata | Annotations (audience, priority) | MCP 2025-11-25 | Content routing and filtering |

**Deprecated/outdated:**
- **MCP 2024-11-05 content types**: Older schema had fewer content types. Use 2025-11-25 types.
- **Simple list results**: Unpaginated lists still supported but pagination preferred for scalability.

## Open Questions

1. **Base64 encoding implementation**
   - What we know: Spec requires base64 for binary content
   - What's unclear: Which library to use (OpenSSL vs lightweight implementation)
   - Recommendation: Use lightweight inline implementation to avoid additional dependency, or reuse OpenSSL if already linked for HTTP transport

2. **Task ID format**
   - What we know: Must be unique, string, spec suggests cryptographically secure
   - What's unclear: Exact UUID library or format to use
   - Recommendation: Use simple UUID v4 generator or `std::random_device` based implementation

3. **Cursor encoding format**
   - What we know: Opaque to client, server decides encoding
   - What's unclear: Best encoding for mcpp use cases (offset vs timestamp vs composite)
   - Recommendation: Start with simple offset-based encoding, extend if needed for stability requirements

4. **Task storage backend**
   - What we know: Tasks need TTL tracking, status updates
   - What's unclear: Memory-only vs persistent storage
   - Recommendation: In-memory storage for MVP, persistence hook for future

5. **Concurrent task execution**
   - What we know: Tasks may run in parallel
   - What's unclear: Thread pool integration vs user-provided executor
   - Recommendation: User provides executor via callback (matches existing handler pattern)

## Sources

### Primary (HIGH confidence)
- [MCP 2025-11-25 schema](https://github.com/modelcontextprotocol/specification/blob/main/schema/2025-11-25/schema.ts) - Content types, annotations, pagination, tasks (lines 1700-1900, 1300-1450)
- [MCP 2025-11-25 pagination spec](https://github.com/modelcontextprotocol/specification/blob/main/docs/specification/2025-11-25/server/utilities/pagination.mdx) - Pagination semantics
- [MCP 2025-11-25 tasks spec](https://github.com/modelcontextprotocol/specification/blob/main/docs/specification/2025-11-25/basic/utilities/tasks.mdx) - Task lifecycle, polling, notifications
- [Rust SDK task.rs](https://github.com/modelcontextprotocol/rust-sdk/blob/main/crates/rmcp/src/model/task.rs) - Reference implementation for task types

### Secondary (MEDIUM confidence)
- Existing mcpp codebase patterns:
  - `src/mcpp/client/sampling.h` (lines 43-86) - ContentBlock pattern
  - `src/mcpp/client/roots.h` (lines 129-202) - List changed notification pattern
  - `src/mcpp/server/tool_registry.h` - Registry pattern with handlers
  - `src/mcpp/server/resource_registry.h` - Template and subscription patterns
  - `src/mcpp/protocol/capabilities.h` - Capability negotiation structure

### Tertiary (LOW confidence)
- None - all research verified against official MCP spec and existing codebase

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All dependencies already in use
- Architecture: HIGH - Based on official MCP spec and existing codebase patterns
- Pitfalls: HIGH - Derived from spec requirements and common async programming issues

**Research date:** 2026-01-31
**Valid until:** 2026-06-30 (MCP spec is stable, but experimental tasks may evolve)
