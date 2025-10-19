cmake_minimum_required(VERSION 3.19)
project(mcpp VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Compiler options
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    set_property(TARGET mcpp PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Options
option(MCPP_BUILD_TESTS "Build tests" OFF)
option(MCPP_BUILD_EXAMPLES "Build examples" ON)
option(MCPP_ENABLE_SSL "Enable SSL/TLS support" OFF)

# Find required packages
find_package(Threads REQUIRED)

# Add thirdparty subdirectories
add_subdirectory(thirdparty/nlohmann_json)
add_subdirectory(thirdparty/spdlog)

# cpp-httplib is header-only, just include it
# spdlog can be compiled, but we'll use it as header-only for simplicity

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/cpp_httplib)

# Core library
add_library(mcpp STATIC
    src/core/json_rpc_message.cpp
    src/core/protocol_version.cpp
    src/transport/stdio_transport.cpp
    src/model/request.cpp
    src/model/response.cpp
    src/model/notification.cpp
    src/utils/error.cpp
    src/utils/logger.cpp
)

target_include_directories(mcpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(mcpp
    PUBLIC
        nlohmann_json::nlohmann_json
        spdlog::spdlog_header_only
    PRIVATE
        Threads::Threads
)

# SSL support (optional)
if(MCPP_ENABLE_SSL)
    find_package(OpenSSL 3.0.0 REQUIRED)
    target_compile_definitions(mcpp PRIVATE MCPP_ENABLE_SSL CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(mcpp PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "OpenSSL found: ${OPENSSL_INCLUDE_DIR}")
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(mcpp PRIVATE ws2_32 crypt32)
endif()

# Examples
if(MCPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(MCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation (simplified for Phase 1)
# TODO: Add proper installation with dependency management in later phases
# include(GNUInstallDirs)
#
# install(TARGETS mcpp
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )
#
# install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Print configuration summary
message(STATUS "")
message(STATUS "=== mcpp Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build tests: ${MCPP_BUILD_TESTS}")
message(STATUS "Build examples: ${MCPP_BUILD_EXAMPLES}")
message(STATUS "SSL support: ${MCPP_ENABLE_SSL}")
if(MCPP_ENABLE_SSL)
    message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
endif()
message(STATUS "===========================")