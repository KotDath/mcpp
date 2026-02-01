#!/usr/bin/env bats
# mcpp - MCP C++ library tests
# https://github.com/mcpp-project/mcpp
#
# Test file for resources/list and resources/read endpoints
# Tests resource listing and reading functionality

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
# resources/list tests
# ============================================================================

@test "resources/list returns registered resources" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/list\"}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success
    assert_output --partial '"result"'

    # Verify resources array exists
    echo "$output" | jq -e '.result.resources' >/dev/null
    assert_equal $? 0

    # Verify "file://tmp/mcpp_test.txt" exists
    echo "$output" | jq -e '.result.resources[] | select(.uri == "file://tmp/mcpp_test.txt")' >/dev/null
    assert_equal $? 0

    # Verify "info://server" exists
    echo "$output" | jq -e '.result.resources[] | select(.uri == "info://server")' >/dev/null
    assert_equal $? 0
}

@test "resources/list resources have metadata" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/list\"}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify resources have name field
    echo "$output" | jq -e '.result.resources[0].name' >/dev/null
    assert_equal $? 0

    # Verify resources have description field
    echo "$output" | jq -e '.result.resources[] | select(.description != null and .description != "")' >/dev/null
    assert_equal $? 0

    # Verify MIME types are correct
    echo "$output" | jq -e '.result.resources[] | select(.mimeType == "text/plain" or .mimeType == "application/json")' >/dev/null
    assert_equal $? 0
}

@test "resources/list returns exactly 2 resources" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/list\"}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Count the resources
    local count
    count=$(echo "$output" | jq -e '.result.resources | length')
    assert_equal "$count" "2"
}

# ============================================================================
# resources/read tests
# ============================================================================

@test "resources/read for server info resource" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/read\",\"params\":{\"uri\":\"info://server\"}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success
    assert_output --partial '"result"'

    # Verify content is returned
    echo "$output" | jq -e '.result.contents' >/dev/null
    assert_equal $? 0

    # Verify content array has at least one item
    echo "$output" | jq -e '.result.contents | length > 0' >/dev/null
    assert_equal $? 0

    # Verify URI matches
    echo "$output" | jq -e '.result.contents[0].uri == "info://server"' >/dev/null
    assert_equal $? 0

    # Verify MIME type is application/json
    echo "$output" | jq -e '.result.contents[0].mimeType == "application/json"' >/dev/null
    assert_equal $? 0
}

@test "resources/read for server info contains server information" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/read\",\"params\":{\"uri\":\"info://server\"}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify text content contains server name
    echo "$output" | jq -e '.result.contents[0].text | contains("mcpp")' >/dev/null
    assert_equal $? 0

    # Verify text content contains version
    echo "$output" | jq -e '.result.contents[0].text | contains("version")' >/dev/null
    assert_equal $? 0

    # Verify text is valid JSON
    local text
    text=$(echo "$output" | jq -r '.result.contents[0].text')
    echo "$text" | jq -e '.' >/dev/null
    assert_equal $? 0
}

@test "resources/read for non-existent resource returns error" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/read\",\"params\":{\"uri\":\"file://nonexistent/file.txt\"}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Should get an error response for non-existent resource
    assert_output --partial '"error"'
}

@test "resources/read for test file resource" {
    # Create a test file in /tmp
    echo "test content" > /tmp/mcpp_test.txt

    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/read\",\"params\":{\"uri\":\"file://tmp/mcpp_test.txt\"}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Verify content is returned
    echo "$output" | jq -e '.result.contents' >/dev/null
    assert_equal $? 0

    # Verify URI matches
    echo "$output" | jq -e '.result.contents[0].uri == "file://tmp/mcpp_test.txt"' >/dev/null
    assert_equal $? 0

    # Clean up test file
    rm -f /tmp/mcpp_test.txt
}

@test "resources/read with invalid URI format" {
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/read\",\"params\":{\"uri\":\"invalid-uri-format\"}}"
    } | inspector_server 2>/dev/null | tail -1 | jq'

    assert_success

    # Should get a valid response (error or empty result)
    assert_output --partial '"jsonrpc"'
}

@test "resources/list and resources/read work in sequence" {
    # First list resources, then read one
    run bash -c '{
        echo "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},\"clientInfo\":{\"name\":\"test\",\"version\":\"1.0\"}}}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/list\"}"
        echo "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"resources/read\",\"params\":{\"uri\":\"info://server\"}}"
    } | inspector_server 2>/dev/null | jq -s'

    assert_success

    # Verify we got 3 responses
    local count
    count=$(echo "$output" | jq -e 'length')
    assert_equal "$count" "3"

    # Verify second response is resources/list
    echo "$output" | jq -e '.[1].result.resources' >/dev/null
    assert_equal $? 0

    # Verify third response is resources/read
    echo "$output" | jq -e '.[2].result.contents' >/dev/null
    assert_equal $? 0
}
