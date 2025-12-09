project(Grams_Tof_RawData)
message("${BMagenta}[============ Entering TOF RawData Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

# --- Python & Boost ---
find_package(Boost REQUIRED COMPONENTS python3 regex)

# --- Platform-specific flags ---
if(UNIX AND NOT APPLE)
    add_definitions(-DLINUX)
endif()

# --- Include directories ---
include_directories(${Boost_INCLUDE_DIRS})

# --- Build a Python module (no lib prefix) ---
add_library(shm_raw_py MODULE ${SOURCES})
set_target_properties(shm_raw_py PROPERTIES
    PREFIX ""
    OUTPUT_NAME shm_raw  # final output: shm_raw.so
)
target_include_directories(shm_raw_py PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/rawdata>
)
if(UNIX AND NOT APPLE)
    target_link_libraries(shm_raw_py PUBLIC rt)
endif()
target_link_libraries(shm_raw_py PUBLIC GramsTofBaseLib Python3::Python Boost::python3 ${Boost_LIBRARIES})

# --- C++ shared library ---
add_library(GramsTofRawDataLib SHARED ${SOURCES})
target_include_directories(GramsTofRawDataLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/rawdata>
)
if(UNIX AND NOT APPLE)
    target_link_libraries(GramsTofRawDataLib PUBLIC rt)
endif()
target_link_libraries(GramsTofRawDataLib PUBLIC GramsTofBaseLib Python3::Python Boost::python3 ${Boost_LIBRARIES})

# --- Executable ---
add_executable(write_raw tools/write_raw.cpp)
target_link_libraries(write_raw PRIVATE GramsTofRawDataLib)

# --- Install ---
install(TARGETS shm_raw_py
    LIBRARY DESTINATION petsys
)

install(TARGETS GramsTofRawDataLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS write_raw
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/rawdata)

