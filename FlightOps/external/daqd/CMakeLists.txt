project(Grams_Tof_Daqd)
message("${BMagenta}[============ Entering TOF Daqd Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

find_package(Boost REQUIRED COMPONENTS python3 regex)

if(UNIX AND NOT APPLE)
  add_definitions(-DLINUX)
endif()

include_directories(${Boost_INCLUDE_DIRS})

########################################################################################################################
# Make a lib for all the stuff
add_library(GramsTofDaqdLib SHARED ${SOURCES})
target_include_directories(GramsTofDaqdLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../kernel> 
    $<INSTALL_INTERFACE:include/daqd>
)
target_link_libraries(GramsTofDaqdLib PUBLIC GramsTofRawDataLib)
target_link_libraries(GramsTofDaqdLib PUBLIC Python3::Python Boost::python3 ${Boost_LIBRARIES})

add_executable(daqd tools/daqd.cpp)
target_link_libraries(daqd PRIVATE GramsTofDaqdLib)
target_link_libraries(daqd PRIVATE rt)

install(TARGETS GramsTofDaqdLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS daqd
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/daqd)

