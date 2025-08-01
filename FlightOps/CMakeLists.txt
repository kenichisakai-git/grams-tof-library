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
target_link_libraries(GramsTofFlightOpsLib PUBLIC rt)

add_executable(grams_tof_daq.exe tools/grams_tof_daq.cpp)
target_link_libraries(grams_tof_daq.exe PRIVATE GramsTofFlightOpsLib)

install(TARGETS GramsTofFlightOpsLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS grams_tof_daq.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(DIRECTORY external/petsys/  DESTINATION ${CMAKE_INSTALL_PREFIX}/petsys)
install(DIRECTORY external/scripts/ DESTINATION ${CMAKE_INSTALL_PREFIX}/scripts)

add_subdirectory(external/kernel)
add_subdirectory(external/rawdata)
add_subdirectory(external/daqd)
add_subdirectory(external/pybind11)

