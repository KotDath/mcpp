# mcpp test suite

# Use FetchContent for reproducible GoogleTest builds
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Include GoogleTest module for gtest_discover_tests
include(GoogleTest)

# Helper to link against the appropriate library based on BUILD_SHARED_LIBS
macro(link_mcpp_target target)
    if(BUILD_SHARED_LIBS)
        target_link_libraries(${target}
            PRIVATE
                mcpp_shared
                GTest::gtest
                GTest::gtest_main
        )
    else()
        target_link_libraries(${target}
            PRIVATE
                mcpp_static
                GTest::gtest
                GTest::gtest_main
        )
    endif()
endmacro()

# Unit tests
add_executable(mcpp_unit_tests
    unit/test_json_rpc.cpp
    unit/test_request_tracker.cpp
    unit/test_timeout_manager.cpp
    unit/test_tool_registry.cpp
    unit/test_resource_registry.cpp
    unit/test_prompt_registry.cpp
    unit/test_pagination.cpp
)

link_mcpp_target(mcpp_unit_tests)

target_include_directories(mcpp_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

gtest_discover_tests(mcpp_unit_tests
    LABELS unit
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Integration tests (empty for now, will be populated in 07-04)
add_executable(mcpp_integration_tests
    integration/test_client_server.cpp
)

link_mcpp_target(mcpp_integration_tests)

target_include_directories(mcpp_integration_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

gtest_discover_tests(mcpp_integration_tests
    LABELS integration
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Compliance tests (empty for now, will be populated in 07-03)
add_executable(mcpp_compliance_tests
    compliance/test_jsonrpc_spec.cpp
)

link_mcpp_target(mcpp_compliance_tests)

target_include_directories(mcpp_compliance_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(mcpp_compliance_tests PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_BINARY_DIR}/data"
)

# Ensure test data is available in build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/jsonrpc_examples.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data
)

gtest_discover_tests(mcpp_compliance_tests
    LABELS compliance
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ============================================================================
# CLI Tests (BATS)
# ============================================================================

# Note: bats-core is installed as a Git submodule in tests/cli/bats
# Run: git submodule update --init --recursive
# If bats is not found, CLI tests will be skipped (not fatal)

# Find bats and jq for CLI tests
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

    # Set test properties for CTest
    # RUN_SERIAL: Prevents parallel execution to avoid port conflicts
    # ENVIRONMENT PATH: Ensures inspector_server is found in build/examples
    set_tests_properties(cli-tests PROPERTIES
        LABELS "cli;bats"
        RUN_SERIAL TRUE
        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/examples:$ENV{PATH}"
    )

    # Optional: Add individual test files as separate tests for granularity
    # This allows running specific test suites via ctest -R cli-<name>
    add_test(
        NAME cli-initialize
        COMMAND ${BATS_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/cli/05-initialize.bats
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(cli-initialize PROPERTIES
        LABELS "cli;bats;initialize"
        RUN_SERIAL TRUE
        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/examples:$ENV{PATH}"
    )

    add_test(
        NAME cli-tools-list
        COMMAND ${BATS_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/cli/01-tools-list.bats
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(cli-tools-list PROPERTIES
        LABELS "cli;bats;tools"
        RUN_SERIAL TRUE
        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/examples:$ENV{PATH}"
    )

    add_test(
        NAME cli-tools-call
        COMMAND ${BATS_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/cli/02-tools-call.bats
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(cli-tools-call PROPERTIES
        LABELS "cli;bats;tools"
        RUN_SERIAL TRUE
        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/examples:$ENV{PATH}"
    )

    add_test(
        NAME cli-resources
        COMMAND ${BATS_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/cli/03-resources.bats
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(cli-resources PROPERTIES
        LABELS "cli;bats;resources"
        RUN_SERIAL TRUE
        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/examples:$ENV{PATH}"
    )

    add_test(
        NAME cli-prompts
        COMMAND ${BATS_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/cli/04-prompts.bats
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(cli-prompts PROPERTIES
        LABELS "cli;bats;prompts"
        RUN_SERIAL TRUE
        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/examples:$ENV{PATH}"
    )

    add_test(
        NAME cli-lifecycle
        COMMAND ${BATS_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/cli/06-lifecycle.bats
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(cli-lifecycle PROPERTIES
        LABELS "cli;bats;lifecycle"
        RUN_SERIAL TRUE
        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/examples:$ENV{PATH}"
    )
endif()
