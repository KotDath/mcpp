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
