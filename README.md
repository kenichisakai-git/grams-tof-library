# grams-tof-library

This project combines the following existing libraries:
- **FlightOps/Time-of-Flight**: ToF DAQ library

## Requirements
- **Python** >= 3.10  
- **Boost** >= 1.88.0 (with Boost.Python)
- **CMake** ≥ 3.0
---

## Building Boost with Boost.Python
### Ubuntu (or Debian-based systems)
On Ubuntu, Boost's build system can usually detect the system-installed Python automatically.  
You do **not** need a `user-config.jam`.
Make sure the necessary development packages are installed:

```bash
sudo apt update
sudo apt install python3-dev
./bootstrap.sh --prefix="$HOME/work/software/boost_1_88_0" --includedir=headers --libdir=dist --with-python=python3
./b2 --clean-all
./b2 install 
```
### Homebrew (macOS)
Boost.Python requires a `user-config.jam` file to build correctly when using Python installed via Homebrew.
See `FlightOps/Documents/boost-user-config-homebrew.jam` for an example.

```bash
cp -a FlightOps/Documents/boost-user-config-homebrew.jam $HOME/work/software/boost_1_88_0/user-config.jam
vim user-config.jam
./bootstrap.sh --prefix="$HOME/work/software/boost_1_88_0" --includedir=headers --libdir=dist --with-python=python3
./b2 --clean-all
./b2 install --user-config=./user-config.jam
```


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

