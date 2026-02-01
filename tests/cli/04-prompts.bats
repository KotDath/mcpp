#!/usr/bin/env bats
# mcpp - MCP C++ library tests
# https://github.com/mcpp-project/mcpp
#
# Test file for prompts/list and prompts/get endpoints
# Tests prompt listing and retrieval with arguments

# Load common setup for test environment
load 'test_helper/common-setup'

# Setup function - runs before each test
setup() {
    _common_setup
}

# ============================================================================
# prompts/list tests
# ============================================================================

@test "prompts/list returns registered prompts" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"prompts/list\"}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success
    assert_output --partial '"result"'

    # Verify prompts array exists
    echo "$output" | jq -e '.result.prompts' >/dev/null
    assert_equal $? 0

    # Verify "code_review" prompt exists
    echo "$output" | jq -e '.result.prompts[] | select(.name == "code_review")' >/dev/null
    assert_equal $? 0
}

@test "prompts/list prompts have arguments" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"prompts/list\"}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify code_review has arguments
    echo "$output" | jq -e '.result.prompts[] | select(.name == "code_review") | .arguments' >/dev/null
    assert_equal $? 0

    # Verify code_review has language argument
    echo "$output" | jq -e '.result.prompts[] | select(.name == "code_review") | .arguments[] | select(.name == "language")' >/dev/null
    assert_equal $? 0

    # Verify code_review has focus argument
    echo "$output" | jq -e '.result.prompts[] | select(.name == "code_review") | .arguments[] | select(.name == "focus")' >/dev/null
    assert_equal $? 0
}

@test "prompts/list prompts have metadata" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"prompts/list\"}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify prompts have description
    echo "$output" | jq -e '.result.prompts[] | select(.description != null and .description != "")' >/dev/null
    assert_equal $? 0

    # Verify arguments have descriptions
    echo "$output" | jq -e '.result.prompts[].arguments[] | select(.description != null and .description != "")' >/dev/null
    assert_equal $? 0
}

@test "prompts/list returns exactly 1 prompt" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"prompts/list\"}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Count the prompts
    local count
    count=$(echo "$output" | jq -e '.result.prompts | length')
    assert_equal "$count" "1"
}

# ============================================================================
# prompts/get tests
# ============================================================================

@test "prompts/get for code_review prompt with arguments" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"prompts/get\",\"params\":{\"name\":\"code_review\",\"arguments\":{\"language\":\"C++\",\"focus\":\"performance\"}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify messages array is returned
    echo "$output" | jq -e '.result.messages' >/dev/null
    assert_equal $? 0

    # Verify prompt template contains requested language (C++)
    echo "$output" | jq -e '.result.messages[0].content[0].text | contains("C++")' >/dev/null
    assert_equal $? 0

    # Verify prompt template contains requested focus (performance)
    echo "$output" | jq -e '.result.messages[0].content[0].text | contains("performance")' >/dev/null
    assert_equal $? 0

    # Verify prompt contains "code review" or "review"
    echo "$output" | jq -e '.result.messages[0].content[0].text | test("review"; "i")' >/dev/null
    assert_equal $? 0
}

@test "prompts/get for code_review prompt with different language" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"prompts/get\",\"params\":{\"name\":\"code_review\",\"arguments\":{\"language\":\"Python\",\"focus\":\"security\"}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify prompt template contains Python
    echo "$output" | jq -e '.result.messages[0].content[0].text | contains("Python")' >/dev/null
    assert_equal $? 0

    # Verify prompt template contains security focus
    echo "$output" | jq -e '.result.messages[0].content[0].text | contains("security")' >/dev/null
    assert_equal $? 0
}

@test "prompts/get with unknown prompt name returns error" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"prompts/get\",\"params\":{\"name\":\"greeting\",\"arguments\":{}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Should get an error response for unregistered prompt
    assert_output --partial '"error"'
}

@test "prompts/get with truly unknown prompt name returns error" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"prompts/get\",\"params\":{\"name\":\"unknown_prompt\",\"arguments\":{}}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Should get an error response
    assert_output --partial '"error"'
}

@test "prompts/get with missing arguments" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"prompts/get\",\"params\":{\"name\":\"code_review\"}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Should get a valid response with default values
    assert_output --partial '"result"'
}

@test "prompts/list and prompts/get work in sequence" {
    # First list prompts, then get one
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"prompts/list\"}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"prompts/get\",\"params\":{\"name\":\"code_review\",\"arguments\":{\"language\":\"Rust\",\"focus\":\"correctness\"}}}"
    } | inspector_server 2>/dev/null | jq -s'

    assert_success

    # Verify we got 3 responses
    local count
    count=$(echo "$output" | jq -e 'length')
    assert_equal "$count" "3"

    # Verify second response is prompts/list
    echo "$output" | jq -e '.[1].result.prompts' >/dev/null
    assert_equal $? 0

    # Verify third response is prompts/get
    echo "$output" | jq -e '.[2].result.messages' >/dev/null
    assert_equal $? 0
}
