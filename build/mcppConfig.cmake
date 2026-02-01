# mcpp CMake package configuration
# https://github.com/mcpp-project/mcpp
#
# Copyright (c) 2025 mcpp contributors
# Distributed under MIT License


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was mcppConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

# Find nlohmann_json dependency
# Assume it's installed or user provides it via CMAKE_PREFIX_PATH
find_dependency(nlohmann_json 3.11.0 REQUIRED)

# Include the targets file
include("${CMAKE_CURRENT_LIST_DIR}/mcppTargets.cmake")

# Create the mcpp::mcpp alias that points to the correct library type
# Users can link against either mcpp::mcpp_static or mcpp::mcpp_shared
if(NOT TARGET mcpp::mcpp)
    if(TARGET mcpp::mcpp_shared)
        add_library(mcpp::mcpp ALIAS mcpp::mcpp_shared)
    elseif(TARGET mcpp::mcpp_static)
        add_library(mcpp::mcpp ALIAS mcpp::mcpp_static)
    endif()
endif()

check_required_components(mcpp)
