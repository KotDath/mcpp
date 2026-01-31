# Phase 7: Build & Validation - Context

**Gathered:** 2026-02-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Production-ready CMake build system with static and shared libraries, MIT licensing, comprehensive test suite, and MCP Inspector integration validation.

This phase delivers the build infrastructure, packaging, and validation — not new MCP features.
</domain>

<decisions>
## Implementation Decisions

### Library packaging
- Build both static and shared libraries by default
- Semantic versioning for shared library SONAME (MAJOR.MINOR.PATCH)
- Configurable install paths: GNU layout by default, single prefix via CMAKE option
- Vendor dependencies (nlohmann/json, spdlog) in third_party/
- HTTP transport: User provides their own HTTP server via adapter pattern (no HTTP lib in mcpp)
- Full package config (mcppConfig.cmake) for find_package(mcpp) support
- Single mcpp::mcpp target for linking (aliases to whichever library type was built)

### Test organization
- Three-tier structure: tests/unit/ (per module), tests/integration/ (end-to-end), tests/compliance/ (JSON-RPC spec)
- 80% minimum line coverage for src/ (exceptions allowed for template headers)
- Common fixtures for shared setup (mock transport, test JSON), plus per-test fixtures
- Test data in tests/data/ directory (JSON fixtures, protocol examples)
- Thread safety testing: You decide (likely ThreadSanitizer in CI)

### Build configuration
- Modular structure: Root CMakeLists.txt + src/CMakeLists.txt, tests/CMakeLists.txt
- Compiler flags: You decide (likely -Wall -Wextra -Werror, -O3 release, -g debug)
- Build variants: Debug and Release only

### Integration validation
- MCP Inspector: BOTH example server + automated test
  - Example server using mcpp, documented in README
  - Automated test that spawns Inspector via npm and validates protocol compliance
- Protocol compliance: Official MCP test suite + custom tests if official suite incomplete
- Async lifetime testing: You decide (likely ASan/LSan tests)

### Claude's Discretion
- Thread safety testing approach (explicit tests vs sanitizer-only)
- Compiler flags and warnings policy
- Async lifetime testing approach (dedicated tests vs sanitizer-only)
- Exact CMake target organization within modular structure
- Whether official MCP test suite exists — fall back to custom compliance tests if not

</decisions>

<specifics>
## Specific Ideas

- "Vendor dependencies" — include nlohmann/json and spdlog in third_party/ to avoid external dependency discovery issues
- "Both example and automated" — want both manual example for users and automated test for CI

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 07-build-validation*
*Context gathered: 2026-02-01*
