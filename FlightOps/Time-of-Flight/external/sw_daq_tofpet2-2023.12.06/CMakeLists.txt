project(sw_daq_tofpet2)
message("${BMagenta}[============ Entering Tofpet2 Subdirectory ============]${ColReset}")

file(GLOB SOURCES 
  "src/base/*.cpp"
  "src/daqd/*.cpp"
  "src/raw_data/*.cpp"
  "src/online_monitor/*.cpp"
)

find_program(LSB_RELEASE_EXEC lsb_release REQUIRED)
execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
  OUTPUT_VARIABLE LSB_ID_SHORT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(COMMAND ${LSB_RELEASE_EXEC} -rs
  OUTPUT_VARIABLE LSB_RELEASE_SHORT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

find_package(PythonLibs 3.6 REQUIRED)
if((${LSB_ID_SHORT} STREQUAL "Ubuntu") AND (${LSB_RELEASE_SHORT} VERSION_LESS_EQUAL "20.04"))
  find_package(Boost REQUIRED python regex)
elseif((EXISTS "/etc/redhat-release") AND (${LSB_RELEASE_SHORT} VERSION_GREATER_EQUAL "9"))
  find_package(Boost REQUIRED python39 regex)
else()
  find_package(Boost REQUIRED python3 regex)
endif()

if(UNIX AND NOT APPLE)
set( CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX" )
set( CMAKE_REQUIRED_DEFINITIONS -DLINUX )
endif()


########################################################################################################################
# Make a lib for all the stuff
add_library(Tofpet2Lib SHARED ${SOURCES})
target_include_directories(Tofpet2Lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/base>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/daqd>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/raw_data>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/online_monitor>
  $<INSTALL_INTERFACE:include/Tofpet2>
)
target_include_directories(Tofpet2Lib PUBLIC ${PYTHON_INCLUDE_DIRS})
target_include_directories(Tofpet2Lib PUBLIC /usr/include/iniparser)
target_link_libraries(Tofpet2Lib PUBLIC ${ROOT_LIBRARIES})
target_link_libraries(Tofpet2Lib PUBLIC ${Boost_LIBRARIES})
target_link_libraries(Tofpet2Lib PUBLIC ${PYTHON_LIBRARIES})
target_link_libraries(Tofpet2Lib PUBLIC iniparser)

add_executable(daqd.exe src/tools/daqd.cpp)
target_link_libraries(daqd.exe PUBLIC Tofpet2Lib)

add_executable(process_tdc_calibration.exe src/tools/process_tdc_calibration.cpp)
target_link_libraries(process_tdc_calibration.exe PUBLIC Tofpet2Lib)

add_executable(process_qdc_calibration.exe src/tools/process_qdc_calibration.cpp)
target_link_libraries(process_qdc_calibration.exe PUBLIC Tofpet2Lib)

add_executable(online_monitor.exe src/tools/online_monitor.cpp)
target_link_libraries(online_monitor.exe PUBLIC Tofpet2Lib)

add_executable(merge_raw.exe src/tools/merge_raw.cpp)
target_link_libraries(merge_raw.exe PUBLIC Tofpet2Lib)

add_executable(print_raw.exe src/tools/print_raw.cpp)
target_link_libraries(print_raw.exe PUBLIC Tofpet2Lib)

add_executable(convert_raw_to_raw.exe src/tools/convert_raw_to_raw.cpp)
target_link_libraries(convert_raw_to_raw.exe PUBLIC Tofpet2Lib)

add_executable(convert_raw_to_singles.exe src/tools/convert_raw_to_singles.cpp)
target_link_libraries(convert_raw_to_singles.exe PUBLIC Tofpet2Lib)

add_executable(convert_raw_to_group.exe src/tools/convert_raw_to_group.cpp)
target_link_libraries(convert_raw_to_group.exe PUBLIC Tofpet2Lib)

add_executable(convert_raw_to_coincidence.exe src/tools/convert_raw_to_coincidence.cpp)
target_link_libraries(convert_raw_to_coincidence.exe PUBLIC Tofpet2Lib)


install(TARGETS Tofpet2Lib
    EXPORT GramsLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS 
  daqd.exe 
  process_tdc_calibration.exe 
  process_qdc_calibration.exe 
  online_monitor.exe
  merge_raw.exe
  print_raw.exe
  convert_raw_to_raw.exe
  convert_raw_to_singles.exe
  convert_raw_to_group.exe
  convert_raw_to_coincidence.exe
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/Tofpet2
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/Tofpet2)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/pytools/ DESTINATION ${CMAKE_INSTALL_PREFIX}/pytools/Tofpet2)

