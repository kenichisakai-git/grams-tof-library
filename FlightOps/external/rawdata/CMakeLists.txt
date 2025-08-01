project(Grams_Tof_RawData)
message("${BMagenta}[============ Entering TOF RawData Subdirectory ============]${ColReset}")

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

include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

########################################################################################################################
# Build a Python module (no lib prefix)
add_library(shm_raw_py MODULE ${SOURCES})
set_target_properties(shm_raw_py PROPERTIES
    PREFIX ""
    OUTPUT_NAME shm_raw  # final output: shm_raw.so
)
target_link_libraries(shm_raw_py PRIVATE rt)

# C++ shared library
add_library(GramsTofRawDataLib SHARED ${SOURCES})

target_include_directories(shm_raw_py PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/FlightOps>
)
target_include_directories(GramsTofRawDataLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/FlightOps>
)

install(TARGETS shm_raw_py
    LIBRARY DESTINATION petsys
)

install(TARGETS GramsTofRawDataLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

