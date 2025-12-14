project(Grams_Tof_Kernel)
message("${BMagenta}[============ Entering TOF Kernel Subdirectory ============]${ColReset}")

cmake_minimum_required(VERSION 3.12)
project(psdaq_kernel_module)

# Optional: Set kernel headers path
set(KERNEL_DIR /lib/modules/${CMAKE_SYSTEM_VERSION}/build)

# Custom target to clean old modules, unload, and install new one
add_custom_target(install_psdaq_module
    COMMAND find /lib/modules -name psdaq.ko.xz -exec rm -f {} \\\;
    COMMAND dkms remove psdaq/4.0 --all || true
    COMMAND rm -rf /var/lib/dkms/psdaq
    COMMAND modprobe -r psdaq || true
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/udev/52-psdaq.rules /etc/udev/rules.d
    COMMAND dkms add ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND dkms build psdaq/4.0
    COMMAND dkms install psdaq/4.0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Installing psdaq kernel module via DKMS"
)

