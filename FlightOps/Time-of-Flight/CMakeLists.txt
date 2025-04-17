project(Grams_Time-of-Flight)
message("${BMagenta}[============ Entering Time-of-Flight Subdirectory ============]${ColReset}")

file(GLOB SOURCES "src/*.cc")
file(GLOB HEADERS "include/*.h")

add_subdirectory(external/sw_daq_tofpet2-2025.01.24)
