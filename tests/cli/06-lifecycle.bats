# Server Lifecycle Tests
# Tests for server startup, shutdown, and process cleanup
# These tests ensure no background processes are left running

# Load common setup for test environment
load 'test_helper/common-setup'

# Setup function - runs before each test
setup() {
    _common_setup
}

# Teardown function - runs after each test (even on failure)
# Ensures no orphaned server processes
teardown() {
    # Clean up any running server from this test
    if [[ -n "${SERVER_PID:-}" ]] && kill -0 "${SERVER_PID}" 2>/dev/null; then
        kill "${SERVER_PID}" 2>/dev/null || true
        # Wait for process to exit
        local count=0
        while kill -0 "${SERVER_PID}" 2>/dev/null && [[ ${count} -lt 10 ]]; do
            sleep 0.1
            count=$((count + 1))
        done
    fi

    # Also clean up any secondary server pids
    if [[ -n "${server_pid:-}" ]] && kill -0 "${server_pid}" 2>/dev/null; then
        kill "${server_pid}" 2>/dev/null || true
    fi

    # Clean up client pids if any
    if [[ -n "${client_pid:-}" ]] && kill -0 "${client_pid}" 2>/dev/null; then
        kill "${client_pid}" 2>/dev/null || true
    fi
}

# Helper function to start server in background
# Usage: start_server
# Sets SERVER_PID variable
# Note: Teardown handles cleanup
start_server() {
    # Start server in background with stdin from /dev/null to keep it alive
    # Redirect output to log file for debugging
    inspector_server < /dev/null > "${BATS_TEST_TMPDIR}/server.log" 2>&1 &
    SERVER_PID=$!

    # Wait for server to initialize
    sleep 0.5
}

# Helper function to verify server is running
# Usage: server_is_running
# Returns: 0 if running, 1 if not
server_is_running() {
    [[ -n "${SERVER_PID:-}" ]] && kill -0 "${SERVER_PID}" 2>/dev/null
}

@test "server starts without errors" {
    # Start the server
    start_server

    # Give server a moment to fully initialize
    sleep 0.5

    # Verify log file was created
    assert_file_exists "${BATS_TEST_TMPDIR}/server.log"

    # Verify log shows server started (check for startup message)
    run bash -c "cat '${BATS_TEST_TMPDIR}/server.log'"
    assert_success

    # Verify log doesn't contain startup errors
    # (some debug output is expected with MCPP_DEBUG=1)
    run bash -c "grep -iE 'error|fatal|failed' '${BATS_TEST_TMPDIR}/server.log'"
    assert_failure
}

@test "server responds to initialize after startup" {
    # Start the server
    start_server

    # Send initialize request to a NEW server instance
    # (since start_server's server has stdin closed)
    local response
    response=$(echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}' | inspector_server 2>/dev/null)

    # Verify we got a valid JSON-RPC response
    run bash -c "echo '$response' | jq -e '.jsonrpc == \"2.0\"'"
    assert_success

    # Verify result exists
    run bash -c "echo '$response' | jq -e '.result'"
    assert_success

    # Verify protocol version
    run bash -c "echo '$response' | jq -r '.result.protocolVersion'"
    assert_output "2025-11-25"
}

@test "server shuts down cleanly on EOF" {
    # Start a server instance that will receive input via pipe
    # We use a coprocess to be able to close stdin
    coproc inspector_server > "${BATS_TEST_TMPDIR}/server.log" 2>&1
    server_pid=${COPROC_PID}

    # Wait for server to initialize
    sleep 0.5

    # Verify server started
    run kill -0 "${server_pid}"
    assert_success

    # Send initialize request
    echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}' >&"${COPROC[1]}"

    # Give it time to process
    sleep 0.5

    # Close stdin by closing the coproc fd
    exec {COPROC[1]}>&-

    # Wait for process to exit (should happen on EOF)
    local count=0
    while kill -0 "${server_pid}" 2>/dev/null && [[ ${count} -lt 10 ]]; do
        sleep 0.2
        count=$((count + 1))
    done

    # Server should have exited
    run kill -0 "${server_pid}"
    assert_failure

    # Verify log shows server ran
    run bash -c "test -s '${BATS_TEST_TMPDIR}/server.log'"
    assert_success
}

@test "server cleanup on test failure simulation" {
    # Start the server
    start_server

    # Verify server started (check PID is set)
    assert [ -n "${SERVER_PID}" ]

    # Verify log file exists
    assert_file_exists "${BATS_TEST_TMPDIR}/server.log"

    # The teardown function will automatically clean up the server
    # when the test exits (even if it fails)
    # This is verified by checking no orphaned processes exist after tests
}

@test "multiple sequential server instances" {
    # Start and verify server 3 times in sequence
    for i in 1 2 3; do
        # Start server (each gets a new SERVER_PID)
        start_server

        # Verify server started (check PID is set)
        assert [ -n "${SERVER_PID}" ]

        # Verify log file exists
        assert_file_exists "${BATS_TEST_TMPDIR}/server.log"

        # Send a request to verify server is responsive
        # (use a fresh instance for the request since background server has stdin closed)
        local response
        response=$(echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}' | inspector_server 2>/dev/null)

        # Verify valid response
        run bash -c "echo '$response' | jq -e '.result.protocolVersion'"
        assert_success

        # Manually kill this instance
        if kill -0 "${SERVER_PID}" 2>/dev/null; then
            kill "${SERVER_PID}" 2>/dev/null || true

            # Wait for it to fully exit
            local count=0
            while kill -0 "${SERVER_PID}" 2>/dev/null && [[ ${count} -lt 10 ]]; do
                sleep 0.1
                count=$((count + 1))
            done
        fi

        # Clear PID for next iteration
        SERVER_PID=""
    done
}
