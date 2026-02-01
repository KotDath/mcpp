# Roadmap: mcpp - C++ MCP Library

## Overview

mcpp v1.0 delivered a complete MCP 2025-11-25 implementation with dual transports (stdio, SSE), thread-safe async design, and comprehensive testing. v1.1 is a focused stabilization milestone fixing the critical JSON-RPC parse error bug (-32700) and enabling proper testing via MCP Inspector CLI integration. The work follows a clear dependency chain: fix library layer first, update example server to use fixes, add automated tests to prevent regression, and document the new testing capabilities.

## Milestones

- ✅ **v1.0 Initial Release** - Phases 1-7 (shipped 2026-02-01)
- 🚧 **v1.1 Stability & Inspector Integration** - Phases 8-11 (in progress)

## Phases

<details>
<summary>✅ v1.0 Initial Release (Phases 1-7) - SHIPPED 2026-02-01</summary>

**[See full archived roadmap: `.planning/milestones/v1.0-ROADMAP.md`](.planning/milestones/v1.0-ROADMAP.md)**

- [x] Phase 1: Protocol Foundation (6/6 plans) — completed 2026-01-31
- [x] Phase 2: Core Server (6/6 plans) — completed 2026-01-31
- [x] Phase 3: Client Capabilities (8/8 plans) — completed 2026-01-31
- [x] Phase 4: Advanced Features & HTTP Transport (6/6 plans) — completed 2026-01-31
- [x] Phase 5: Content & Tasks (4/4 plans) — completed 2026-01-31
- [x] Phase 6: High-Level API (7/7 plans) — completed 2026-02-01
- [x] Phase 7: Build & Validation (6/6 plans) — completed 2026-02-01

**Total:** 46 plans across 7 phases, all complete

</details>

### 🚧 v1.1 Stability & Inspector Integration (In Progress)

**Milestone Goal:** Fix critical JSON-RPC parse error bug and enable MCP Inspector CLI testing

**Milestone Goal:** Fix critical JSON-RPC parse error bug and enable MCP Inspector CLI testing

#### Phase 8: Bug Fix Foundation
**Goal**: Library provides proper JSON-RPC request validation and stdio protocol compliance
**Depends on**: Phase 7 (v1.0 complete)
**Requirements**: BUGFIX-01, BUGFIX-02, BUGFIX-03
**Success Criteria** (what must be TRUE):
  1. Library provides JsonRpcRequest::from_json() for proper request validation
  2. All JSON output includes trailing newline delimiter for stdio compliance
  3. Debug output goes to stderr, not stdout (prevents protocol pollution)
**Plans**: 2 plans

Plans:
- [x] 08-01-PLAN.md — Implement JsonRpcRequest::from_json() validation with ParseError diagnostics
- [x] 08-02-PLAN.md — Add newline-delimited output helpers and stderr-only debug logging macro

#### Phase 9: Inspector Server Integration
**Goal**: Example server uses new validation layer and is compatible with MCP Inspector
**Depends on**: Phase 8
**Requirements**: BUGFIX-04, BUGFIX-05, INSPECT-01, INSPECT-02, INSPECT-03, INSPECT-04
**Success Criteria** (what must be TRUE):
  1. Example server uses JsonRpcRequest::from_json() for request parsing
  2. Parse error responses include properly extracted request ID
  3. mcp.json configuration enables standard Inspector compatibility
  4. Manual testing with MCP Inspector CLI succeeds (tools/call works)
**Plans**: 3 plans

Plans:
- [x] 09-01-PLAN.md — Add JsonRpcRequest::extract_request_id() helper for parse error responses
- [x] 09-02-PLAN.md — Update inspector_server.cpp to use from_json() validation layer
- [x] 09-03-PLAN.md — Create mcp.json configuration and TESTING.md documentation

#### Phase 10: Automated CLI Testing
**Goal**: Bats-core test suite validates all MCP methods via Inspector
**Depends on**: Phase 9
**Requirements**: TEST-01, TEST-02, TEST-03, TEST-04, TEST-05, TEST-06
**Success Criteria** (what must be TRUE):
  1. Bats-core 1.11.0+ test framework integrated with CMake
  2. Basic integration tests pass (tools/list, tools/call, resources/list, prompts/list)
  3. JSON responses validated using jq
  4. Server lifecycle helpers (startup, shutdown, cleanup) prevent process leaks
  5. CI/CD runs cli-tests target as part of test suite
**Plans**: 6 plans (4 original + 2 gap closure)

Plans:
- [x] 10-01-PLAN.md — Set up bats-core Git submodules and common setup helper
- [x] 10-02-PLAN.md — Create initialize and tools/list endpoint tests
- [x] 10-03-PLAN.md — Create tools/call, resources, and prompts endpoint tests
- [x] 10-04-PLAN.md — Create server lifecycle tests and CMake integration
- [x] 10-05-PLAN.md — Create CI/CD pipeline and fix CMake bats discovery (Gap 1 closure)
- [x] 10-06-PLAN.md — Refactor test files to use common-setup consistently (Gap 2 closure)

#### Phase 11: Documentation
**Goal**: Users can run Inspector tests and write custom integration tests
**Depends on**: Phase 10
**Requirements**: DOCS-01, DOCS-02
**Success Criteria** (what must be TRUE):
  1. README includes MCP Inspector testing instructions
  2. Testing guide explains how to write custom Inspector tests
**Plans**: 2 plans

Plans:
- [ ] 11-01-PLAN.md — Update README.md with Inspector Testing section (UI and CLI modes)
- [ ] 11-02-PLAN.md — Create TESTING.md with BATS testing guide

## Progress

**Execution Order:**
Phases execute in numeric order: 8 → 9 → 10 → 11

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Protocol Foundation | v1.0 | 6/6 | Complete | 2026-01-31 |
| 2. Core Server | v1.0 | 6/6 | Complete | 2026-01-31 |
| 3. Client Capabilities | v1.0 | 8/8 | Complete | 2026-01-31 |
| 4. Advanced Features & HTTP Transport | v1.0 | 6/6 | Complete | 2026-01-31 |
| 5. Content & Tasks | v1.0 | 4/4 | Complete | 2026-01-31 |
| 6. High-Level API | v1.0 | 7/7 | Complete | 2026-02-01 |
| 7. Build & Validation | v1.0 | 6/6 | Complete | 2026-02-01 |
| 8. Bug Fix Foundation | v1.1 | 2/2 | Complete | 2026-02-01 |
| 9. Inspector Server Integration | v1.1 | 3/3 | Complete | 2026-02-01 |
| 10. Automated CLI Testing | v1.1 | 6/6 | Complete | 2026-02-01 |
| 11. Documentation | v1.1 | 0/2 | Not started | - |

**v1.1 Progress:** [████████░░] 80% (4/5 phases complete; Phase 11 planning ready)
