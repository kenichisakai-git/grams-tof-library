#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Grams::GramsTofFlightOpsLib" for configuration "RelWithDebInfo"
set_property(TARGET Grams::GramsTofFlightOpsLib APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Grams::GramsTofFlightOpsLib PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libGramsTofFlightOpsLib.so"
  IMPORTED_SONAME_RELWITHDEBINFO "libGramsTofFlightOpsLib.so"
  )

list(APPEND _cmake_import_check_targets Grams::GramsTofFlightOpsLib )
list(APPEND _cmake_import_check_files_for_Grams::GramsTofFlightOpsLib "${_IMPORT_PREFIX}/lib/libGramsTofFlightOpsLib.so" )

# Import target "Grams::GramsTofDaqdLib" for configuration "RelWithDebInfo"
set_property(TARGET Grams::GramsTofDaqdLib APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Grams::GramsTofDaqdLib PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libGramsTofDaqdLib.so"
  IMPORTED_SONAME_RELWITHDEBINFO "libGramsTofDaqdLib.so"
  )

list(APPEND _cmake_import_check_targets Grams::GramsTofDaqdLib )
list(APPEND _cmake_import_check_files_for_Grams::GramsTofDaqdLib "${_IMPORT_PREFIX}/lib/libGramsTofDaqdLib.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
