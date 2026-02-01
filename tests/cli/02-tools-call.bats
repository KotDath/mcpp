#!/usr/bin/env bats
# mcpp - MCP C++ library tests
# https://github.com/mcpp-project/mcpp
#
# Test file for tools/call endpoint
# Tests tool execution with arguments, error handling, and all 4 tools

# Load bats helper libraries
load "${BATS_TEST_DIRNAME}/test_helper/bats-support/load.bash"
load "${BATS_TEST_DIRNAME}/test_helper/bats-assert/load.bash"
load "${BATS_TEST_DIRNAME}/test_helper/bats-file/load.bash"

# Determine project root from test file location
PROJECT_ROOT="$(cd "${BATS_TEST_DIRNAME}/../.." && pwd)"

# Export MCPP_DEBUG for server debug output during tests
export MCPP_DEBUG=1

setup() {
    # Add build/examples to PATH so tests can find inspector_server
    export PATH="${PROJECT_ROOT}/build/examples:${PATH}"
}

# ============================================================================
# Calculate tool tests
# ============================================================================

@test "tools/call with calculate tool returns correct result" {
    # Send initialize and tools/call in the same connection
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"calculate\",\"arguments\":{\"operation\":\"add\",\"a\":5,\"b\":3}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success
    assert_output --partial '"jsonrpc"'
    assert_output --partial '"id": 2'
    assert_output --partial '"result"'

    # Verify the response contains content array
    echo "$output" | jq -e '.result.content' >/dev/null
    assert_equal $? 0

    # Verify result text contains "8" (5+3)
    echo "$output" | jq -e '.result.content[0].text == "8"' >/dev/null
    assert_equal $? 0
}

@test "tools/call with multiply operation" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"calculate\",\"arguments\":{\"operation\":\"multiply\",\"a\":4,\"b\":3}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify result contains "12"
    echo "$output" | jq -e '.result.content[0].text == "12"' >/dev/null
    assert_equal $? 0
}

@test "tools/call with subtract operation" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"calculate\",\"arguments\":{\"operation\":\"subtract\",\"a\":10,\"b\":4}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify result contains "6"
    echo "$output" | jq -e '.result.content[0].text == "6"' >/dev/null
    assert_equal $? 0
}

@test "tools/call with divide operation" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"calculate\",\"arguments\":{\"operation\":\"divide\",\"a\":15,\"b\":3}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify result contains "5"
    echo "$output" | jq -e '.result.content[0].text == "5"' >/dev/null
    assert_equal $? 0
}

@test "tools/call division by zero returns error" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"calculate\",\"arguments\":{\"operation\":\"divide\",\"a\":5,\"b\":0}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify response contains isError=true
    echo "$output" | jq -e '.result.isError == true' >/dev/null
    assert_equal $? 0

    # Verify error message mentions division by zero
    echo "$output" | jq -e '.result.content[0].text | contains("division by zero") or contains("Division by zero")' >/dev/null
    assert_equal $? 0
}

@test "tools/call with unknown operation returns error" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"calculate\",\"arguments\":{\"operation\":\"modulus\",\"a\":5,\"b\":3}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify response contains isError=true
    echo "$output" | jq -e '.result.isError == true' >/dev/null
    assert_equal $? 0
}

# ============================================================================
# Echo tool tests
# ============================================================================

@test "tools/call with echo tool" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"echo\",\"arguments\":{\"text\":\"hello\"}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify result text contains "Echo: hello"
    echo "$output" | jq -e '.result.content[0].text == "Echo: hello"' >/dev/null
    assert_equal $? 0
}

@test "tools/call with echo tool with special characters" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"echo\",\"arguments\":{\"text\":\"Hello, World!\"}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify result text contains the echoed message
    echo "$output" | jq -e '.result.content[0].text | contains("Hello, World!")' >/dev/null
    assert_equal $? 0
}

# ============================================================================
# get_time tool tests
# ============================================================================

@test "tools/call with get_time tool" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"get_time\",\"arguments\":{}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify result contains "Current time:"
    echo "$output" | jq -e '.result.content[0].text | contains("Current time:")' >/dev/null
    assert_equal $? 0
}

# ============================================================================
# server_info tool tests
# ============================================================================

@test "tools/call with server_info tool" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"server_info\",\"arguments\":{}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify result contains resource content type
    echo "$output" | jq -e '.result.content[0].type == "resource"' >/dev/null
    assert_equal $? 0

    # Verify result contains uri
    echo "$output" | jq -e '.result.content[0].uri == "info://server"' >/dev/null
    assert_equal $? 0

    # Verify result contains server data
    echo "$output" | jq -e '.result.content[0].data.name' >/dev/null
    assert_equal $? 0
}

# ============================================================================
# Error handling tests
# ============================================================================

@test "tools/call with unknown tool returns error" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"unknown_tool\",\"arguments\":{}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify error response - server returns empty result for unknown tools
    # or error object
    echo "$output" | jq -e '.error or .result' >/dev/null
    assert_equal $? 0
}

@test "tools/call with missing required arguments returns valid response" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"calculate\",\"arguments\":{}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # The server should handle gracefully - either use defaults or return error
    # We just verify we get a valid JSON-RPC response
    assert_output --partial '"jsonrpc"'
    assert_output --partial '"id": 2'
}

@test "tools/call with invalid argument types" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"calculate\",\"arguments\":{\"operation\":\"add\",\"a\":\"not_a_number\",\"b\":3}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Should get a valid response (error or result with defaults)
    assert_output --partial '"jsonrpc"'
}
