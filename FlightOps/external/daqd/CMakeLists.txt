project(Grams_Tof_Daqd)
message("${BMagenta}[============ Entering TOF Daqd Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

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
add_library(GramsTofDaqdLib SHARED ${SOURCES})
target_include_directories(GramsTofDaqdLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../kernel/include> 
    $<INSTALL_INTERFACE:include/FlightOps>
)

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

