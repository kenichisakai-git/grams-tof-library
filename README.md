# grams-tof-library

This project combines the following existing libraries:
- **FlightOps/Time-of-Flight**: ToF DAQ library based on Tofpet2 library

## Requirements
### Ubuntu (for FlightOps)
- **Python** ≥ 3.10  
- **CMake** ≥ 3.0
- **AIO** ≥ 0.3.112-13
- **Iniparser** ≥ 4.1-4
- **Boost** ≥ 1.88.0 (with Boost.Python)
- **ROOT** ≥ 6.28.02

Tofpet2 officially supports Ubuntu 20.04 and 22.04, as well as RHEL/CentOS versions 7 through 9.
See [README.md](FlightOps/Time-of-Flight/external/sw_daq_tofpet2-2025.01.24/README.md) for Tofpet2. Changes were applied due to the limited expandability of the original implementation.

#### Install essential development packages:
```bash
sudo apt update
sudo apt install python3-dev libaio-dev libaio-dev
```
---

#### Building Boost with Boost.Python
See [BOOST_BUILD.md](FlightOps/Documents/BOOST_BUILD.md) for detailed instructions on building Boost with Boost.Python on Ubuntu and macOS.

## Building 
```bash
mkdir build
cd build
cmake ../.
make -j$(nproc)  # make using 50% of your cores.
make install # copy lib/bin/dat to install directoy(install).
cd ../install
sudo driver/INSTALL_DRIVERS.sh
```

## Environment
```bash
export GRAMS_TOF_LIB=$HOME/work/grams/grams-tof-library/install
source $GRAMS_TOF_LIB/Tool/config/grams_env.bash
```

## linked by external library
```bash
find_package(BessLibrary REQUIRED)
if (BessLibrary_FOUND)
    message(STATUS "BessLibrary found successfully!")
else()
    message(WARNING "BessLibrary not found.")
endif()
...
get_target_property(inc_dirs Bess::BessAnalysisLib INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "BessAnalysisLib includes: ${inc_dirs}") #just check
...
target_link_libraries(GalpropDataLib PUBLIC Bess::BessAnalysisLib) #header dir is automatically loaded
...
```

