project(Grams_Tof_FlightOps)
message("${BMagenta}[============ Entering TOF FlightOps Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

############################################################################################################
# Platform-specific build control
############################################################################################################
if(APPLE)
    message(STATUS "Detected macOS build - minimal TOF DAQ configuration")

    add_subdirectory(external/base)
    add_subdirectory(external/rawdata)
    add_subdirectory(external/scripts_c)
    add_subdirectory(external/CLI11-2.2.0)

else()
    message(STATUS "Detected Linux build - full TOF DAQ system")

    # ----------------- Build Main Shared Library -----------------
    add_library(GramsTofFlightOpsLib SHARED ${SOURCES})
    target_include_directories(GramsTofFlightOpsLib PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/FlightOps>
    )

    target_link_libraries(GramsTofFlightOpsLib PUBLIC
        pybind11::embed
        Python3::Python
        GramsTofDaqdLib
        GramsTofInihLib
        GramsTofScriptsCLib
        GramsTofGroundOpsLib
        fmt::fmt
        rt
    )

    if(DEFINED ENV{NCURSES_ROOT})
        message(STATUS "Using ncurses from $ENV{NCURSES_ROOT}")
        include_directories($ENV{NCURSES_ROOT}/include)
        include_directories($ENV{NCURSES_ROOT}/include/ncursesw)
        link_directories($ENV{NCURSES_ROOT}/lib)
    endif()

    # ----------------- Executables -----------------
    add_executable(grams_tof_server_daq tools/grams_tof_server_daq.cpp)
    target_link_libraries(grams_tof_server_daq PRIVATE GramsTofFlightOpsLib cli11)

    add_executable(test_grams_tof_server_daq tools/test_grams_tof_server_daq.cpp)
    target_link_libraries(test_grams_tof_server_daq PRIVATE GramsTofFlightOpsLib cli11)

    add_executable(grams_tof_client_daq tools/grams_tof_client_daq.cpp)
    target_link_libraries(grams_tof_client_daq PRIVATE GramsTofFlightOpsLib cli11)

    add_executable(test_grams_tof_client_daq tools/test_grams_tof_client_daq.cpp)
    #target_link_libraries(test_grams_tof_client_daq PRIVATE GramsTofFlightOpsLib cli11 ncursesw)
    target_link_libraries(test_grams_tof_client_daq PRIVATE GramsTofFlightOpsLib cli11 ncursesw tinfow)
 
    add_executable(test_grams_tof_config tools/test_grams_tof_config.cpp)
    target_link_libraries(test_grams_tof_config PRIVATE GramsTofFlightOpsLib)

    # ----------------- Install Targets -----------------
    install(TARGETS GramsTofFlightOpsLib
        EXPORT GramsTofLibraryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )

    install(TARGETS grams_tof_server_daq
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )
    install(TARGETS test_grams_tof_server_daq
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )
    install(TARGETS grams_tof_client_daq
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )
    install(TARGETS test_grams_tof_client_daq
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )
    install(TARGETS test_grams_tof_config
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )

    # ----------------- Install Configs / Scripts -----------------
    install(DIRECTORY external/petsys/  DESTINATION ${CMAKE_INSTALL_PREFIX}/petsys)
    install(DIRECTORY external/scripts/ DESTINATION ${CMAKE_INSTALL_PREFIX}/scripts)
    install(DIRECTORY external/config/  DESTINATION ${CMAKE_INSTALL_PREFIX}/config)
    install(DIRECTORY sh/               DESTINATION ${CMAKE_INSTALL_PREFIX}/sh)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/FlightOps)

    # ----------------- External Libraries -----------------
    add_subdirectory(external/base)
    add_subdirectory(external/rawdata)
    add_subdirectory(external/daqd)
    add_subdirectory(external/pybind11)
    add_subdirectory(external/inih)
    add_subdirectory(external/scripts_c)
    add_subdirectory(external/CLI11-2.2.0)
    add_subdirectory(external/shota)

    if(INSTALL_KERNEL_MODULE)
        add_subdirectory(external/kernel)
    endif()

    # ----------------- fmt Library -----------------
    set(FMT_INSTALL ON CACHE BOOL "" FORCE)
    set(FMT_STATIC ON CACHE BOOL "" FORCE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_subdirectory(external/fmt-7.1.3)
    set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)

endif()

