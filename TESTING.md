# Testing mcpp with BATS

A code-first guide to testing MCP servers using the BATS framework. Learn by examining real tests, then write your own.

## Overview

mcpp uses BATS (Bash Automated Testing System) for CLI integration testing. Tests validate MCP protocol behavior over stdio transport by:

- Sending JSON-RPC requests to inspector_server
- Validating response structure with jq
- Checking for correct tools, resources, and prompts

**Philosophy:** Learn by example. Walk through existing tests to understand patterns, then apply them to your own servers.

## Test Infrastructure

### Location

All CLI tests are in `tests/cli/`:

```
tests/cli/
├── 01-tools-list.bats       # Tool discovery tests
├── 02-tools-call.bats       # Tool execution tests
├── 03-resources.bats        # Resource serving tests
├── 04-prompts.bats          # Prompt template tests
├── 05-initialize.bats       # Handshake tests
├── 06-lifecycle.bats        # Server startup/shutdown tests
├── test_helper/
│   ├── common-setup.bash    # Shared setup function
│   ├── bats-assert/         # Assertion library (Git submodule)
│   ├── bats-support/        # Helper library (Git submodule)
│   └── bats-file/           # Filesystem assertions (Git submodule)
```

### Prerequisites

```bash
# Install jq (JSON processor for assertions)
sudo apt install jq  # Debian/Ubuntu
brew install jq      # macOS

# BATS libraries are included as Git submodules
git submodule update --init --recursive
```

### Running Tests

```bash
# Run all CLI tests via CMake/CTest
cd build && ctest -L cli

# Run specific test file directly
bats tests/cli/01-tools-list.bats

# Run with debug output
MCPP_DEBUG=1 bats tests/cli/01-tools-list.bats

# Run tests verbosely
bats --verbose tests/cli/01-tools-list.bats

# Run with BATS timing information
bats --timing tests/cli/01-tools-list.bats
```

## Reading Existing Tests

Let's walk through `tests/cli/01-tools-list.bats` to understand the patterns:

```bash
#!/usr/bin/env bats
# Load common setup for test environment
load 'test_helper/common-setup'

# Setup function - runs before each test
setup() {
    _common_setup
}

# Helper function to send both initialize and tools/list requests
# MCP protocol requires initialize before tools/list
send_tools_list() {
    local initialize='{"jsonrpc":"2.0","id":1,"method":"initialize",...}'
    local tools_list='{"jsonrpc":"2.0","id":2,"method":"tools/list"}'
    {
        echo "$initialize"
        echo "$tools_list"
    } | inspector_server 2>/dev/null | tail -n 1
}

@test "tools/list returns valid JSON-RPC response" {
    run send_tools_list

    # Command should succeed
    assert_success

    # Response should contain jsonrpc version
    assert_output --partial '"jsonrpc":"2.0"'

    # Response should contain id
    assert_output --partial '"id":2'

    # Response should contain result
    assert_output --partial '"result"'
}
```

**What's happening:**

1. **`load 'test_helper/common-setup'`** - Loads shared test environment
   - Sets PATH to include build/examples/
   - Exports MCPP_DEBUG=1 for debugging
   - Provides helper functions like `inspector_server_path()`

2. **`setup() { _common_setup; }`** - Runs before each test
   - Ensures consistent environment
   - Prevents test pollution

3. **Helper function `send_tools_list()`** - Encapsulates common pattern
   - Sends initialize request first (MCP requirement)
   - Sends tools/list request
   - Uses `tail -n 1` to get only the final response

4. **`run send_tools_list`** - Executes command and captures output
   - Exit code stored in `$status`
   - Output stored in `$output`

5. **`assert_success`** - Verifies exit code was 0

6. **`assert_output --partial`** - Checks output contains substring

## Common Patterns

### Pattern 1: Initialize Before Request

MCP requires initialize handshake before most operations:

```bash
# WRONG: skips initialize
echo '{"jsonrpc":"2.0","method":"tools/list","id":1}' | server

# RIGHT: initialize first
{
    echo '{"jsonrpc":"2.0","id":1,"method":"initialize",...}'
    echo '{"jsonrpc":"2.0","id":2,"method":"tools/list"}'
} | server
```

The initialize handshake establishes the protocol version and client capabilities. Without it, most MCP servers will not respond to subsequent requests.

### Pattern 2: Suppress Debug Output

Use `2>/dev/null` to prevent MCPP_DEBUG logging from polluting test output:

```bash
echo request | inspector_server 2>/dev/null
```

Without this, debug logs get mixed with JSON-RPC responses, causing jq parsing failures and assertion errors.

### Pattern 3: Extract Final Response

When sending multiple requests, use `tail -n 1` to get the last response:

```bash
{
    echo '{"jsonrpc":"2.0","id":1,"method":"initialize",...}'
    echo '{"jsonrpc":"2.0","id":2,"method":"tools/list"}'
} | inspector_server 2>/dev/null | tail -n 1
```

This is necessary because the server responds to each request. For testing a specific endpoint, we only care about the final response.

### Pattern 4: Validate JSON Structure

Use `jq -e` to validate JSON structure and exit code:

```bash
# Check that result.tools array exists
echo "$response" | jq -e '.result.tools' >/dev/null

# Check that specific tool exists
echo "$response" | jq -e '.result.tools[] | select(.name == "calculate")' >/dev/null

# Verify nested property
echo "$response" | jq -e '.result.tools[0].inputSchema.properties' >/dev/null
```

The `-e` flag makes jq exit with status 1 if the output is empty or null, which is perfect for assertions.

### Pattern 5: Inline JSON Requests

Use inline JSON in single quotes to avoid subshell export issues:

```bash
# GOOD: inline JSON
run bash -c '{
    echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",...}"
    echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}"
} | inspector_server 2>/dev/null | tail -1 | jq'

# BAD: using variables that might not export correctly
local request='{"jsonrpc":"2.0",...}'
echo "$request" | server  # Can fail with complex JSON
```

## Writing Custom Tests

### Test File Template

Create a new test file in `tests/cli/`:

```bash
#!/usr/bin/env bats
# mcpp - MCP C++ library tests
# https://github.com/mcpp-project/mcpp
#
# Test file for [your feature description]
# Tests [what this test file covers]

# Load common setup for test environment
load 'test_helper/common-setup'

# Setup function - runs before each test
setup() {
    _common_setup
}

# Optional: teardown for cleanup
teardown() {
    # Cleanup runs even on test failure
    # Common use: kill background processes, remove temp files
    if [[ -n "${SERVER_PID:-}" ]] && kill -0 "${SERVER_PID}" 2>/dev/null; then
        kill "${SERVER_PID}" 2>/dev/null || true
    fi
}

@test "[descriptive test name]" {
    # Send initialize + request
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"your/method\",\"params\":{}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    # Verify success
    assert_success

    # Verify response structure
    assert_output --partial '"jsonrpc":"2.0"'
    assert_output --partial '"result"'

    # Verify specific content using jq
    echo "$output" | jq -e '.result.field == "expected"' >/dev/null
    assert_equal $? 0
}
```

### Helper Functions Available

From `test_helper/common-setup.bash`:

```bash
# Get absolute path to inspector_server
inspector_server_path  # Returns: /path/to/project/build/examples/inspector_server

# Check if inspector_server is built
inspector_server_exists  # Returns 0 if built, 1 if not

# Wait for a file to appear (for async tests)
wait_for_file <path> <max_seconds>

# Wait for content in a file
wait_for_content <file> <string> <max_seconds>
```

### Best Practices

1. **Use descriptive test names**
   ```bash
   @test "tools/list returns expected tools"  # GOOD
   @test "test tools"  # BAD - too vague
   ```

2. **Test one thing per test**
   ```bash
   @test "tools/list returns valid JSON-RPC response"  # Tests structure only
   @test "tools/list returns expected tools"  # Tests content only
   ```

3. **Use helper functions for repeated patterns**
   ```bash
   # Define once
   send_initialize() {
       echo '{"jsonrpc":"2.0","id":1,"method":"initialize",...}'
   }

   # Use in multiple tests
   @test "test 1" {
       run bash -c '{ send_initialize; echo request; } | server'
   }
   ```

4. **Validate both structure and content**
   ```bash
   # Check structure
   assert_output --partial '"jsonrpc":"2.0"'
   assert_output --partial '"result"'

   # Check content
   echo "$output" | jq -e '.result.tools | length > 0' >/dev/null
   ```

5. **Use jq for JSON assertions**
   ```bash
   # Extract field
   local tool_count
   tool_count=$(echo "$output" | jq '.result.tools | length')

   # Assert on value
   [ "$tool_count" -gt 0 ]

   # Check nested properties
   echo "$output" | jq -e '.result.tools[] | select(.name == "calculate")' >/dev/null
   ```

6. **Clean up resources in teardown**
   ```bash
   teardown() {
       # Always cleanup, even if test fails
       if [[ -n "${SERVER_PID:-}" ]]; then
           kill "${SERVER_PID}" 2>/dev/null || true
       fi
   }
   ```

## CMake Integration

CLI tests are integrated with CTest:

```bash
# Run all CLI tests via CMake
ctest --test-dir build -L cli

# Run with verbose output
ctest --test-dir build -L cli --verbose

# Run specific test
ctest --test-dir build -R cli-tools-list --verbose
```

CLI tests run serially to prevent conflicts (RUN_SERIAL TRUE property set in CMakeLists.txt).

The CLI test target is registered in `tests/CMakeLists.txt`:

```cmake
if(BATS_PROGRAM AND JQ_PROGRAM)
    add_test(NAME cli-tools-list COMMAND ${BATS_PROGRAM}
        ${CMAKE_CURRENT_SOURCE_DIR}/cli/01-tools-list.bats
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(cli-tools-list PROPERTIES
        LABELS "cli"
        RUN_SERIAL TRUE)
endif()
```

## Troubleshooting

### "inspector_server: command not found"

**Symptoms:** Tests fail with command not found error.

**Cause:** Binary not built or not in PATH.

**Solution:**
```bash
# Rebuild the example server
cmake --build build --target inspector_server

# Verify binary exists
ls -la build/examples/inspector_server

# Check PATH in test (should include build/examples)
echo $PATH
```

### Tests timing out

**Symptoms:** BATS tests hang indefinitely.

**Cause:** Server not responding, stdio buffering issue.

**Solution:**
```bash
# Check debug output
MCPP_DEBUG=1 bats tests/cli/01-tools-list.bats

# Verify server responds to manual request
echo '{"jsonrpc":"2.0","id":1,"method":"initialize",...}' | ./build/examples/inspector_server

# Check for orphaned processes
ps aux | grep inspector_server
```

### "jq: command not found"

**Symptoms:** Tests using jq fail.

**Cause:** jq not installed.

**Solution:**
```bash
# Debian/Ubuntu
sudo apt install jq

# macOS
brew install jq

# Verify installation
jq --version
```

### "parse error: Invalid literal" in jq

**Symptoms:** jq fails to parse response.

**Cause:** Response not valid JSON (might contain debug output).

**Solution:**
```bash
# Ensure stderr is suppressed
echo request | server 2>/dev/null | jq

# Check what's being parsed
echo request | server 2>/dev/null | cat

# Verify response is complete
echo request | server 2>/dev/null | jq '.'
```

### BATS submodules not found

**Symptoms:** Tests fail with "load.bash: No such file or directory".

**Cause:** Git submodules not initialized.

**Solution:**
```bash
git submodule update --init --recursive

# Verify submodules are present
ls tests/cli/test_helper/bats-support/load.bash
ls tests/cli/test_helper/bats-assert/load.bash
```

### Tests pass locally but fail in CI

**Symptoms:** Local tests pass, CI tests fail.

**Cause:** Environment differences (PATH, missing dependencies).

**Solution:**
- Check CI workflow matches local environment
- Verify `MCPP_DEBUG` is exported in tests
- Ensure PATH includes build/examples/
- Check that Git submodules are checked out in CI (`submodules: recursive`)

### "Permission denied" running test files

**Symptoms:** Cannot execute .bats files.

**Cause:** Test files not executable.

**Solution:**
```bash
chmod +x tests/cli/*.bats

# Or run via bats explicitly
bats tests/cli/01-tools-list.bats
```

### Tests leave zombie processes

**Symptoms:** Subsequent tests fail or behave unexpectedly.

**Cause:** Background server processes not cleaned up.

**Solution:**
```bash
# Kill any orphaned inspector_server processes
pkill -f inspector_server

# Ensure your test has a teardown function
teardown() {
    if [[ -n "${SERVER_PID:-}" ]] && kill -0 "${SERVER_PID}" 2>/dev/null; then
        kill "${SERVER_PID}" 2>/dev/null || true
    fi
}
```

## See Also

- [README.md](README.md) - Quick start with MCP Inspector
- [examples/TESTING.md](examples/TESTING.md) - Manual Inspector testing guide
- [BATS-core documentation](https://bats-core.readthedocs.io/)
- [jq manual](https://stedolan.github.io/jq/)
- [MCP Protocol Specification](https://modelcontextprotocol.io/docs/concepts/servers/)
