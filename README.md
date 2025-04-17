# grams-tof-library

This project combines the following existing libraries:
- **FlightOps/Time-of-Flight**: ToF DAQ library

## Requirements
### Ubuntu (for FlightOps)
- **Python** ≥ 3.10  
- **CMake** ≥ 3.0
- **AIO** ≥ 0.3.112-13
- **Iniparser** ≥ 4.1-4
- **Boost** ≥ 1.88.0 (with Boost.Python)
- **ROOT** ≥ 6.28.02

#### Install essential development packages:
```bash
sudo apt update
sudo apt install python3-dev libaio-dev libaio-dev
```
---

#### Building Boost with Boost.Python
See [FlightOps/Documents/BOOST_BUILD.md](FlightOps/Documents/BOOST_BUILD.md) for detailed instructions on building Boost with Boost.Python on Ubuntu and macOS.

## Environment
```bash
export PLIB=$HOME/work/bess/bess-library/install
source $PLIB/Tool/config/bess_env.bash
```

## Analysis
### params
```bash
params75_250411_clib 
https://drive.google.com/file/d/1lZOkobAUrGxd_cStMqfEk9kSkADnI21n/view?usp=sharing
```

### generating DSTs
```bash
cd flight/run1001
cd params
proot.exe TOFQ -p -o ../dst/bessp_dst_NEG2025v1.root ../raw/bessp_raw*.dat 
```

### filling histograms on an event-by-event basis
```bash
vim $PLIB_SEL_DIRECTORY/TofChk_MT03.C #modify the selector
cd flight/run1001
cd params
root
root [0] .x $PLIB/selector_user/dstsel.C("../dst/bessp_dst_NEG2025v1.root","TofChk_MT03")
root ../dst/dstsel_NEG2025v1-TofChk_MT03.root #check histograms
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

