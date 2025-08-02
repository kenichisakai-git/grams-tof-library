#pragma once

#include <cstdint>
#include <ostream>

enum class TOFCommandCode : uint16_t {
    START_DAQ    = 0x5000,
    STOP_DAQ     = 0x5001,
    RESET        = 0x5002,
    RUN_BIAS_CAL = 0x5003,
};

inline std::ostream& operator<<(std::ostream& os, TOFCommandCode code) {
    switch (code) {
        case TOFCommandCode::START_DAQ:    return os << "START_DAQ";
        case TOFCommandCode::STOP_DAQ:     return os << "STOP_DAQ";
        case TOFCommandCode::RESET:        return os << "RESET";
        case TOFCommandCode::RUN_BIAS_CAL: return os << "RUN_BIAS_CAL";
        default:                           return os << "UNKNOWN_CODE";
    }
}

