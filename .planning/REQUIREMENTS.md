# Requirements: mcpp v1.1

**Defined:** 2026-02-01
**Core Value:** Developers can build MCP clients and servers in C++ that are fast, correct, and support the complete protocol spec without wrestling with JSON-RPC details or transport plumbing

## v1.1 Requirements

Requirements for Stability & Inspector Integration milestone. Each maps to roadmap phases.

### Bug Fixes

- [x] **BUGFIX-01**: Library provides JsonRpcRequest::from_json() for proper request validation
- [x] **BUGFIX-02**: All JSON output includes trailing newline delimiter for stdio compliance
- [x] **BUGFIX-03**: Debug output goes to stderr, not stdout (prevents protocol pollution)
- [ ] **BUGFIX-04**: Example server uses new request validation layer
- [ ] **BUGFIX-05**: Parse error responses include properly extracted request ID

### Inspector Integration

- [ ] **INSPECT-01**: inspector_server.cpp updated to use JsonRpcRequest::from_json()
- [ ] **INSPECT-02**: mcp.json configuration file for standard Inspector compatibility
- [ ] **INSPECT-03**: Enhanced debug mode with trace-level logging for troubleshooting
- [ ] **INSPECT-04**: Manual testing verified with MCP Inspector CLI

### Automated Testing

- [ ] **TEST-01**: Bats-core 1.11.0+ test framework integrated for CLI testing
- [ ] **TEST-02**: Basic integration tests (tools/list, tools/call, resources/list, prompts/list)
- [ ] **TEST-03**: JSON response validation using jq
- [ ] **TEST-04**: Server lifecycle helpers (startup, shutdown, cleanup)
- [ ] **TEST-05**: Full endpoint coverage (all MCP methods)
- [ ] **TEST-06**: CI/CD integration with CMake test suite

### Documentation

- [ ] **DOCS-01**: README updated with Inspector testing instructions
- [ ] **DOCS-02**: Complete testing guide for writing custom Inspector tests

## Future Requirements

Deferred to v1.2+:

- Tool result streaming tests
- Performance benchmarks under Inspector
- WebSocket transport testing (when added to spec)
- Memory sanitizer testing integration

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Inspector UI mode testing | CLI mode sufficient for automated testing; UI mode is manual |
| Modifying core library API | Bug fixes only; no API changes in v1.1 |
| New transport implementations | Focus on existing stdio transport stability |
| Header-only distribution | Build system enhancement, not stability-related |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| BUGFIX-01 | Phase 8 | Complete |
| BUGFIX-02 | Phase 8 | Complete |
| BUGFIX-03 | Phase 8 | Complete |
| BUGFIX-04 | Phase 9 | Pending |
| BUGFIX-05 | Phase 9 | Pending |
| INSPECT-01 | Phase 9 | Pending |
| INSPECT-02 | Phase 9 | Pending |
| INSPECT-03 | Phase 9 | Pending |
| INSPECT-04 | Phase 9 | Pending |
| TEST-01 | Phase 10 | Pending |
| TEST-02 | Phase 10 | Pending |
| TEST-03 | Phase 10 | Pending |
| TEST-04 | Phase 10 | Pending |
| TEST-05 | Phase 10 | Pending |
| TEST-06 | Phase 10 | Pending |
| DOCS-01 | Phase 11 | Pending |
| DOCS-02 | Phase 11 | Pending |

**Coverage:**
- v1.1 requirements: 18 total
- Mapped to phases: 18 (100%)
- Unmapped: 0

---
*Requirements defined: 2026-02-01*
*Last updated: 2026-02-01 after roadmap creation*
