#ifndef __PSDAQ_H__
#define __PSDAQ_H__

#ifdef __linux__
#include <linux/ioctl.h>
#include <linux/types.h>

struct ioctl_reg_t
{
    uint32_t offset;
    uint32_t value;
};
#define PSDAQ_IOCTL_READ_REGISTER _IOR('q', 1, struct ioctl_reg_t *)
#define PSDAQ_IOCTL_WRITE_REGISTER _IOW('q', 2, struct ioctl_reg_t *)
#define PSDAQ_IOCTL_READ_VERSION _IOR('q', 3, struct ioctl_reg_t *)

#else  // macOS stub
#include <stdint.h>
struct ioctl_reg_t
{
    uint32_t offset;
    uint32_t value;
};
#define PSDAQ_IOCTL_READ_REGISTER 0
#define PSDAQ_IOCTL_WRITE_REGISTER 0
#define PSDAQ_IOCTL_READ_VERSION 0

#endif

#endif

