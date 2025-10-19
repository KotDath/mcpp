# Examples CMakeLists.txt

# Echo server example
add_executable(echo_server
    echo_server.cpp
)

target_link_libraries(echo_server PRIVATE mcpp)

# Stdio client example
add_executable(stdio_client
    stdio_client.cpp
)

target_link_libraries(stdio_client PRIVATE mcpp)

# Set output directory
set_target_properties(echo_server stdio_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)