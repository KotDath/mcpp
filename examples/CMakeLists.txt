# mcpp examples

# Inspector server example - MCP server for testing with MCP Inspector
add_executable(inspector_server
    inspector_server.cpp
)

# Link against the appropriate library based on BUILD_SHARED_LIBS
if(BUILD_SHARED_LIBS)
    target_link_libraries(inspector_server
        PRIVATE mcpp_shared
    )
else()
    target_link_libraries(inspector_server
        PRIVATE mcpp_static
    )
endif()

target_include_directories(inspector_server
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)

# HTTP server integration example - demonstrates HttpTransport adapter pattern
add_executable(http_server_integration
    http_server_integration.cpp
)

if(BUILD_SHARED_LIBS)
    target_link_libraries(http_server_integration
        PRIVATE mcpp_shared
    )
else()
    target_link_libraries(http_server_integration
        PRIVATE mcpp_static
    )
endif()

target_include_directories(http_server_integration
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)
