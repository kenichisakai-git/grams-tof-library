project(Grams_Tof_Base)
message("${BMagenta}[============ Entering TOF Base Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

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
    $<INSTALL_INTERFACE:include/FlightOps>
)
target_link_libraries(GramsTofBaseLib PUBLIC iniparser)

install(TARGETS GramsTofBaseLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

