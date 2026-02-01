# Install script for directory: /home/kotdath/omp/personal/cpp/mcpp

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/kotdath/omp/personal/cpp/mcpp/build/libmcpp.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmcpp.so.0.1.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmcpp.so.0"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/kotdath/omp/personal/cpp/mcpp/build/libmcpp.so.0.1.0"
    "/home/kotdath/omp/personal/cpp/mcpp/build/libmcpp.so.0"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmcpp.so.0.1.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmcpp.so.0"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kotdath/omp/personal/cpp/mcpp/build/libmcpp.so")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mcpp" TYPE FILE FILES
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/api/context.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/api/peer.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/api/role.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/api/running_service.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/api/service.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/async/callbacks.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/async/timeout.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/client.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/client/cancellation.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/client/elicitation.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/client/future_wrapper.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/client/roots.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/client/sampling.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/client_blocking.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/core/error.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/core/json_rpc.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/core/request_tracker.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/content/content.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/content/pagination.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/protocol/capabilities.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/protocol/initialize.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/protocol/types.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/server/mcp_server.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/server/prompt_registry.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/server/request_context.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/server/resource_registry.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/server/task_manager.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/server/tool_registry.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/transport/stdio_transport.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/transport/transport.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/util/atomic_id.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/util/error.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/util/logger.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/util/pagination.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/util/retry.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/util/sse_formatter.h"
    "/home/kotdath/omp/personal/cpp/mcpp/src/mcpp/util/uri_template.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/mcpp/mcppTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/mcpp/mcppTargets.cmake"
         "/home/kotdath/omp/personal/cpp/mcpp/build/CMakeFiles/Export/d7f690a797bb1a192acfa94006dd6ec8/mcppTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/mcpp/mcppTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/mcpp/mcppTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/mcpp" TYPE FILE FILES "/home/kotdath/omp/personal/cpp/mcpp/build/CMakeFiles/Export/d7f690a797bb1a192acfa94006dd6ec8/mcppTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/mcpp" TYPE FILE FILES "/home/kotdath/omp/personal/cpp/mcpp/build/CMakeFiles/Export/d7f690a797bb1a192acfa94006dd6ec8/mcppTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/mcpp" TYPE FILE FILES
    "/home/kotdath/omp/personal/cpp/mcpp/build/mcppConfig.cmake"
    "/home/kotdath/omp/personal/cpp/mcpp/build/mcppConfigVersion.cmake"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/kotdath/omp/personal/cpp/mcpp/build/tests/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/kotdath/omp/personal/cpp/mcpp/build/examples/cmake_install.cmake")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/kotdath/omp/personal/cpp/mcpp/build/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/kotdath/omp/personal/cpp/mcpp/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
