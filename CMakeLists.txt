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
option(INSTALL_KERNEL_MODULE "Install psdaq kernel module via DKMS" OFF)

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

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_MACOSX_RPATH TRUE)

# ROOT setup
if(DEFINED ENV{ROOTSYS})
  message(STATUS "Found ROOTSYS at $ENV{ROOTSYS}")
  list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
else()
  message(STATUS "ROOTSYS not found. Searching in /cern")
  message(STATUS "  => This will match the first ROOT install it finds, which might not be what you want!")
  list(APPEND CMAKE_PREFIX_PATH /cern)  # enable searches in old-school CERN paths
endif()

# Try to find ROOT using CMake package first
find_package(ROOT REQUIRED)
if(ROOT_FOUND)
  message(STATUS "ROOT found via find_package: ${ROOT_DIR}")
  include(${ROOT_USE_FILE})
else()
  message(STATUS "find_package(ROOT) failed. Falling back to root-config...")

  # Fallback using root-config
  execute_process(COMMAND root-config --incdir OUTPUT_VARIABLE ROOT_INCDIR)
  string(STRIP ${ROOT_INCDIR} ROOT_INCDIR)

  execute_process(COMMAND root-config --libs OUTPUT_VARIABLE ROOT_LIBS)
  string(STRIP ${ROOT_LIBS} ROOT_LIBS)

  execute_process(COMMAND root-config --cflags OUTPUT_VARIABLE ROOT_CXX_FLAGS)
  string(STRIP ${ROOT_CXX_FLAGS} ROOT_CXX_FLAGS)

  # Apply ROOT settings
  include_directories(${ROOT_INCDIR})
  link_libraries(${ROOT_LIBS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS}")
endif()

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

