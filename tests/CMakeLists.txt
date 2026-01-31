# mcpp test suite

# Use FetchContent for reproducible GoogleTest builds
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.14.0
)
# For Windows: Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

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

target_link_libraries(mcpp_unit_tests
    PRIVATE
        mcpp
        GTest::gtest
        GTest::gtest_main
)

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

target_link_libraries(mcpp_integration_tests
    PRIVATE
        mcpp
        GTest::gtest
        GTest::gtest_main
)

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

target_link_libraries(mcpp_compliance_tests
    PRIVATE
        mcpp
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(mcpp_compliance_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(mcpp_compliance_tests PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

gtest_discover_tests(mcpp_compliance_tests
    LABELS compliance
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
