project(Grams_Tof_GroundOps)
 
add_library(GramsTofGroundOpsLib INTERFACE)
target_include_directories(GramsTofGroundOpsLib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
    $<INSTALL_INTERFACE:include/FlightOps>
)

install(TARGETS GramsTofGroundOpsLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

