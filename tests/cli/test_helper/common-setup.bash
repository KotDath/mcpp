# Common setup function for all BATS test files
# This file provides consistent test environment configuration

# Load bats-support helper library
# The bats helper libraries are in test_helper/ subdirectory
load "${BATS_TEST_DIRNAME}/test_helper/bats-support/load.bash"

# Load bats-assert helper library
load "${BATS_TEST_DIRNAME}/test_helper/bats-assert/load.bash"

# Load bats-file helper library for filesystem assertions
load "${BATS_TEST_DIRNAME}/test_helper/bats-file/load.bash"

# Determine project root from test file location
# BATS_TEST_DIRNAME is the directory containing the test files (tests/cli/)
# We navigate up two levels to get to the project root (mcpp directory)
PROJECT_ROOT="$(cd "${BATS_TEST_DIRNAME}/../.." && pwd)"

# Export MCPP_DEBUG for server debug output during tests
export MCPP_DEBUG=1

# Common setup function to be called in each test's setup()
_common_setup() {
    # Use bats-file's temp directory for test-specific temporary files
    # BATS_TEST_TMPDIR is automatically created and cleaned up by bats
    local tmpdir="${BATS_TEST_TMPDIR}"

    # Add build/examples to PATH so tests can find inspector_server
    # This allows tests to run inspector_server without absolute paths
    export PATH="${PROJECT_ROOT}/build/examples:${PATH}"

    # Export project root for tests that need to reference project files
    export PROJECT_ROOT

    # Export tmpdir for tests that need temporary files
    export TEST_TMPDIR="${tmpdir}"
}

# Helper function to get the path to inspector_server
# Returns: absolute path to inspector_server binary
inspector_server_path() {
    echo "${PROJECT_ROOT}/build/examples/inspector_server"
}

# Helper function to check if inspector_server is built
# Returns: 0 if built, 1 if not
inspector_server_exists() {
    [[ -f "$(inspector_server_path)" ]]
}

# Helper function to wait for a file to appear
# Usage: wait_for_file <path> <max_seconds>
wait_for_file() {
    local file="$1"
    local max_seconds="${2:-5}"
    local count=0

    while [[ ! -f "${file}" ]] && [[ ${count} -lt ${max_seconds} ]]; do
        sleep 0.1
        count=$((count + 1))
    done

    [[ -f "${file}" ]]
}

# Helper function to wait for a string in a file
# Usage: wait_for_content <file> <string> <max_seconds>
wait_for_content() {
    local file="$1"
    local string="$2"
    local max_seconds="${3:-5}"
    local count=0

    while ! grep -q "${string}" "${file}" 2>/dev/null && [[ ${count} -lt ${max_seconds} ]]; do
        sleep 0.1
        count=$((count + 1))
    done

    grep -q "${string}" "${file}" 2>/dev/null
}
