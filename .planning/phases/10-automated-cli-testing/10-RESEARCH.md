# Phase 10: Automated CLI Testing - Research

**Researched:** 2026-02-01
**Domain:** Bats-core CLI testing framework with CMake integration
**Confidence:** HIGH

## Summary

Phase 10 requires implementing an automated CLI testing suite using bats-core 1.11.0+ to validate all MCP methods via the Inspector server. Research reveals that **bats-core** is the established standard for bash CLI testing, with a well-documented integration pattern with CMake using `add_test()` and `find_program(BASH_PROGRAM bash)`.

The core testing approach involves:
1. **bats-core** as the test framework with **bats-support**, **bats-assert**, and **bats-file** helper libraries
2. **jq** for JSON response validation and extraction
3. **CMake integration** via `add_test()` pointing to the bats executable
4. **Process lifecycle management** using bash traps to prevent server leaks

The existing `inspector_server` example already implements MCP stdio transport with Content-Length headers (from Phase 9), providing a solid test target. The gopher-mcp reference implementation shows shell testing patterns with background process management using PID tracking and `trap` cleanup.

**Primary recommendation:** Use bats-core with helper libraries (bats-support, bats-assert, bats-file) as Git submodules, integrate with CMake using `find_program(BASH_PROGRAM bash)` and `add_test(NAME cli-tests COMMAND bats ...)`.

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| bats-core | 1.11.0+ | Bash CLI testing framework | TAP-compliant, de facto standard for bash testing, integrates with CTest |
| bats-support | latest | Helper library for bats-core | Required by bats-assert, provides common test utilities |
| bats-assert | latest | Assertion library for bats-core | Provides `assert_output`, `assert_success`, `assert_failure`, etc. |
| bats-file | latest | Filesystem assertions for bats-core | Provides `assert_file_exists`, `assert_dir_exists`, etc. |
| jq | 1.6+ | JSON processor for output validation | Standard CLI tool for JSON parsing, widely available |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| mcpp-inspector-server | local | MCP server test target | The Phase 9 example server implements all MCP methods |
| bash | 4.0+ | Shell for test execution | Required by bats-core |
| CMake | 3.16+ | Build system integration | Using `add_test()` for CTest integration |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| bats-core | Shell scripts with manual assertions | No TAP output, harder to maintain, no CTest integration |
| jq | Python/Node.js JSON parsing | Additional dependency complexity, slower startup |
| Git submodules for bats | System package install | Reproducibility issues across platforms, version skew |

**Installation:**
```bash
# Add as Git submodules for reproducible builds
git submodule add https://github.com/bats-core/bats-core.git test/bats
git submodule add https://github.com/bats-core/bats-support.git test/test_helper/bats-support
git submodule add https://github.com/bats-core/bats-assert.git test/test_helper/bats-assert
git submodule add https://github.com/bats-core/bats-file.git test/test_helper/bats-file

# System dependencies (for CI/CD)
apt-get install bats jq  # Debian/Ubuntu
brew install bats jq     # macOS
```

## Architecture Patterns

### Recommended Project Structure

```
mcpp/
├── tests/
│   ├── cli/                    # NEW: CLI tests
│   │   ├── bats/               # bats-core Git submodule
│   │   ├── test_helper/        # Bats helper libraries
│   │   │   ├── bats-support/   # Git submodule
│   │   │   ├── bats-assert/    # Git submodule
│   │   │   ├── bats-file/      # Git submodule
│   │   │   └── common-setup.sh # Custom test setup
│   │   ├── 01-tools-list.bats  # Tools/list endpoint tests
│   │   ├── 02-tools-call.bats  # Tools/call endpoint tests
│   │   ├── 03-resources.bats   # Resources endpoints
│   │   ├── 04-prompts.bats     # Prompts endpoints
│   │   ├── 05-initialize.bats  # Initialize/protocol tests
│   │   └── 06-lifecycle.bats   # Server lifecycle tests
│   ├── unit/                   # Existing GoogleTest unit tests
│   ├── integration/            # Existing integration tests
│   └── CMakeLists.txt          # Test CMake configuration
├── examples/
│   └── inspector_server        # Test target from Phase 9
└── CMakeLists.txt
```

### Pattern 1: Common Setup Helper

**What:** A centralized setup function loaded by all test files to provide consistent environment, PATH configuration, and helper library loading.

**When to use:** All bats test files should load this via `setup()` function.

**Example:**
```bash
# Source: https://bats-core.readthedocs.io/en/stable/_sources/tutorial
#!/usr/bin/env bash

_common_setup() {
    # Load helper libraries
    load 'test_helper/bats-support/load'
    load 'test_helper/bats-assert/load'
    load 'test_helper/bats-file/load'

    # Get the containing directory of this file
    PROJECT_ROOT="$( cd "$( dirname "$BATS_TEST_FILENAME" )/../.." >/dev/null 2>&1 && pwd )"

    # Make executables visible to PATH
    PATH="$PROJECT_ROOT/build/examples:$PATH"

    # Set debug flag for inspector_server
    export MCPP_DEBUG=1
}
```

### Pattern 2: Server Lifecycle Management

**What:** Start server in background, communicate via stdio, ensure cleanup on test exit/failure.

**When to use:** All tests that interact with the inspector_server via stdio.

**Example:**
```bash
# Start server in background with trap for cleanup
setup() {
    load 'test_helper/common-setup'
    _common_setup

    # Start inspector_server in background
    inspector_server > "$BATS_TEST_TMPDIR/server.log" 2>&1 &
    SERVER_PID=$!

    # Trap to ensure cleanup even on test failure
    trap "kill $SERVER_PID 2>/dev/null || true; wait $SERVER_PID 2>/dev/null || true" EXIT

    # Give server time to initialize
    sleep 0.5
}

teardown() {
    # Cleanup is handled by trap, but can add additional checks here
    # Verify server was actually running
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo "Server died during test"
        cat "$BATS_TEST_TMPDIR/server.log"
        return 1
    fi
}
```

### Pattern 3: JSON-RPC Request/Response Testing

**What:** Send JSON-RPC requests via stdin, parse responses with jq, validate structure and content.

**When to use:** Testing MCP protocol methods (tools/list, tools/call, resources/list, prompts/list).

**Example:**
```bash
@test "tools/list returns valid JSON-RPC response" {
    # Send initialize request first
    echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}' | inspector_server

    # Send tools/list request
    run bash -c 'echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}" | inspector_server'

    # Check exit status
    assert_success

    # Validate JSON response
    assert_output --partial '"jsonrpc":"2.0"'
    assert_output --partial '"id":2'
    assert_output --partial '"result"'
}

@test "tools/list contains expected tools" {
    # Send initialize and tools/list
    {
        echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'
        echo '{"jsonrpc":"2.0","id":2,"method":"tools/list"}'
    } | inspector_server | jq -e '.result.tools[] | select(.name == "calculate")'

    # jq returns 0 if match found, 1 otherwise
    assert_equal $? 0
}
```

### Pattern 4: CMake Integration

**What:** Use CMake's `find_program()` and `add_test()` to integrate bats tests with CTest.

**When to use:** Adding CLI tests to the CMake build system.

**Example:**
```cmake
# Source: https://stackoverflow.com/questions/25627336/integrate-bash-test-scripts-in-cmake

# Find bats and jq
find_program(BATS_PROGRAM bats)
find_program(JQ_PROGRAM jq)

# Add CLI tests only if bats and jq are available
if(BATS_PROGRAM AND JQ_PROGRAM)
    # Add a test that runs the entire bats suite
    add_test(
        NAME cli-tests
        COMMAND ${BATS_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/cli
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Set test labels
    set_tests_properties(cli-tests PROPERTIES LABELS "cli;bats")

    # Individual test files can also be added
    add_test(
        NAME cli-tools-list
        COMMAND ${BATS_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/cli/01-tools-list.bats
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()
```

### Anti-Patterns to Avoid

- **Manual process cleanup without traps**: Always use `trap cleanup EXIT` to ensure background processes are killed even on test failure.
- **Hardcoding absolute paths**: Use `$BATS_TEST_FILENAME`, `$PROJECT_ROOT`, or `$BATS_TEST_TMPDIR` for portability.
- **Assuming server is ready without checking**: Add delays or health checks after starting background processes.
- **Ignoring stderr output**: Always capture and check both stdout and stderr from the server.
- **Not validating JSON structure**: Use `jq -e` to validate JSON before extracting values.
- **Testing without initializing first**: MCP protocol requires initialize call before most operations.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Bash test assertions | Custom `if [[ ... ]]` checks | bats-assert assertions (`assert_output`, `assert_success`) | Better error messages, consistent API, TAP output |
| JSON validation in bash | Complex regex/grep patterns | `jq -e '.field'` | Handles edge cases, validates structure, readable |
| Process cleanup tracking | Manual PID bookkeeping | `trap 'func' EXIT` pattern | Guaranteed cleanup even on failure/timeout |
| Test discovery in CMake | Manually listing each .bats file | `add_test(NAME cli-tests COMMAND bats ./dir/)` | Bats auto-discovers tests, CTest groups them |
| File system tests | Custom `[ -f file ]` checks | bats-file (`assert_file_exists`, `assert_dir_exists`) | Better error messages, negations supported |

**Key insight:** Building custom test infrastructure in bash is error-prone and hard to maintain. The bats-core ecosystem provides battle-tested solutions for common testing patterns.

## Common Pitfalls

### Pitfall 1: Process Leaks from Background Servers

**What goes wrong:** Test starts server in background, test fails or times out, server process keeps running.

**Why it happens:** Background processes (`&`) continue running even if parent script exits. Manual cleanup only runs on success paths.

**How to avoid:** Always use `trap cleanup_function EXIT` to ensure cleanup runs regardless of test outcome.

**Warning signs:** `ps aux | grep inspector_server` shows multiple instances after test runs.

**Solution:**
```bash
setup() {
    inspector_server > /dev/null 2>&1 &
    SERVER_PID=$!
    trap "kill $SERVER_PID 2>/dev/null || true" EXIT
}
```

### Pitfall 2: Race Conditions in Server Startup

**What goes wrong:** Test sends requests immediately after starting server, gets connection errors or EOF.

**Why it happens:** `&` starts the process but doesn't wait for initialization. Server needs time to set up stdio handlers.

**How to avoid:** Add `sleep 0.5` after starting background processes, or implement health check loop.

**Warning signs:** Tests fail intermittently, especially on CI/CD or slower machines.

**Solution:**
```bash
setup() {
    inspector_server > /dev/null 2>&1 &
    SERVER_PID=$!
    trap "kill $SERVER_PID 2>/dev/null || true" EXIT
    sleep 0.5  # Allow server initialization

    # Optional: health check
    timeout 2s bash -c "while ! kill -0 $SERVER_PID 2>/dev/null; do sleep 0.1; done"
}
```

### Pitfall 3: Stdio Buffering Issues

**What goes wrong:** Output doesn't appear immediately, tests timeout waiting for responses.

**Why it happens:** stdio buffering (especially when piping through bash). Server may be block-buffered instead of line-buffered.

**How to avoid:** Server should flush stdout after each response. Tests should handle partial reads.

**Warning signs:** Tests hang indefinitely, no output from server.

**Solution:**
```cpp
// In inspector_server, after writing response:
std::cout << response_str << std::flush;  // Always flush
```

### Pitfall 4: Not Validating JSON Before Parsing

**What goes wrong:** `jq` fails silently or produces confusing errors when input isn't valid JSON.

**Why it happens:** Server may output error messages, debug logs, or incomplete JSON to stdout.

**How to avoid:** Use `jq -e` for validation, check exit status before extracting values.

**Warning signs:** Tests pass but jq errors appear in output, or tests fail with "parse error".

**Solution:**
```bash
@test "validate JSON response" {
    run bash -c 'echo "{}" | inspector_server'
    assert_success

    # Validate JSON before parsing
    echo "$output" | jq -e '.' >/dev/null
    assert_equal $? 0  # jq returns 0 on valid JSON

    # Now extract values safely
    result=$(echo "$output" | jq -r '.result')
}
```

### Pitfall 5: CMake Tests Not Finding Test Executables

**What goes wrong:** CTest runs but fails because `inspector_server` isn't in PATH.

**Why it happens:** `add_test()` doesn't automatically add build directory to PATH. Tests assume executables are available.

**How to avoid:** Set `PATH` in test environment or use absolute paths to build artifacts.

**Warning signs:** `inspector_server: command not found` in test output.

**Solution:**
```cmake
# In tests/CMakeLists.txt
set_tests_properties(cli-tests PROPERTIES
    ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/examples:$ENV{PATH}"
)
```

## Code Examples

Verified patterns from official sources:

### Basic Bats Test with Setup

```bash
# Source: https://bats-core.readthedocs.io/en/stable/_sources/tutorial
setup() {
    load 'test_helper/bats-support/load'
    load 'test_helper/bats-assert/load'

    DIR="$( cd "$( dirname "$BATS_TEST_FILENAME" )" >/dev/null 2>&1 && pwd )"
    PATH="$DIR/../src:$PATH"
}

@test "can run our script" {
    run project.sh
    assert_output 'Welcome to our project!'
}
```

### JSON-RPC MCP Request Test

```bash
@test "initialize request returns valid protocol version" {
    run bash -c 'echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}" | inspector_server | jq'

    assert_success
    assert_output --partial '"result"'
    assert_output --partial '"protocolVersion":"2025-11-25"'
}
```

### Tool Call with Arguments

```bash
@test "tools/call with calculate tool returns result" {
    # First initialize
    echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}' | inspector_server > /dev/null

    # Then call calculate tool
    run bash -c 'echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"calculate\",\"arguments\":{\"operation\":\"add\",\"a\":5,\"b\":3}}}" | inspector_server | jq'

    assert_success

    # Verify response structure
    echo "$output" | jq -e '.result.content[0].text == "8"' >/dev/null
    assert_equal $? 0
}
```

### Resources List Test

```bash
@test "resources/list returns registered resources" {
    # Initialize first
    echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}' | inspector_server > /dev/null

    run bash -c 'echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/list\"}" | inspector_server | jq'

    assert_success

    # Check for expected resources
    echo "$output" | jq -e '.result.resources[] | select(.uri == "info://server")' >/dev/null
    assert_equal $? 0
}
```

### Server Lifecycle Helper Functions

```bash
# Helper to start inspector_server with proper cleanup
start_inspector_server() {
    local log_file="${1:-$BATS_TEST_TMPDIR/server.log}"

    inspector_server > "$log_file" 2>&1 &
    echo $!  # Return PID
}

# Helper to stop server and check logs
stop_inspector_server() {
    local server_pid=$1
    local log_file="${2:-$BATS_TEST_TMPDIR/server.log}"

    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true

    # Fail if server crashed
    if ! grep -q "Server shutting down" "$log_file" 2>/dev/null; then
        echo "Server may have crashed:"
        cat "$log_file"
        return 1
    fi
}

# Usage in test
@test "full request lifecycle" {
    SERVER_PID=$(start_inspector_server)
    trap "stop_inspector_server $SERVER_PID" EXIT

    # Run tests...
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual bash scripts with `[ ]` tests | bats-core with TAP output | ~2015 (bats-core matured) | Better test organization, CI/CD integration |
| Subprocess cleanup with `pkill` | `trap cleanup EXIT` pattern | Established best practice | Reliable cleanup, no process leaks |
| Inline test assertions | bats-assert/bats-file helper libraries | ~2019 | More readable tests, better error messages |
| Individual test CMake targets | Single `bats` call with test discovery | Ongoing best practice | Simpler CMake, faster test runs |

**Deprecated/outdated:**
- **bats v0.4.0 syntax**: The preprocessor changed significantly in v1.0+. Always use current syntax.
- **`set -e` in test files**: Bats handles errors internally; `set -e` can interfere.
- **Manual test discovery**: Bats auto-discovers `@test` functions; don't manually list tests in CMake.

## Open Questions

1. **MCP Inspector CLI availability**
   - What we know: Phase 9 verified inspector_server works with `mcp-inspector connect stdio`
   - What's unclear: Whether `mcp-inspector` CLI should be used for tests or if direct stdio communication is preferred
   - Recommendation: Use direct stdio communication for tests (no external dependency on mcp-inspector CLI), but keep inspector available for manual testing

2. **Test isolation for parallel execution**
   - What we know: CTest can run tests in parallel with `--parallel` flag
   - What's unclear: Whether the inspector_server can handle multiple concurrent test runs
   - Recommendation: Use `set_tests_properties(cli-tests PROPERTIES RUN_SERIAL TRUE)` initially to ensure serial execution, investigate parallelization later

3. **Temporary file location for test data**
   - What we know: Bats provides `$BATS_TEST_TMPDIR` for temporary files
   - What's unclear: Where to put persistent test data (e.g., test resources)
   - Recommendation: Use `$BATS_TEST_TMPDIR` for ephemeral data, create `tests/cli/fixtures/` for persistent test fixtures

## Sources

### Primary (HIGH confidence)
- [/bats-core/bats-core](https://context7.com/bats-core/bats-core/llms.txt) - Bats-core installation, test structure, setup/teardown hooks
- [/websites/bats-core_readthedocs_io_en_stable](https://bats-core.readthedocs.io/en/stable/) - Official Bats documentation, tutorial, writing tests guide
- [/bats-core/bats-file](https://github.com/bats-core/bats-file) - Filesystem assertions (assert_file_exists, assert_dir_exists)
- [/jqlang/jq](https://context7.com/jqlang/jq/llms.txt) - JSON validation and parsing
- [StackOverflow: Integrate bash test scripts in cmake](https://stackoverflow.com/questions/25627336/integrate-bash-test-scripts-in-cmake) - CMake integration pattern with `find_program(BASH_PROGRAM bash)` and `add_test()`

### Secondary (MEDIUM confidence)
- [Atomic Object: Bash background process cleanup](https://spin.atomicobject.com/start-stop-bash-background-process/) - Using `kill 0` for process group cleanup
- [LinuxConfig: Propagate signals to child processes](https://linuxconfig.org/how-to-propagate-a-signal-to-child-processes-from-a-bash-script) - Signal handling for background processes
- [StackOverflow: Kill background processes on exit](https://stackoverflow.com/questions/360201/how-do-i-kill-background-processes-jobs-when-my-shell-script-exits) - Trap patterns for cleanup
- [HackerOne: Testing Bash Scripts with BATS](https://www.hackerone.com/blog/testing-bash-scripts-bats-practical-guide) - Bats testing patterns and best practices

### Tertiary (LOW confidence)
- [gopher-mcp examples: test_advanced_echo.sh](thirdparty/gopher-mcp/examples/stdio_echo/test_advanced_echo.sh) - Reference implementation of bash test patterns (examined locally)
- [gopher-mcp shell tests](thirdparty/gopher-mcp/shell/test_mcp_*.sh) - Example shell tests for MCP servers (examined locally)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - bats-core is the established standard for bash CLI testing, with official documentation and widespread adoption
- Architecture: HIGH - CMake integration pattern verified via StackOverflow, process lifecycle patterns from multiple authoritative sources
- Pitfalls: MEDIUM - Process leaks and race conditions are well-documented in bash testing literature, but specific MCP stdio testing patterns need validation

**Research date:** 2026-02-01
**Valid until:** 2026-03-01 (30 days - stable tooling domain)
