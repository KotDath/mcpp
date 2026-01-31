# mcpp - MCP C++ library
# https://github.com/mcpp-project/mcpp
#
# Copyright (c) 2025 mcpp contributors
# Distributed under MIT License

cmake_minimum_required(VERSION 3.16)

project(mcpp VERSION 0.1.0 LANGUAGES CXX)

# C++20 is required for std::stop_token (ASYNC-02 cancellation support)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(MCPP_BUILD_TESTS "Build mcpp tests" ON)
option(MCPP_BUILD_EXAMPLES "Build mcpp examples" ON)

# Find dependencies
# Use local copy of nlohmann_json header-only library
set(nlohmann_json_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nlohmann_json/include")
if(NOT EXISTS "${nlohmann_json_INCLUDE_DIR}/nlohmann/json.hpp")
    message(FATAL_ERROR "nlohmann/json.hpp not found. Please run: cp thirdparty/gopher-mcp/build/_deps/nlohmann_json-src/include/nlohmann/json.hpp thirdparty/nlohmann_json/include/nlohmann/")
endif()
add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_INCLUDE_DIR}"
)

# Source files
set(MCPP_PUBLIC_HEADERS
    # API headers (Phase 6: High-Level API)
    src/mcpp/api/context.h
    src/mcpp/api/peer.h
    src/mcpp/api/role.h
    src/mcpp/api/running_service.h
    src/mcpp/api/service.h
    # Async headers
    src/mcpp/async/callbacks.h
    src/mcpp/async/timeout.h
    # Client headers
    src/mcpp/client.h
    src/mcpp/client/cancellation.h
    src/mcpp/client/elicitation.h
    src/mcpp/client/future_wrapper.h
    src/mcpp/client/roots.h
    src/mcpp/client/sampling.h
    src/mcpp/client_blocking.h
    # Core headers
    src/mcpp/core/error.h
    src/mcpp/core/json_rpc.h
    src/mcpp/core/request_tracker.h
    # Content headers
    src/mcpp/content/content.h
    src/mcpp/content/pagination.h
    # Protocol headers
    src/mcpp/protocol/capabilities.h
    src/mcpp/protocol/initialize.h
    src/mcpp/protocol/types.h
    # Server headers
    src/mcpp/server/mcp_server.h
    src/mcpp/server/prompt_registry.h
    src/mcpp/server/request_context.h
    src/mcpp/server/resource_registry.h
    src/mcpp/server/task_manager.h
    src/mcpp/server/tool_registry.h
    # Transport headers
    src/mcpp/transport/stdio_transport.h
    src/mcpp/transport/transport.h
    # Util headers (Phase 4: HTTP Transport, Phase 6: High-Level API)
    src/mcpp/util/atomic_id.h
    src/mcpp/util/error.h
    src/mcpp/util/logger.h
    src/mcpp/util/pagination.h
    src/mcpp/util/retry.h
    src/mcpp/util/sse_formatter.h
    src/mcpp/util/uri_template.h
)

set(MCPP_SOURCES
    src/mcpp/client.cpp
    src/mcpp/async/timeout.cpp
    src/mcpp/client/cancellation.cpp
    src/mcpp/client/elicitation.cpp
    src/mcpp/client/future_wrapper.cpp
    src/mcpp/client/roots.cpp
    src/mcpp/client/sampling.cpp
    src/mcpp/core/json_rpc.cpp
    src/mcpp/core/request_tracker.cpp
    src/mcpp/transport/stdio_transport.cpp
    src/mcpp/transport/http_transport.cpp
    src/mcpp/server/mcp_server.cpp
    src/mcpp/server/prompt_registry.cpp
    src/mcpp/server/request_context.cpp
    src/mcpp/server/resource_registry.cpp
    src/mcpp/server/task_manager.cpp
    src/mcpp/server/tool_registry.cpp
    # Util sources
    src/mcpp/util/error.cpp
    src/mcpp/util/logger.cpp
)

# Build both static and shared libraries
add_library(mcpp_static STATIC ${MCPP_SOURCES})
add_library(mcpp_shared SHARED ${MCPP_SOURCES})

# Common include directories for both libraries
target_include_directories(mcpp_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(mcpp_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Link nlohmann_json to both libraries
target_link_libraries(mcpp_static PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(mcpp_shared PUBLIC nlohmann_json::nlohmann_json)

# Set library properties
set_target_properties(mcpp_static PROPERTIES
    VERSION ${PROJECT_VERSION}
    OUTPUT_NAME mcpp
    ARCHIVE_OUTPUT_NAME mcpp
)

set_target_properties(mcpp_shared PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME mcpp
    LIBRARY_OUTPUT_NAME mcpp
    PUBLIC_HEADER "${MCPP_PUBLIC_HEADERS}"
)

# Apply warning flags to both libraries
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mcpp_static PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter
    )
    target_compile_options(mcpp_shared PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter
    )
endif()

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Generate version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/mcppConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Configure package config
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/mcppConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/mcppConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mcpp
)

# Install both static and shared libraries
install(TARGETS mcpp_static mcpp_shared
    EXPORT mcppTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mcpp
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install exported targets
install(EXPORT mcppTargets
    FILE mcppTargets.cmake
    NAMESPACE mcpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mcpp
)

# Install package config files
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/mcppConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/mcppConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mcpp
)

# Tests
if(MCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(MCPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
