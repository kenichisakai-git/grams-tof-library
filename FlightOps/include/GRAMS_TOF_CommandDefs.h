#pragma once

#include <cstdint>
#include <ostream>

enum class TOFCommandCode : uint16_t {
    START_DAQ    = 0x5000,
    STOP_DAQ     = 0x5001,
    RESET_DAQ    = 0x5002,

    RUN_INIT_SYS = 0x5100,
    RUN_BIAS_CAL = 0x5101,
};

inline std::ostream& operator<<(std::ostream& os, TOFCommandCode code) {
    switch (code) {
        case TOFCommandCode::START_DAQ:    return os << "START_DAQ";
        case TOFCommandCode::STOP_DAQ:     return os << "STOP_DAQ";
        case TOFCommandCode::RESET_DAQ:    return os << "RESET_DAQ";

        case TOFCommandCode::RUN_INIT_SYS: return os << "RUN_INIT_SYS";
        case TOFCommandCode::RUN_BIAS_CAL: return os << "RUN_BIAS_CAL";
        default:                           return os << "UNKNOWN_CODE";
    }
}

