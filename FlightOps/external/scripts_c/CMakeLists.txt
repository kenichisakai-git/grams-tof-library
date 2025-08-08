project(Grams_Tof_Scripts_C)
message("${BMagenta}[============ Entering TOF Scripts C Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

# Make a lib for all the stuff
add_library(GramsTofScriptsCLib SHARED ${SOURCES})
target_include_directories(GramsTofScriptsCLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/FlightOps>
)
target_link_libraries(GramsTofScriptsCLib PUBLIC ${ROOT_LIBRARIES})

add_executable(process_threshold_calibration.exe tools/process_threshold_calibration.cpp)
target_link_libraries(process_threshold_calibration.exe PRIVATE GramsTofScriptsCLib)

install(TARGETS GramsTofScriptsCLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS process_threshold_calibration.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

