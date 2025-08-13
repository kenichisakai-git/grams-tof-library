#pragma once

#include <cstdint>
#include <ostream>

enum class TOFCommandCode : uint16_t {
    START_DAQ                              = 0x5000,
    STOP_DAQ                               = 0x5001,
    RESET_DAQ                              = 0x5002,

    RUN_INIT_SYSTEM                        = 0x5100,
    RUN_MAKE_BIAS_CALIB_TABLE              = 0x5101,
    RUN_MAKE_SIMPLE_BIAS_SET_TABLE         = 0x5102,
    RUN_MAKE_SIMPLE_CHANNEL_MAP            = 0x5103,
    RUN_MAKE_SIMPLE_DISC_SET_TABLE         = 0x5104,
    RUN_READ_TEMPERATURE_SENSORS           = 0x5105,
    RUN_ACQUIRE_THRESHOLD_CALIBRATION      = 0x5106,
    RUN_ACQUIRE_QDC_CALIBRATION            = 0x5107,
    RUN_ACQUIRE_TDC_CALIBRATION            = 0x5108,
    RUN_ACQUIRE_SIPM_DATA                  = 0x5109,
    
    RUN_PROCESS_THRESHOLD_CALIBRATION      = 0x5200,
    RUN_PROCESS_TDC_CALIBRATION            = 0x5201,
    RUN_PROCESS_QDC_CALIBRATION            = 0x5202,
    RUN_CONVERT_RAW_TO_SINGLES             = 0x5203,

    ACK                                    = 0x5FFF
};

enum class AckStatus : uint8_t {
    FAILURE = 0,
    SUCCESS = 1
};

inline std::ostream& operator<<(std::ostream& os, TOFCommandCode code) {
    switch (code) {
        case TOFCommandCode::START_DAQ:                           return os << "START_DAQ";
        case TOFCommandCode::STOP_DAQ:                            return os << "STOP_DAQ";
        case TOFCommandCode::RESET_DAQ:                           return os << "RESET_DAQ";

        case TOFCommandCode::RUN_INIT_SYSTEM:                     return os << "RUN_INIT_SYSTEM";
        case TOFCommandCode::RUN_MAKE_BIAS_CALIB_TABLE:           return os << "RUN_MAKE_BIAS_CALIB_TABLE";
        case TOFCommandCode::RUN_MAKE_SIMPLE_BIAS_SET_TABLE:      return os << "RUN_MAKE_SIMPLE_BIAS_SET_TABLE";
        case TOFCommandCode::RUN_MAKE_SIMPLE_CHANNEL_MAP:         return os << "RUN_MAKE_SIMPLE_CHANNEL_MAP";
        case TOFCommandCode::RUN_MAKE_SIMPLE_DISC_SET_TABLE:      return os << "RUN_MAKE_SIMPLE_DISC_SET_TABLE";
        case TOFCommandCode::RUN_READ_TEMPERATURE_SENSORS:        return os << "RUN_READ_TEMPERATURE_SENSORS";
        case TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION:   return os << "RUN_ACQUIRE_THRESHOLD_CALIBRATION";
        case TOFCommandCode::RUN_ACQUIRE_QDC_CALIBRATION:         return os << "RUN_ACQUIRE_QDC_CALIBRATION";
        case TOFCommandCode::RUN_ACQUIRE_TDC_CALIBRATION:         return os << "RUN_ACQUIRE_TDC_CALIBRATION";
        case TOFCommandCode::RUN_ACQUIRE_SIPM_DATA:               return os << "RUN_ACQUIRE_SIPM_DATA";
        
        case TOFCommandCode::RUN_PROCESS_THRESHOLD_CALIBRATION:   return os << "RUN_PROCESS_THRESHOLD_CALIBRATION";
        case TOFCommandCode::RUN_PROCESS_TDC_CALIBRATION:         return os << "RUN_PROCESS_TDC_CALIBRATION";
        case TOFCommandCode::RUN_PROCESS_QDC_CALIBRATION:         return os << "RUN_PROCESS_QDC_CALIBRATION";
        case TOFCommandCode::RUN_CONVERT_RAW_TO_SINGLES:          return os << "RUN_CONVERT_RAW_TO_SINGLES";
        case TOFCommandCode::ACK:                                 return os << "ACK";

        default:                                                  return os << "UNKNOWN_CODE";
    }
}

