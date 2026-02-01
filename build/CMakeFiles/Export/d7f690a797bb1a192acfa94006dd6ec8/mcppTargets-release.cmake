#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mcpp::mcpp_static" for configuration "Release"
set_property(TARGET mcpp::mcpp_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(mcpp::mcpp_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libmcpp.a"
  )

list(APPEND _cmake_import_check_targets mcpp::mcpp_static )
list(APPEND _cmake_import_check_files_for_mcpp::mcpp_static "${_IMPORT_PREFIX}/lib/libmcpp.a" )

# Import target "mcpp::mcpp_shared" for configuration "Release"
set_property(TARGET mcpp::mcpp_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(mcpp::mcpp_shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libmcpp.so.0.1.0"
  IMPORTED_SONAME_RELEASE "libmcpp.so.0"
  )

list(APPEND _cmake_import_check_targets mcpp::mcpp_shared )
list(APPEND _cmake_import_check_files_for_mcpp::mcpp_shared "${_IMPORT_PREFIX}/lib/libmcpp.so.0.1.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
