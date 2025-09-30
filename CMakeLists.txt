cmake_minimum_required(VERSION 3.20)

project(grams-tof-library VERSION 1.0)

include(Common/cmake/root_tools.cmake)
include(Common/cmake/colors.cmake)
include(Common/cmake/props.cmake)
include(Common/cmake/targets.cmake)

include(GNUInstallDirs)
include(ExternalProject)

# Add some global compiler options
add_compile_options (-fdiagnostics-color=always)

# Developer-only compiler flags
option(ENABLE_DEV_FLAGS "Enable developer-only compiler flags (warnings, sanitizers, debug info)" OFF)

# Configure
if(ENABLE_DEV_FLAGS)
    message(STATUS "Developer flags enabled: -Wall -Wextra -g -fsanitize=thread")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -fsanitize=thread")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=thread")
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
    endif()
endif()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

# To the user:
# This is where you specify the location of the installation directory.
# You can change this to any directory of your choice where you want the built project to be installed.
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/00install")

#ROOT
if(DEFINED ENV{ROOTSYS})
  message("Found ROOTSYS at $ENV{ROOTSYS}")
else()
  message("ROOTSYS not found.  Searching in /cern.")
  message("  => This will match the first ROOT install it finds, which might not be what you want!")
  list(APPEND CMAKE_PREFIX_PATH /cern)     # enable searches in old-school cern paths
endif()
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

add_subdirectory(FlightOps)
#install(DIRECTORY ${CMAKE_SOURCE_DIR}/Tool/ DESTINATION ${CMAKE_INSTALL_PREFIX}/Tool)

install(EXPORT GramsTofLibraryTargets
    FILE GramsTofLibraryTargets.cmake
    NAMESPACE Grams::
    DESTINATION lib/cmake/GramsTofLibrary
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/GramsTofLibraryConfigVersion.cmake"
    VERSION 1.0
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Common/cmake/GramsTofLibraryConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/GramsTofLibraryConfig.cmake"
    INSTALL_DESTINATION lib/cmake/GramsTofLibrary
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/GramsTofLibraryConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/GramsTofLibraryConfigVersion.cmake"
    DESTINATION lib/cmake/GramsTofLibrary
)

