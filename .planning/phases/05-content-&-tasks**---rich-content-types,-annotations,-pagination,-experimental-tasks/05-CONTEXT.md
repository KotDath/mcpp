# Phase 5: Content & Tasks - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

## Phase Boundary

Rich content type handling (text, image, audio, resource links, embedded resources), content annotations, cursor-based pagination support for all list operations, list changed notifications for registries, and experimental task lifecycle management with client-initiated polling.

## Implementation Decisions

### Content type handling

- **Binary content representation**: Images and audio stored as base64-encoded strings with `mimeType` field (matches both Rust SDK and gopher-mcp implementations)
- **Content annotations**: Optional field on each content struct - `optional<Annotations>` with `audience` (vector<Role>) and `priority` (double 0.0-1.0) fields
- **Resource links vs embedded resources**: Separate types - `ResourceLink` (type="resource") for references, `EmbeddedResource` (type="embedded") with nested `content` array for inline content
- **Builder helpers**: Fluent factory functions for content creation - `make_image_content()`, `make_audio_content()`, `make_embedded_resource()`, `make_resource_content()` with optional chaining for annotations

### Pagination semantics

- **Cursor type**: `std::string` as opaque cursor - server can encode any format (offset, timestamp, ID)
- **Opt-in pagination**: `optional<std::string> cursor` parameter - first page returned when cursor not provided
- **Coverage**: All list operations support pagination - `tools/list`, `resources/list`, `prompts/list`
- **Result structure**: `PaginatedResult` base with `optional<Cursor> nextCursor`, `optional<uint64_t> total` for count

### Task lifecycle

- **TTL**: Optional field, no default - server decides whether to set retention window (matches Rust SDK pattern)
- **Polling model**: Client-initiated polling via `tasks/get` request
- **poll_interval**: Optional field in Task metadata suggests how often client should poll (milliseconds)
- **Task expiration**: When TTL reached, task status becomes `Failed` with appropriate message
- **Task states**: Working, InputRequired, Completed, Failed, Cancelled (enum matching Rust SDK)

### List changed notifications

- **Mutual opt-in**: Both server and client must enable capability
  - Server: Per-list capability flags (tools.listChanged, resources.listChanged, prompts.listChanged)
  - Client: Indicates which notifications it wants to receive
- **Automatic on change**: Registries automatically send list_changed notifications on register/unregister/modify operations
- **Notification types**: Separate notifications for each registry
  - `notifications/tools/list_changed`
  - `notifications/resources/list_changed`
  - `notifications/prompts/list_changed`
- **No parameters**: Notifications are simple JSON-RPC notifications with no parameters (NotificationNoParam pattern from Rust SDK)

### Claude's Discretion

- Exact base64 encoding implementation
- Cursor encoding format (server-side decision)
- Default poll_interval value when set
- Notification batching/throttling if needed

## Specific Ideas

- Follow Rust SDK patterns from `/thirdparty/rust-sdk/crates/rmcp/src/model/task.rs` for task structure
- Follow gopher-mcp content patterns from `/thirdparty/gopher-mcp/` for content type variants
- Use builder pattern for capabilities (similar to existing `ClientCapabilities::Builder` from 03-08)

## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 05-content-&-tasks*
*Context gathered: 2026-01-31*
