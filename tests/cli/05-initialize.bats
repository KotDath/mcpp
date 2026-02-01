# MCP Initialize Handshake Tests
# Tests for the JSON-RPC initialize handshake that establishes protocol version

# Load common setup for test environment
load 'test_helper/common-setup'

# Setup function - runs before each test
setup() {
    _common_setup
}

# Helper function to send initialize request and get response
# Usage: send_initialize | jq -e '.result.protocolVersion == "2025-11-25"'
send_initialize() {
    local request='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'
    echo "$request" | inspector_server 2>/dev/null
}

@test "initialize request returns valid JSON-RPC response" {
    run send_initialize

    # Command should succeed
    assert_success

    # Response should contain jsonrpc version
    assert_output --partial '"jsonrpc":"2.0"'

    # Response should contain result field
    assert_output --partial '"result"'

    # Response should contain id
    assert_output --partial '"id":1'
}

@test "initialize returns correct protocol version" {
    run bash -c 'echo '\''{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'\'' | inspector_server 2>/dev/null | jq -e '\''.result.protocolVersion == "2025-11-25"'\'''

    # jq -e returns 0 if expression is true, 1 if false
    assert_success

    # Verify the actual value extracted
    run bash -c 'echo '\''{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'\'' | inspector_server 2>/dev/null | jq -r '\''.result.protocolVersion'\'
    assert_output "2025-11-25"
}

@test "initialize returns server capabilities" {
    local request='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'
    local response
    response=$(echo "$request" | inspector_server 2>/dev/null)

    # Check that capabilities object exists
    run bash -c "echo '$response' | jq -e '.result.capabilities'"
    assert_success

    # Check that tools capability exists
    run bash -c "echo '$response' | jq -e '.result.capabilities.tools'"
    assert_success

    # Check that resources capability exists
    run bash -c "echo '$response' | jq -e '.result.capabilities.resources'"
    assert_success

    # Check that prompts capability exists
    run bash -c "echo '$response' | jq -e '.result.capabilities.prompts'"
    assert_success
}

@test "initialize returns server info" {
    local request='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'
    local response
    response=$(echo "$request" | inspector_server 2>/dev/null)

    # Check that serverInfo object exists
    run bash -c "echo '$response' | jq -e '.result.serverInfo'"
    assert_success

    # Check that serverInfo.name exists and is not empty
    run bash -c "echo '$response' | jq -r '.result.serverInfo.name'"
    assert_success
    refute_output ""

    # Check that serverInfo.version exists and is not empty
    run bash -c "echo '$response' | jq -r '.result.serverInfo.version'"
    assert_success
    refute_output ""

    # Verify server name matches expected
    run bash -c "echo '$response' | jq -r '.result.serverInfo.name'"
    assert_output "mcpp Inspector Server"
}
