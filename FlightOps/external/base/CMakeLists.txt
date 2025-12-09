project(Grams_Tof_Base)
message("${BMagenta}[============ Entering TOF Base Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

# --- iniparser ---
set(INIPARSER_INCLUDE_DIR $ENV{INIPARSER_INCLUDE_DIR} CACHE PATH "iniparser include dir")
set(INIPARSER_LIBRARY $ENV{INIPARSER_LIBRARY} CACHE FILEPATH "iniparser library")

if(NOT EXISTS ${INIPARSER_INCLUDE_DIR}/iniparser/iniparser.h OR NOT EXISTS ${INIPARSER_LIBRARY})
  message(FATAL_ERROR "iniparser not found. Please export INIPARSER_INCLUDE_DIR and INIPARSER_LIBRARY.")
endif()

include_directories(${INIPARSER_INCLUDE_DIR})

# --- Python & Boost ---
find_package(Boost REQUIRED COMPONENTS python3 regex)

# --- Platform-specific flags ---
if(UNIX AND NOT APPLE)
  add_definitions(-DLINUX)
endif()

# --- Include and link directories ---
include_directories(${Boost_INCLUDE_DIRS})

find_path(INIPARSER_INCLUDE_DIR NAMES iniparser.h PATHS /usr/include/iniparser)
find_library(INIPARSER_LIBRARY NAMES iniparser)

if(NOT INIPARSER_INCLUDE_DIR OR NOT INIPARSER_LIBRARY)
    message(FATAL_ERROR "iniparser not found. Please install libiniparser-dev or set INIPARSER_INCLUDE_DIR / INIPARSER_LIBRARY manually.")
endif()

########################################################################################################################
# Make a lib for all the stuff
add_library(GramsTofBaseLib SHARED ${SOURCES})
target_include_directories(GramsTofBaseLib PUBLIC
    ${INIPARSER_INCLUDE_DIR}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/base>
)
#target_link_libraries(GramsTofBaseLib PUBLIC iniparser)
target_link_libraries(GramsTofBaseLib PUBLIC ${INIPARSER_LIBRARY})
target_link_libraries(GramsTofBaseLib PUBLIC Python3::Python)
target_link_libraries(GramsTofBaseLib PUBLIC Boost::python3)

install(TARGETS GramsTofBaseLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/base)

