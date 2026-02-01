# MCP Tools/List Endpoint Tests
# Tests for the tools/list endpoint that discovers available tools

# Load common setup for test environment
load 'test_helper/common-setup'

# Setup function - runs before each test
setup() {
    _common_setup
}

# Helper function to send both initialize and tools/list requests
# MCP protocol requires initialize before tools/list
# Usage: send_tools_list
send_tools_list() {
    local initialize='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'
    local tools_list='{"jsonrpc":"2.0","id":2,"method":"tools/list"}'
    {
        echo "$initialize"
        echo "$tools_list"
    } | inspector_server 2>/dev/null | tail -n 1  # Get only the second response (tools/list)
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

@test "tools/list returns expected tools" {
    local response
    response=$(send_tools_list)

    # Check that the calculate tool exists
    run bash -c "echo '$response' | jq -e '.result.tools[] | select(.name == \"calculate\")'"
    assert_success

    # Check that the echo tool exists
    run bash -c "echo '$response' | jq -e '.result.tools[] | select(.name == \"echo\")'"
    assert_success

    # Check that the get_time tool exists
    run bash -c "echo '$response' | jq -e '.result.tools[] | select(.name == \"get_time\")'"
    assert_success

    # Check that the server_info tool exists
    run bash -c "echo '$response' | jq -e '.result.tools[] | select(.name == \"server_info\")'"
    assert_success
}

@test "tools/list tools have schema" {
    local response
    response=$(send_tools_list)

    # Check that all tools have inputSchema
    run bash -c "echo '$response' | jq -e '.result.tools[].inputSchema'"
    assert_success

    # Verify calculate tool has operation property in schema
    run bash -c "echo '$response' | jq -e '.result.tools[] | select(.name == \"calculate\") | .inputSchema.properties.operation'"
    assert_success

    # Verify calculate tool has a property in schema
    run bash -c "echo '$response' | jq -e '.result.tools[] | select(.name == \"calculate\") | .inputSchema.properties.a'"
    assert_success

    # Verify calculate tool has b property in schema
    run bash -c "echo '$response' | jq -e '.result.tools[] | select(.name == \"calculate\") | .inputSchema.properties.b'"
    assert_success
}

@test "tools/list before initialize fails gracefully" {
    # Send tools/list WITHOUT initialize first
    local tools_list='{"jsonrpc":"2.0","id":1,"method":"tools/list"}'

    # Server should respond (either with error or empty result)
    # The key is that it doesn't crash
    run bash -c "echo '$tools_list' | inspector_server 2>/dev/null"

    # Should get some response (exit code 0)
    assert_success

    # Response should be valid JSON (contains jsonrpc field)
    assert_output --partial '"jsonrpc"'
}
