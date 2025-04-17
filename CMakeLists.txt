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

# Configure
set(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

# To the user:
# This is where you specify the location of the installation directory.
# You can change this to any directory of your choice where you want the built project to be installed.
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install") 

if(DEFINED ENV{ROOTSYS})
  message("Found ROOTSYS at $ENV{ROOTSYS}")
else()
  message("ROOTSYS not found.  Searching in /cern.")
  message("  => This will match the first ROOT install it finds, which might not be what you want!")
  list(APPEND CMAKE_PREFIX_PATH /cern)     # enable searches in old-school cern paths
endif()
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

add_subdirectory(FlightOps/Time-of-Flight)

#install(EXPORT GramsLibraryTargets
#    FILE GramsLibraryTargets.cmake
#    NAMESPACE Grams::
#    DESTINATION lib/cmake/GramsLibrary
#)

#include(CMakePackageConfigHelpers)

#write_basic_package_version_file(
#    "${CMAKE_CURRENT_BINARY_DIR}/GramsLibraryConfigVersion.cmake"
#    VERSION 1.0
#    COMPATIBILITY AnyNewerVersion
#)

#configure_package_config_file(
#    "${CMAKE_CURRENT_SOURCE_DIR}/Common/cmake/GramsLibraryConfig.cmake.in"
#    "${CMAKE_CURRENT_BINARY_DIR}/GramsLibraryConfig.cmake"
#    INSTALL_DESTINATION lib/cmake/GramsLibrary
#)

#install(FILES
#    "${CMAKE_CURRENT_BINARY_DIR}/GramsLibraryConfig.cmake"
#    "${CMAKE_CURRENT_BINARY_DIR}/GramsLibraryConfigVersion.cmake"
#    DESTINATION lib/cmake/GramsLibrary
#)

