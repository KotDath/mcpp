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
find_package(nlohmann_json 3.11.0 REQUIRED)

# Source files
set(MCPP_PUBLIC_HEADERS
    src/mcpp/client.h
    src/mcpp/async/callbacks.h
    src/mcpp/async/timeout.h
    src/mcpp/client/cancellation.h
    src/mcpp/client/roots.h
    src/mcpp/client/sampling.h
    src/mcpp/core/error.h
    src/mcpp/core/json_rpc.h
    src/mcpp/core/request_tracker.h
    src/mcpp/protocol/capabilities.h
    src/mcpp/protocol/initialize.h
    src/mcpp/protocol/types.h
    src/mcpp/transport/transport.h
    src/mcpp/transport/stdio_transport.h
    src/mcpp/server/tool_registry.h
    src/mcpp/server/resource_registry.h
    src/mcpp/server/prompt_registry.h
    src/mcpp/server/request_context.h
    src/mcpp/server/mcp_server.h
)

set(MCPP_SOURCES
    src/mcpp/client.cpp
    src/mcpp/async/timeout.cpp
    src/mcpp/client/cancellation.cpp
    src/mcpp/client/roots.cpp
    src/mcpp/client/sampling.cpp
    src/mcpp/core/json_rpc.cpp
    src/mcpp/core/request_tracker.cpp
    src/mcpp/transport/stdio_transport.cpp
    src/mcpp/server/tool_registry.cpp
    src/mcpp/server/resource_registry.cpp
    src/mcpp/server/prompt_registry.cpp
    src/mcpp/server/request_context.cpp
    src/mcpp/server/mcp_server.cpp
)

# Create the mcpp library
add_library(mcpp ${MCPP_SOURCES})
target_include_directories(mcpp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(mcpp PUBLIC nlohmann_json::nlohmann_json)

# Set library properties
set_target_properties(mcpp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${MCPP_PUBLIC_HEADERS}"
)

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mcpp PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS mcpp
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mcpp
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
