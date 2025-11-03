project(Grams_Tof_Daqd)
message("${BMagenta}[============ Entering TOF Daqd Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

find_package(PythonLibs 3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS python3 regex)

if(UNIX AND NOT APPLE)
  add_definitions(-DLINUX)
endif()

include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

########################################################################################################################
# Make a lib for all the stuff
add_library(GramsTofDaqdLib SHARED ${SOURCES})
target_include_directories(GramsTofDaqdLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../kernel/include> 
    $<INSTALL_INTERFACE:include/FlightOps>
)
target_link_libraries(GramsTofDaqdLib PUBLIC GramsTofRawDataLib)

add_executable(daqd.exe tools/daqd.cpp)
target_link_libraries(daqd.exe PRIVATE GramsTofDaqdLib)
target_link_libraries(daqd.exe PRIVATE rt)

install(TARGETS GramsTofDaqdLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS daqd.exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

