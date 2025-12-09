project(Grams_Tof_GroundOps)

file(GLOB HEADERS *.hh *.h)
 
add_library(GramsTofGroundOpsLib INTERFACE)
target_include_directories(GramsTofGroundOpsLib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
    $<INSTALL_INTERFACE:include/shota>
)

install(TARGETS GramsTofGroundOpsLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/shota)

