project(Grams_Tof_FlightOps)
message("${BMagenta}[============ Entering TOF FlightOps Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

########################################################################################################################
# Make a lib for all the stuff
add_library(GramsTofFlightOpsLib SHARED ${SOURCES})
target_include_directories(GramsTofFlightOpsLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/FlightOps>
)
target_link_libraries(GramsTofFlightOpsLib PUBLIC pybind11::embed)
target_link_libraries(GramsTofFlightOpsLib PUBLIC Python3::Python)
target_link_libraries(GramsTofFlightOpsLib PUBLIC GramsTofDaqdLib)
target_link_libraries(GramsTofFlightOpsLib PUBLIC GramsTofInihLib)
target_link_libraries(GramsTofFlightOpsLib PUBLIC GramsTofScriptsCLib)
target_link_libraries(GramsTofFlightOpsLib PUBLIC fmt::fmt)
target_link_libraries(GramsTofFlightOpsLib PUBLIC rt)

add_executable(grams_tof_daq.exe tools/grams_tof_daq.cpp)
target_link_libraries(grams_tof_daq.exe PRIVATE GramsTofFlightOpsLib)

add_executable(grams_tof_client.exe tools/grams_tof_client.cpp)
target_link_libraries(grams_tof_client.exe PRIVATE GramsTofFlightOpsLib)

add_executable(grams_tof_config_test.exe tools/grams_tof_config_test.cpp)
target_link_libraries(grams_tof_config_test.exe PRIVATE GramsTofFlightOpsLib)

install(TARGETS GramsTofFlightOpsLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS grams_tof_daq.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(TARGETS grams_tof_client.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(TARGETS grams_tof_config_test.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(DIRECTORY external/petsys/  DESTINATION ${CMAKE_INSTALL_PREFIX}/petsys)
install(DIRECTORY external/scripts/ DESTINATION ${CMAKE_INSTALL_PREFIX}/scripts)
install(DIRECTORY external/config/ DESTINATION ${CMAKE_INSTALL_PREFIX}/config)

add_subdirectory(external/base)
add_subdirectory(external/kernel)
add_subdirectory(external/rawdata)
add_subdirectory(external/daqd)
add_subdirectory(external/pybind11)
add_subdirectory(external/inih)
add_subdirectory(external/scripts_c)

set(FMT_INSTALL ON CACHE BOOL "" FORCE)
set(FMT_STATIC ON CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(external/fmt-7.1.3)
set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
