project(Grams_Tof_Scripts_C)
message("${BMagenta}[============ Entering TOF Scripts C Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

find_package(Boost REQUIRED COMPONENTS random)

# Make a lib for all the stuff
add_library(GramsTofScriptsCLib SHARED ${SOURCES})
target_include_directories(GramsTofScriptsCLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/scripts_c>
)
target_link_libraries(GramsTofScriptsCLib PUBLIC ${ROOT_LIBRARIES})
target_link_libraries(GramsTofScriptsCLib PUBLIC Boost::random)
target_link_libraries(GramsTofScriptsCLib PUBLIC GramsTofRawDataLib)

add_executable(process_threshold_calibration tools/process_threshold_calibration.cpp)
target_link_libraries(process_threshold_calibration PRIVATE GramsTofScriptsCLib cli11)

add_executable(process_qdc_calibration tools/process_qdc_calibration.cpp)
target_link_libraries(process_qdc_calibration PRIVATE GramsTofScriptsCLib)

add_executable(process_tdc_calibration tools/process_tdc_calibration.cpp)
target_link_libraries(process_tdc_calibration PRIVATE GramsTofScriptsCLib)

add_executable(convert_raw_to_singles tools/convert_raw_to_singles.cpp)
target_link_libraries(convert_raw_to_singles PRIVATE GramsTofScriptsCLib)

add_executable(convert_raw_to_raw tools/convert_raw_to_raw.cpp)
target_link_libraries(convert_raw_to_raw PRIVATE GramsTofScriptsCLib)

install(TARGETS GramsTofScriptsCLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/scripts_c)

install(TARGETS process_threshold_calibration
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(TARGETS process_qdc_calibration
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(TARGETS process_tdc_calibration
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(TARGETS convert_raw_to_singles
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(TARGETS convert_raw_to_raw
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

