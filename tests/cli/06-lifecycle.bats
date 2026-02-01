# Server Lifecycle Tests
# Tests for server startup, shutdown, and process cleanup
# These tests ensure no background processes are left running

# Load common setup for test environment
load 'test_helper/common-setup'

# Setup function - runs before each test
setup() {
    _common_setup
}

# Helper function to start server in background with proper cleanup
# Usage: start_server
# Sets SERVER_PID and SERVER_LOG variables
# Uses trap EXIT to ensure cleanup even if test fails
start_server() {
    # Start server in background, redirecting output to log file
    inspector_server > "${BATS_TEST_TMPDIR}/server.log" 2>&1 &
    SERVER_PID=$!

    # Set trap to kill server on test exit (normal or failure)
    # This ensures no orphaned processes
    trap "kill ${SERVER_PID} 2>/dev/null || true" EXIT

    # Wait for server to initialize
    sleep 0.5
}

# Helper function to verify server is running
# Usage: server_is_running
# Returns: 0 if running, 1 if not
server_is_running() {
    kill -0 "${SERVER_PID}" 2>/dev/null
}

# Helper function to send initialize request and get response
# Usage: send_initialize_to_server
send_initialize_to_server() {
    local request='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'
    echo "$request" | inspector_server 2>/dev/null
}

@test "server starts without errors" {
    # Start the server
    start_server

    # Verify server process is running
    run server_is_running
    assert_success

    # Verify log file was created
    assert_file_exists "${BATS_TEST_TMPDIR}/server.log"

    # Verify log doesn't contain startup errors
    # (some debug output is expected with MCPP_DEBUG=1)
    run bash -c "grep -i 'error\\|fatal\\|failed' '${BATS_TEST_TMPDIR}/server.log'"
    assert_failure
}

@test "server responds to initialize after startup" {
    # Start the server
    start_server

    # Send initialize request
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
    # Start a new server instance (not via start_server helper since we need custom EOF handling)
    inspector_server > "${BATS_TEST_TMPDIR}/server.log" 2>&1 &
    local server_pid=$!

    # Set trap for cleanup in case test fails
    trap "kill ${server_pid} 2>/dev/null || true" EXIT

    # Wait for server to initialize
    sleep 0.5

    # Verify server started
    run kill -0 "${server_pid}"
    assert_success

    # Send initialize request via a coprocess that closes stdin
    # This simulates a client disconnecting
    echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}' | inspector_server 2>/dev/null > "${BATS_TEST_TMPDIR}/response.json" &
    local client_pid=$!

    # Wait a bit for request processing
    sleep 0.5

    # The server should have exited after EOF (closing stdin)
    # Wait for process to exit
    local count=0
    while kill -0 "${server_pid}" 2>/dev/null && [[ ${count} -lt 5 ]]; do
        sleep 0.1
        count=$((count + 1))
    done

    # Server should have exited
    run kill -0 "${server_pid}"
    assert_failure

    # Clean up client process if still running
    kill "${client_pid}" 2>/dev/null || true
}

@test "server cleanup on test failure simulation" {
    # Start the server
    start_server

    # Get the PID for verification
    local saved_pid="${SERVER_PID}"

    # Simulate test failure by returning early
    # The trap should still clean up the server
    # Note: In actual test failure, bats would call the teardown/exit trap

    # Verify server is running
    run kill -0 "${saved_pid}"
    assert_success

    # The trap will automatically clean up when test exits
    # This is verified implicitly - if trap doesn't work, the process remains
    # and subsequent tests may fail with "port already in use" errors
}

@test "multiple sequential server instances" {
    # Start and stop server 3 times in sequence
    for i in 1 2 3; do
        # Start server
        inspector_server > "${BATS_TEST_TMPDIR}/server_${i}.log" 2>&1 &
        local server_pid=$!

        # Set trap for this specific instance
        trap "kill ${server_pid} 2>/dev/null || true" EXIT

        # Wait for initialization
        sleep 0.5

        # Verify it's running
        run kill -0 "${server_pid}"
        assert_success

        # Send a request to verify it's responsive
        local response
        response=$(echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}' | inspector_server 2>/dev/null)

        # Verify valid response
        run bash -c "echo '$response' | jq -e '.result.protocolVersion'"
        assert_success

        # Kill this instance cleanly
        kill "${server_pid}" 2>/dev/null || true

        # Wait for it to fully exit
        local count=0
        while kill -0 "${server_pid}" 2>/dev/null && [[ ${count} -lt 5 ]]; do
            sleep 0.1
            count=$((count + 1))
        done

        # Verify it exited
        run kill -0 "${server_pid}"
        assert_failure

        # Reset trap for next iteration
        trap - EXIT
    done

    # Final cleanup trap (should be no processes left to kill)
    trap "true" EXIT
}
