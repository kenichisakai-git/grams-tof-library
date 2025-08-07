project(Grams_Tof_Inih)
message("${BMagenta}[============ Entering TOF Inih Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cpp" "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(GramsTofInihLib SHARED ${SOURCES})
target_include_directories(GramsTofInihLib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/FlightOps>
)

install(TARGETS GramsTofInihLib
    EXPORT GramsTofLibraryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

