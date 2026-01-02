#pragma once

#include "CommunicationCodes.hh" 
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
    RUN_ACQUIRE_TDC_CALIBRATION            = 0x5107,
    RUN_ACQUIRE_QDC_CALIBRATION            = 0x5108,
    RUN_ACQUIRE_SIPM_DATA                  = 0x5109,
    RUN_ACQUIRE_THRESHOLD_CALIBRATION_BN   = 0x5110,
    RUN_ACQUIRE_THRESHOLD_CALIBRATION_D    = 0x5111,
    
    RUN_PROCESS_THRESHOLD_CALIBRATION      = 0x5200,
    RUN_PROCESS_TDC_CALIBRATION            = 0x5201,
    RUN_PROCESS_QDC_CALIBRATION            = 0x5202,
    RUN_CONVERT_RAW_TO_RAW                 = 0x5203,
    RUN_CONVERT_RAW_TO_SINGLES             = 0x5204,

    ACK                                    = 0x5FFF,
    CALLBACK                               = 0x5FFE,
    STATUS                                 = 0x5FFD, 
    DUMMY_TEST                             = 0x5FFC,

    HEART_BEAT                             = 0xFFFF,
    UNKNOWN                                = 0x0000 
};

enum class AckStatus : uint8_t {
    FAILURE = 0,
    SUCCESS = 1
};

inline std::ostream& operator<<(std::ostream& os, TOFCommandCode code) {
    switch (code) {
        case TOFCommandCode::START_DAQ:                            return os << "START_DAQ";
        case TOFCommandCode::STOP_DAQ:                             return os << "STOP_DAQ";
        case TOFCommandCode::RESET_DAQ:                            return os << "RESET_DAQ";

        case TOFCommandCode::RUN_INIT_SYSTEM:                      return os << "RUN_INIT_SYSTEM";
        case TOFCommandCode::RUN_MAKE_BIAS_CALIB_TABLE:            return os << "RUN_MAKE_BIAS_CALIB_TABLE";
        case TOFCommandCode::RUN_MAKE_SIMPLE_BIAS_SET_TABLE:       return os << "RUN_MAKE_SIMPLE_BIAS_SET_TABLE";
        case TOFCommandCode::RUN_MAKE_SIMPLE_CHANNEL_MAP:          return os << "RUN_MAKE_SIMPLE_CHANNEL_MAP";
        case TOFCommandCode::RUN_MAKE_SIMPLE_DISC_SET_TABLE:       return os << "RUN_MAKE_SIMPLE_DISC_SET_TABLE";
        case TOFCommandCode::RUN_READ_TEMPERATURE_SENSORS:         return os << "RUN_READ_TEMPERATURE_SENSORS";
        case TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION:    return os << "RUN_ACQUIRE_THRESHOLD_CALIBRATION";
        case TOFCommandCode::RUN_ACQUIRE_TDC_CALIBRATION:          return os << "RUN_ACQUIRE_TDC_CALIBRATION";
        case TOFCommandCode::RUN_ACQUIRE_QDC_CALIBRATION:          return os << "RUN_ACQUIRE_QDC_CALIBRATION";
        case TOFCommandCode::RUN_ACQUIRE_SIPM_DATA:                return os << "RUN_ACQUIRE_SIPM_DATA";
        case TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION_BN: return os << "RUN_ACQUIRE_THRESHOLD_CALIBRATION_BN";
        case TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION_D:  return os << "RUN_ACQUIRE_THRESHOLD_CALIBRATION_D";
        
        case TOFCommandCode::RUN_PROCESS_THRESHOLD_CALIBRATION:    return os << "RUN_PROCESS_THRESHOLD_CALIBRATION";
        case TOFCommandCode::RUN_PROCESS_TDC_CALIBRATION:          return os << "RUN_PROCESS_TDC_CALIBRATION";
        case TOFCommandCode::RUN_PROCESS_QDC_CALIBRATION:          return os << "RUN_PROCESS_QDC_CALIBRATION";
        case TOFCommandCode::RUN_CONVERT_RAW_TO_RAW:               return os << "RUN_CONVERT_RAW_TO_RAW";
        case TOFCommandCode::RUN_CONVERT_RAW_TO_SINGLES:           return os << "RUN_CONVERT_RAW_TO_SINGLES";

        case TOFCommandCode::ACK:                                  return os << "ACK";
        case TOFCommandCode::CALLBACK:                             return os << "CALLBACK";
        case TOFCommandCode::STATUS:                               return os << "STATUS";
        case TOFCommandCode::DUMMY_TEST:                           return os << "DUMMY_TEST";

        case TOFCommandCode::HEART_BEAT:                           return os << "HEART_BEAT";
        default:                                                   return os << "UNKNOWN";
    }
}

namespace tof_bridge {

using namespace pgrams::communication;

// Convert TOFCommandCode -> CommunicationCodes
inline CommunicationCodes toCommCode(TOFCommandCode code) {
    switch (code) {
        case TOFCommandCode::START_DAQ:                            return CommunicationCodes::TOF_Start_DAQ;
        case TOFCommandCode::STOP_DAQ:                             return CommunicationCodes::TOF_Stop_DAQ;
        case TOFCommandCode::RESET_DAQ:                            return CommunicationCodes::TOF_Reset_DAQ;

        case TOFCommandCode::RUN_INIT_SYSTEM:                      return CommunicationCodes::TOF_Run_Init_System;
        case TOFCommandCode::RUN_MAKE_BIAS_CALIB_TABLE:            return CommunicationCodes::TOF_Run_Make_Bias_Calib_Table;
        case TOFCommandCode::RUN_MAKE_SIMPLE_BIAS_SET_TABLE:       return CommunicationCodes::TOF_Run_Make_Simple_Bias_Set_Table;
        case TOFCommandCode::RUN_MAKE_SIMPLE_CHANNEL_MAP:          return CommunicationCodes::TOF_Run_Make_Simple_Channel_Map;
        case TOFCommandCode::RUN_MAKE_SIMPLE_DISC_SET_TABLE:       return CommunicationCodes::TOF_Run_Make_Simple_Disc_Set_Table;
        case TOFCommandCode::RUN_READ_TEMPERATURE_SENSORS:         return CommunicationCodes::TOF_Run_Read_Temperature_Sensors;
        case TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION:    return CommunicationCodes::TOF_Run_Acquire_Threshold_Calibration;
        case TOFCommandCode::RUN_ACQUIRE_TDC_CALIBRATION:          return CommunicationCodes::TOF_Run_Acquire_TDC_Calibration;
        case TOFCommandCode::RUN_ACQUIRE_QDC_CALIBRATION:          return CommunicationCodes::TOF_Run_Acquire_QDC_Calibration;
        case TOFCommandCode::RUN_ACQUIRE_SIPM_DATA:                return CommunicationCodes::TOF_Run_Acquire_SiPM_Data;
        case TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION_BN: return CommunicationCodes::TOF_Run_Acquire_Threshold_Calibration_BN;
        case TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION_D:  return CommunicationCodes::TOF_Run_Acquire_Threshold_Calibration_D;

        case TOFCommandCode::RUN_PROCESS_THRESHOLD_CALIBRATION:    return CommunicationCodes::TOF_Run_Process_Threshold_Calibration;
        case TOFCommandCode::RUN_PROCESS_TDC_CALIBRATION:          return CommunicationCodes::TOF_Run_Process_TDC_Calibration;
        case TOFCommandCode::RUN_PROCESS_QDC_CALIBRATION:          return CommunicationCodes::TOF_Run_Process_QDC_Calibration;
        case TOFCommandCode::RUN_CONVERT_RAW_TO_RAW:               return CommunicationCodes::TOF_Run_Convert_Raw_To_Raw;
        case TOFCommandCode::RUN_CONVERT_RAW_TO_SINGLES:           return CommunicationCodes::TOF_Run_Convert_Raw_To_Singles;

        case TOFCommandCode::ACK:                                  return CommunicationCodes::TOF_ACK;
        case TOFCommandCode::CALLBACK:                             return CommunicationCodes::TOF_Callback;
        case TOFCommandCode::DUMMY_TEST:                           return CommunicationCodes::TOF_DummyTest;

        case TOFCommandCode::HEART_BEAT:                           return CommunicationCodes::COM_HeartBeat;

        default: return CommunicationCodes::TOF_Status; // safe fallback
    }
}

// Convert CommunicationCodes -> TOFCommandCode
inline TOFCommandCode toTOFCommand(CommunicationCodes code) {
    switch (code) {
        case CommunicationCodes::TOF_Start_DAQ:                            return TOFCommandCode::START_DAQ;
        case CommunicationCodes::TOF_Stop_DAQ:                             return TOFCommandCode::STOP_DAQ;
        case CommunicationCodes::TOF_Reset_DAQ:                            return TOFCommandCode::RESET_DAQ;

        case CommunicationCodes::TOF_Run_Init_System:                      return TOFCommandCode::RUN_INIT_SYSTEM;
        case CommunicationCodes::TOF_Run_Make_Bias_Calib_Table:            return TOFCommandCode::RUN_MAKE_BIAS_CALIB_TABLE;
        case CommunicationCodes::TOF_Run_Make_Simple_Bias_Set_Table:       return TOFCommandCode::RUN_MAKE_SIMPLE_BIAS_SET_TABLE;
        case CommunicationCodes::TOF_Run_Make_Simple_Channel_Map:          return TOFCommandCode::RUN_MAKE_SIMPLE_CHANNEL_MAP;
        case CommunicationCodes::TOF_Run_Make_Simple_Disc_Set_Table:       return TOFCommandCode::RUN_MAKE_SIMPLE_DISC_SET_TABLE;
        case CommunicationCodes::TOF_Run_Read_Temperature_Sensors:         return TOFCommandCode::RUN_READ_TEMPERATURE_SENSORS;
        case CommunicationCodes::TOF_Run_Acquire_Threshold_Calibration:    return TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION;
        case CommunicationCodes::TOF_Run_Acquire_TDC_Calibration:          return TOFCommandCode::RUN_ACQUIRE_TDC_CALIBRATION;
        case CommunicationCodes::TOF_Run_Acquire_QDC_Calibration:          return TOFCommandCode::RUN_ACQUIRE_QDC_CALIBRATION;
        case CommunicationCodes::TOF_Run_Acquire_SiPM_Data:                return TOFCommandCode::RUN_ACQUIRE_SIPM_DATA;
        case CommunicationCodes::TOF_Run_Acquire_Threshold_Calibration_BN: return TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION_BN;
        case CommunicationCodes::TOF_Run_Acquire_Threshold_Calibration_D:  return TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION_D;

        case CommunicationCodes::TOF_Run_Process_Threshold_Calibration:    return TOFCommandCode::RUN_PROCESS_THRESHOLD_CALIBRATION;
        case CommunicationCodes::TOF_Run_Process_TDC_Calibration:          return TOFCommandCode::RUN_PROCESS_TDC_CALIBRATION;
        case CommunicationCodes::TOF_Run_Process_QDC_Calibration:          return TOFCommandCode::RUN_PROCESS_QDC_CALIBRATION;
        case CommunicationCodes::TOF_Run_Convert_Raw_To_Raw:               return TOFCommandCode::RUN_CONVERT_RAW_TO_RAW;
        case CommunicationCodes::TOF_Run_Convert_Raw_To_Singles:           return TOFCommandCode::RUN_CONVERT_RAW_TO_SINGLES;

        case CommunicationCodes::TOF_ACK:                                  return TOFCommandCode::ACK;
        case CommunicationCodes::TOF_Callback:                             return TOFCommandCode::CALLBACK;
        case CommunicationCodes::TOF_DummyTest:                            return TOFCommandCode::DUMMY_TEST;

        case CommunicationCodes::COM_HeartBeat:                            return TOFCommandCode::HEART_BEAT;

        default: return TOFCommandCode::UNKNOWN; // safe default
    }
}

} // namespace tof_bridge
