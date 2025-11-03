project(Grams_Tof_Scripts_C)
message("${BMagenta}[============ Entering TOF Scripts C Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

find_package(Boost REQUIRED COMPONENTS random)

# Make a lib for all the stuff
add_library(GramsTofScriptsCLib SHARED ${SOURCES})
target_include_directories(GramsTofScriptsCLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/FlightOps>
)
target_link_libraries(GramsTofScriptsCLib PUBLIC ${ROOT_LIBRARIES})
target_link_libraries(GramsTofScriptsCLib PUBLIC Boost::random)
target_link_libraries(GramsTofScriptsCLib PUBLIC GramsTofRawDataLib)

add_executable(process_threshold_calibration.exe tools/process_threshold_calibration.cpp)
target_link_libraries(process_threshold_calibration.exe PRIVATE GramsTofScriptsCLib cli11)

add_executable(process_qdc_calibration.exe tools/process_qdc_calibration.cpp)
target_link_libraries(process_qdc_calibration.exe PRIVATE GramsTofScriptsCLib)

add_executable(process_tdc_calibration.exe tools/process_tdc_calibration.cpp)
target_link_libraries(process_tdc_calibration.exe PRIVATE GramsTofScriptsCLib)

add_executable(convert_raw_to_singles.exe tools/convert_raw_to_singles.cpp)
target_link_libraries(convert_raw_to_singles.exe PRIVATE GramsTofScriptsCLib)

add_executable(convert_raw_to_raw.exe tools/convert_raw_to_raw.cpp)
target_link_libraries(convert_raw_to_raw.exe PRIVATE GramsTofScriptsCLib)

install(TARGETS GramsTofScriptsCLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS process_threshold_calibration.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(TARGETS process_qdc_calibration.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(TARGETS process_tdc_calibration.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(TARGETS convert_raw_to_singles.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(TARGETS convert_raw_to_raw.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

