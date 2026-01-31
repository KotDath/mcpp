# Phase 07: Build & Validation - Research

**Researched:** 2026-02-01
**Domain:** CMake build systems, C++ testing frameworks, MCP validation
**Confidence:** HIGH

## Summary

Phase 7 focuses on production-ready build infrastructure and comprehensive testing for the mcpp library. This phase delivers CMake build system with static/shared libraries, complete MIT licensing, GoogleTest-based unit tests, MCP conformance validation, and sanitizer-based testing for thread safety and lifetime issues.

The research identified:
- **CMake 3.16+** as the baseline with FetchContent for GoogleTest integration
- **GoogleTest** as the standard C++ testing framework with CTest integration via `gtest_discover_tests()`
- **MCP conformance tests** available via `@modelcontextprotocol/conformance` npm package (work-in-progress but usable)
- **GCC sanitizers** (ASan, LSan, TSan) as the primary approach for lifetime and thread safety testing
- **Code coverage** via gcc --coverage flag with lcov/genhtml for reporting

**Primary recommendation:** Use FetchContent for GoogleTest, standard CMake package config for find_package() support, combined ASan+LSan for lifetime testing, and TSan for thread safety in CI. MCP conformance tests exist but are unstable; build custom JSON-RPC compliance tests as backup.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CMake | 3.16+ | Build system | Minimum version for FetchContent (3.14+), target properties, GNUInstallDirs |
| GoogleTest | 1.14+ (main branch) | Unit testing framework | De facto standard for C++ testing, integrates with CTest via gtest_discover_tests() |
| CTest | (bundled with CMake) | Test runner | Standard CMake test executor, supports parallel execution, labels, JSON output |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| lcov/genhtml | (system) | Code coverage reporting | Generate HTML coverage reports from gcc --coverage data |
| @modelcontextprotocol/inspector | latest (npm) | Manual MCP server testing | Interactive development and validation |
| @modelcontextprotocol/conformance | latest (npm) | Automated protocol compliance | CI validation against MCP spec (unstable but functional) |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| FetchContent for GoogleTest | FindGTest.cmake / system install | FetchContent ensures reproducible builds, no system dependency |
| gcc --coverage | Clang's coverage gcov | GCC coverage is more mature, better tooling with lcov |
| Sanitizers | Explicit thread tests | Sanitizers catch actual bugs vs theoretical issues; write some explicit tests too |
| MCP conformance npm | Custom test suite only | Conformance suite exists but unstable; need custom tests as fallback |

**Installation:**
```bash
# GoogleTest via FetchContent (no install needed - declared in CMakeLists.txt)
# Code coverage tools (Ubuntu/Debian):
sudo apt install lcov

# MCP Inspector/conformance for testing:
npm install -g @modelcontextprotocol/inspector
npm install -g @modelcontextprotocol/conformance
```

## Architecture Patterns

### Recommended Project Structure
```
mcpp/
├── CMakeLists.txt              # Root build config
├── LICENSE                     # MIT license
├── cmake/
│   └── mcppConfig.cmake.in    # Package config template for find_package()
├── src/
│   ├── mcpp/                  # Public headers (installed)
│   └── [source files]         # Implementation
├── tests/
│   ├── CMakeLists.txt         # Test configuration
│   ├── unit/                  # Per-module unit tests
│   ├── integration/           # End-to-end tests
│   ├── compliance/            # JSON-RPC spec tests
│   └── data/                  # Test fixtures (JSON, protocol examples)
├── examples/
│   ├── inspector_server.cpp   # Example MCP server for Inspector testing
│   └── CMakeLists.txt
└── third_party/               # Vendored dependencies
    └── nlohmann_json/
        └── include/nlohmann/
```

### Pattern 1: CMake Package Configuration
**What:** Create installable CMake package for `find_package(mcpp)` support
**When to use:** Any library intended for distribution to other projects
**Example:**
```cmake
# Source: https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Generate version file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/mcppConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

# Configure package config
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/mcppConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/mcppConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mcpp
)

# Install targets and config
install(TARGETS mcpp EXPORT mcppTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mcpp
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT mcppTargets
  FILE mcppTargets.cmake
  NAMESPACE mcpp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mcpp
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/mcppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/mcppConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mcpp
)
```

### Pattern 2: GoogleTest with CTest Discovery
**What:** Automatic test discovery and registration with CTest
**When to use:** All GoogleTest-based test executables
**Example:**
```cmake
# Source: https://google.github.io/googletest/quickstart-cmake.html
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(mcpp_tests
  # test sources...
)
target_link_libraries(mcpp_tests
  PRIVATE
    mcpp
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(mcpp_tests)
```

### Pattern 3: Shared Library Versioning (SONAME)
**What:** Proper semantic versioning for shared libraries
**When to use:** All shared libraries for ABI compatibility tracking
**Example:**
```cmake
# Source: https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html
set_target_properties(mcpp PROPERTIES
  VERSION ${PROJECT_VERSION}        # e.g., 0.1.0
  SOVERSION ${PROJECT_VERSION_MAJOR} # e.g., 0 -> libmcpp.so.0
)

# Results in:
#   libmcpp.so.0.1.0  -> actual file
#   libmcpp.so.0      -> symlink to libmcpp.so.0.1.0 (SONAME)
#   libmcpp.so        -> symlink to libmcpp.so.0 (linker)
```

### Pattern 4: Single Alias Target
**What:** Provide `mcpp::mcpp` alias that works for both static and shared builds
**When to use:** Libraries that support multiple build types
**Example:**
```cmake
# Build both static and shared
add_library(mcpp_static STATIC ${MCPP_SOURCES})
add_library(mcpp_shared SHARED ${MCPP_SOURCES})

# Alias that selects the right one based on BUILD_SHARED_LIBS
add_library(mcpp::mcpp ALIAS mcpp_shared)
if(NOT BUILD_SHARED_LIBS)
  add_library(mcpp::mcpp ALIAS mcpp_static)
endif()
```

### Anti-Patterns to Avoid
- **Hard-coded install paths:** Always use `GNUInstallDirs` variables (`CMAKE_INSTALL_LIBDIR`, etc.) for portability
- **Missing SOVERSION:** Shared libraries without SOVERSION break ABI compatibility tracking
- **Non-relocatable packages:** Use generator expressions like `$<INSTALL_INTERFACE:include>` for relocatable installs
- **Werror in user-facing builds:** `-Werror` should be optional/CI-only; users' compilers may have different warnings

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Test framework | Custom test runner | GoogleTest + CTest | Discovery, fixtures, assertions, CI integration all built-in |
| Package config | Manual find_package logic | CMake's install(EXPORT) | Handles config files, versioning, targets automatically |
| Code coverage | Custom gcov parsing | lcov/genhtml | Mature tools with HTML output, branch coverage |
| Thread safety detection | Custom race condition tests | ThreadSanitizer (-fsanitize=thread) | Catches real races missed by explicit tests |
| Memory leak detection | Custom leak tracking | AddressSanitizer + LeakSanitizer | Zero false positives, catches leaks in all code paths |
| JSON-RPC testing | Custom protocol validator | MCP conformance suite + custom | Official suite validates against spec |

**Key insight:** Custom solutions for testing infrastructure divert effort from actual test content. CMake, GoogleTest, and sanitizers provide production-grade foundations that handle edge cases.

## Common Pitfalls

### Pitfall 1: C++20 Requirement vs C++17 Spec
**What goes wrong:** Current CMakeLists.txt requires C++20 for `std::stop_token`, but REQUIREMENTS.md specifies C++17 (gcc-11 compatible).
**Why it happens:** `std::stop_token` is C++20; cancellation support (ASYNC-02) needs it.
**How to avoid:** This is a REQUIREMENTS conflict that must be resolved. Options:
  1. Update REQUIREMENTS.md to allow C++20 (simpler, modern)
  2. Use `<stop_token>` polyfill or alternative for C++17 (complex, less portable)
**Warning signs:** Build fails with gcc-11 in C++17 mode due to missing `std::jthread`, `std::stop_token`.

### Pitfall 2: Missing Source Files in CMakeLists.txt
**What goes wrong:** New source files added but not listed in `MCPP_SOURCES`, causing linker errors.
**Why it happens:** CMake uses explicit source listing; new files aren't auto-discovered.
**How to avoid:** Document in Phase 7 planning that `MCPP_SOURCES` must be updated when adding `.cpp` files. Consider using `file(GLOB...)` with caveats or a pattern.
**Warning signs:** Undefined reference errors for new functions.

### Pitfall 3: Non-vendored Dependency Breaks Build
**What goes wrong:** nlohmann/json path or version changes between systems/machines.
**Why it happens:** Current CMakeLists.txt points to `thirdparty/nlohmann_json/include` but the content may not exist.
**How to avoid:** Vendor nlohmann/json completely in `third_party/`. Update CMakeLists.txt path to match actual structure.
**Warning signs:** Build fails on fresh clone with "nlohmann/json.hpp not found".

### Pitfall 4: Unstable MCP Conformance Tests in CI
**What goes wrong:** `@modelcontextprotocol/conformance` is marked "work in progress" and "unstable".
**Why it happens:** Official conformance suite is early-stage (per GitHub README warning).
**How to avoid:** Build custom JSON-RPC compliance tests as primary validation. Use official conformance as optional/beta. Don't block CI on unstable external tests.
**Warning signs:** Flaky CI tests, conformance test failures unrelated to code changes.

### Pitfall 5: Sanitizer Compatibility Issues
**What goes wrong:** ASan/TSan can't be used together; LSan is standalone on some platforms.
**Why it happens:** ASan includes LSan on most platforms; TSan is incompatible with ASan.
**How to avoid:** Use separate CI builds: one with `-fsanitize=address,leak`, another with `-fsanitize=thread`. Document this in test plan.
**Warning signs:** "incompatible sanitizers" linker errors, sanitizer not detecting issues.

## Code Examples

### CMakeLists.txt for GoogleTest Integration
```cmake
# Source: https://google.github.io/googletest/quickstart-cmake.html

option(MCPP_BUILD_TESTS "Build mcpp tests" ON)

if(MCPP_BUILD_TESTS)
  enable_testing()

  # Use FetchContent for reproducible builds
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.14.0
  )
  # For Windows: Prevent overriding parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  # Add test subdirectory
  add_subdirectory(tests)
endif()
```

### Test CMakeLists.txt Structure
```cmake
# tests/CMakeLists.txt

# Unit tests
add_executable(mcpp_unit_tests
  unit/test_json_rpc.cpp
  unit/test_request_tracker.cpp
  # ...
)
target_link_libraries(mcpp_unit_tests
  PRIVATE mcpp GTest::gtest GTest::gtest_main
)
target_include_directories(mcpp_unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
gtest_discover_tests(mcpp_unit_tests LABELS unit)

# Integration tests
add_executable(mcpp_integration_tests
  integration/test_client_server.cpp
  # ...
)
target_link_libraries(mcpp_integration_tests
  PRIVATE mcpp GTest::gtest GTest::gtest_main
)
gtest_discover_tests(mcpp_integration_tests LABELS integration)

# Compliance tests
add_executable(mcpp_compliance_tests
  compliance/test_jsonrpc_spec.cpp
  # ...
)
target_link_libraries(mcpp_compliance_tests
  PRIVATE mcpp GTest::gtest GTest::gtest_main
)
gtest_discover_tests(mcpp_compliance_tests LABELS compliance)
```

### Sanitizer Build Configuration
```bash
# AddressSanitizer + LeakSanitizer (most common)
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -g" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address -fsanitize=leak" \
      -B build/sanitizer

# ThreadSanitizer (separate build - incompatible with ASan)
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" \
      -B build/tsan
```

### MCP Inspector Integration Test (Conceptual)
```bash
# Add to tests/CMakeLists.txt as a custom test
add_test(NAME mcpp_inspector_validation
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_inspector_test.sh
    $<TARGET_FILE:inspector_example_server>
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# scripts/run_inspector_test.sh would:
# 1. Start the example server in background
# 2. Run npx @modelcontextprotocol/conformance server --url http://localhost:PORT
# 3. Check exit code and results/*.json for pass/fail
# 4. Kill server
```

### Code Coverage with CMake/CTest
```cmake
# Add to CMakeLists.txt for coverage builds
option(BUILD_COVERAGE "Enable code coverage" OFF)

if(BUILD_COVERAGE)
  target_compile_options(mcpp PRIVATE --coverage)
  target_link_options(mcpp PRIVATE --coverage)
endif()

# CTest coverage step (manual or via CTest script)
# Usage: cmake -DBUILD_COVERAGE=ON -B build/coverage
#         cd build/coverage && make && ctest
#         lcov --capture --directory . --output-file coverage.info
#         genhtml coverage.info --output-directory coverage_html
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| FindGTest.cmake module | FetchContent_Declare | CMake 3.14+ | Reproducible builds, no system dependency on GTest |
| Manual test registration | gtest_discover_tests() | CMake 3.10+ | Automatic test discovery, per-test-case CTest tests |
| Custom package config files | install(EXPORT) + write_basic_package_version_file() | CMake 3.0+ | Proper version checking, relocatable packages |
| -Werror always | Werror as option/CI-only | - | Avoids breaking on compiler-specific warnings |

**Deprecated/outdated:**
- **FindGTest module:** Prefer FetchContent for vendoring GTest with project
- **include(CTest) without enable_testing():** enable_testing() must be called first
- **CMAKE_INSTALL_PREFIX hard-coding:** Always use GNUInstallDirs variables
- **Pre-CMake 3.14 FetchContent:** Old syntax was more verbose; use modern OVERRIDE_FIND_PACKAGE if needed

## Open Questions

### Q1: C++17 vs C++20 Requirement Conflict
**What we know:**
- REQUIREMENTS.md specifies C++17 with gcc-11 compatibility
- Current code uses C++20 for `std::stop_token` (cancellation support)
- CMakeLists.txt sets `CMAKE_CXX_STANDARD 20`

**What's unclear:**
- Which requirement takes precedence? Should we update REQUIREMENTS.md or find C++17 alternative?

**Recommendation:** Update REQUIREMENTS.md to allow C++20. Rationale:
  - C++20 is 5 years old (2020), well-supported by gcc-11+
  - `std::jthread` and `std::stop_token` are core to cancellation design
  - C++17 polyfills would add complexity/dependencies
  - Modern C++ libraries targeting 2025+ should use C++20

### Q2: spdlog Dependency Strategy
**What we know:**
- Logging (UTIL-01) uses spdlog when available (`__has_include`)
- Current approach: graceful fallback when spdlog not present
- No formal dependency declaration in CMakeLists.txt

**What's unclear:**
- Should spdlog be a required dependency, optional, or vendored?

**Recommendation:** Make spdlog optional with FetchContent fallback, vendor in third_party/ if unavailable. Rationale:
  - Logging is important but shouldn't block compilation
  - Current fallback to stderr is acceptable
  - Document that full logging features require spdlog

### Q3: http_transport.cpp Missing from CMakeLists.txt
**What we know:**
- `src/mcpp/transport/http_transport.cpp` exists but is not in `MCPP_SOURCES`
- This will cause linker errors for anything using HTTP transport

**What's unclear:**
- Is this intentional (HTTP transport is optional/experimental)?

**Recommendation:** Add to `MCPP_SOURCES` in CMakeLists.txt. HTTP transport is part of Phase 4 deliverables and should be included.

### Q4: MCP Conformance Test Stability
**What we know:**
- Official conformance package exists but is marked "unstable" and "work in progress"
- Source: GitHub README includes explicit warning

**What's unclear:**
- Can we rely on it for CI, or should we build custom tests first?

**Recommendation:** Build custom JSON-RPC compliance tests as primary validation. Use official conformance as secondary/beta. Don't block release on unstable external tests.

## Sources

### Primary (HIGH confidence)
- CMake 4.2 Documentation - cmake-packages(7), ctest(1), SOVERSION property
  - https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html
  - https://cmake.org/cmake/help/latest/manual/ctest.1.html
  - https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html
- GoogleTest Quickstart CMake Guide
  - http://google.github.io/googletest/quickstart-cmake.html
- MCP Inspector (npm package)
  - https://www.npmjs.com/package/@modelcontextprotocol/inspector
- MCP Conformance Tests (GitHub)
  - https://github.com/modelcontextprotocol/conformance

### Secondary (MEDIUM confidence)
- CMake Code Coverage with gcovr/lcov
  - https://danielsieger.com/blog/2024/08/03/code-coverage-with-cmake.html
- GCC Sanitizers (ASan/TSan/LSan)
  - https://www.osc.edu/resources/getting_started/howto/howto_use_address_sanitizer
  - https://docs.fuchsia.dev/contribute/testing/sanitizers.html
- C++ Thread Safety Testing discussions
  - Various StackOverflow and Reddit threads (general consensus: use sanitizers)

### Tertiary (LOW confidence)
- WebSearch results on "MCP Inspector security testing" and "MCP best practices"
  - General advice confirmed with official sources above
- WebSearch results on "GoogleTest CMake best practices"
  - Confirmed with official GoogleTest documentation

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Based on official CMake and GoogleTest documentation
- Architecture: HIGH - CMake package configuration patterns are well-documented and stable
- Pitfalls: MEDIUM - C++17/20 conflict is a real ambiguity; other pitfalls are based on standard practice
- MCP testing: MEDIUM - Official conformance suite exists but is unstable; fallback approach is clear

**Research date:** 2026-02-01
**Valid until:** 2026-05-01 (CMake/GoogleTest stable; MCP conformance may change faster)

---

*Phase: 07-build-validation*
*Research completed: 2026-02-01*
