---
phase: 06-high-level-api
plan: 01
subsystem: high-level-api
tags: [c++20, concepts, std::atomic, std::shared_mutex, role-based-typing, service-trait, peer-state]

# Dependency graph
requires:
  - phase: 05-content---tasks
    provides: task lifecycle, rich content types, pagination
provides:
  - Role marker types (RoleClient, RoleServer) for compile-time role distinction
  - ServiceRole concept for type-safe role validation
  - Service<Role> trait abstraction with polymorphic handler interface
  - ClientInfo/ServerInfo with experimental capabilities (UTIL-03)
  - AtomicRequestIdProvider for lock-free ID generation
  - Peer<Role> template with thread-safe connection state foundation
affects: [06-02-running-service, 06-03-logging, 06-04-error-handling]

# Tech tracking
tech-stack:
  added: [C++20 concepts, std::atomic<uint32_t>, std::shared_mutex, std::future]
  patterns: [role-based typing, service trait abstraction, RAII non-copyable types, reader-writer mutex, lock-free atomic counter]

key-files:
  created: [src/mcpp/api/role.h, src/mcpp/api/service.h, src/mcpp/api/peer.h, src/mcpp/util/atomic_id.h]
  modified: []

key-decisions:
  - "Role-based typing with empty marker structs (RoleClient/RoleServer) for compile-time safety"
  - "ServiceRole concept using std::same_as for simple role validation"
  - "memory_order_relaxed for atomic ID generation (ordering not required for uniqueness)"
  - "std::shared_mutex for peer_info (concurrent reads, exclusive writes)"
  - "Non-copyable/non-movable types for ID provider and Peer (prevents accidental ID collisions)"
  - "Stub send_request/send_notification returning std::future (async pattern ready for 06-02)"

patterns-established:
  - "Pattern 1: Role marker types with IS_CLIENT constexpr bool for role detection"
  - "Pattern 2: RoleTypes trait for role-specific type aliases (PeerReq, PeerResp, PeerNot, Info, PeerInfo)"
  - "Pattern 3: Service trait as pure virtual interface for polymorphic handlers"
  - "Pattern 4: Shared atomic state via shared_ptr for cross-component coordination"

# Metrics
duration: 3min
completed: 2026-02-01
---

# Phase 6 Plan 1: High-Level API Foundation Summary

**Role-based type safety with Service trait abstraction, thread-safe Peer state management, and lock-free atomic ID generation following rust-sdk patterns**

## Performance

- **Duration:** 3 min
- **Started:** 2026-01-31T21:28:23Z
- **Completed:** 2026-01-31T21:31:30Z
- **Tasks:** 4 (all completed)
- **Files created:** 4

## Accomplishments

- Created role marker types (RoleClient, RoleServer) with ServiceRole concept for compile-time role distinction
- Implemented Service<Role> trait abstraction with pure virtual handler methods and experimental capabilities (UTIL-03)
- Built AtomicRequestIdProvider using std::atomic with fetch_add and memory_order_relaxed for lock-free ID generation
- Designed Peer<Role> template with std::shared_mutex for thread-safe peer_info access and stub async methods

## Task Commits

Each task was committed atomically:

1. **Task 1: Create role marker types and ServiceRole concept** - `9297fc3` (feat)
2. **Task 2: Create Service trait abstraction with experimental capabilities** - `202fd47` (feat)
3. **Task 3: Create AtomicRequestIdProvider utility** - `202fd47` (feat - combined with task 2)
4. **Task 4: Create Peer template with thread-safe state** - `fc8fee8` (feat)

## Files Created

- `src/mcpp/api/role.h` - RoleClient/RoleServer marker types, ServiceRole concept
- `src/mcpp/api/service.h` - Service<Role> trait, ClientInfo/ServerInfo with experimental, RequestContext/NotificationContext
- `src/mcpp/api/peer.h` - Peer<Role> with shared_mutex, stub async methods, peer_info accessor
- `src/mcpp/util/atomic_id.h` - AtomicRequestIdProvider with lock-free fetch_add

## Decisions Made

- Used C++20 concepts (ServiceRole) instead of SFINAE for cleaner compile-time role validation
- Memory order relaxed for atomic ID generation - strict ordering not required for uniqueness
- std::shared_mutex for peer_info - allows concurrent reads with exclusive writes
- Non-copyable/non-movable for Peer and AtomicRequestIdProvider - prevents accidental ID collisions and lifetime issues
- Included error.h in service.h to resolve JsonRpcError forward declaration issue in json_rpc.h
- Stub send_request/send_notification return std::future - establishes async pattern for 06-02 implementation

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- **JsonRpcError forward declaration error:** The json_rpc.h header forward declares JsonRpcError but std::optional requires complete type. Fixed by adding `#include "mcpp/core/error.h"` to service.h.
- **nlohmann/json.hpp not found:** Compiler couldn't find the header. Fixed by adding thirdparty/nlohmann_json/include to include path.

## Verification

All verification criteria passed:

1. All new headers compile without errors: `g++ -std=c++20 -fsyntax-only` successful
2. Static assertions verify ServiceRole concept: `static_assert(ServiceRole<RoleClient>)` passes
3. AtomicRequestIdProvider generates unique IDs in multi-threaded test (10 threads x 100 IDs = 1000 unique IDs)
4. No circular dependencies between headers (role.h has no mcpp deps, atomic_id.h has no mcpp deps, service.h depends only on role.h, peer.h depends on role.h/service.h/atomic_id.h)
5. ClientInfo/ServerInfo contain experimental field with std::map<std::string, nlohmann::json> for custom capabilities

## UTIL-03 Compliance

The experimental field was added to both ClientInfo and ServerInfo structures:

```cpp
struct ClientInfo {
    std::string name;
    std::string version;
    std::map<std::string, nlohmann::json> experimental;  // UTIL-03
    nlohmann::json to_json() const;  // Includes experimental in JSON
};

struct ServerInfo {
    std::string name;
    std::string version;
    std::map<std::string, nlohmann::json> experimental;  // UTIL-03
    nlohmann::json to_json() const;  // Includes experimental in JSON
};
```

This enables custom feature negotiation beyond standard MCP capabilities.

## Next Phase Readiness

- Role-based type safety foundation complete, ready for RunningService implementation (06-02)
- Service trait interface established, ready for concrete handler implementations
- Peer template foundation ready for full async implementation with transport integration
- AtomicRequestIdProvider ready for integration with request/response correlation
- No blockers or concerns

---
*Phase: 06-high-level-api*
*Completed: 2026-02-01*
