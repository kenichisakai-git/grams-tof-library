#include "GRAMS_TOF_CommandDispatch.h"
#include "GRAMS_TOF_Logger.h"
#include "GRAMS_TOF_Config.h"

GRAMS_TOF_CommandDispatch::GRAMS_TOF_CommandDispatch(
    GRAMS_TOF_PythonIntegration& pyint,
    GRAMS_TOF_Analyzer& analyzer)
    : pyint_(pyint), analyzer_(analyzer), table_{}
{
    auto& config = GRAMS_TOF_Config::instance();

    // START_DAQ
    table_[TOFCommandCode::START_DAQ] = [&](const std::vector<int>&) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Starting DAQ...");
        
            std::lock_guard<std::mutex> lock(daqMutex_);
        
            if (daqRunning_) {
                Logger::instance().warn("[GRAMS_TOF_CommandDispatch] DAQ is already running.");
                return false;
            }
        
            // Join previous thread if needed
            if (daqThread_.joinable()) {
                daqThread_.join();
            }
        
            daqRunning_ = true;
            daqThread_ = std::thread(&GRAMS_TOF_CommandDispatch::runDAQThread, this);
            return true;
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in START_DAQ");
            return false;
        }
    };

    // STOP_DAQ
    table_[TOFCommandCode::STOP_DAQ] = [&](const std::vector<int>&) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Stopping DAQ...");
            {
                std::lock_guard<std::mutex> lock(daqMutex_);
                if (!daqRunning_) {
                    Logger::instance().warn("[GRAMS_TOF_CommandDispatch] DAQ is not running.");
                    return false;
                }
            }

            pyint_.getDAQ().stop();

            if (daqThread_.joinable()) {
                daqThread_.join();
            }

            {
                std::lock_guard<std::mutex> lock(daqMutex_);
                daqRunning_ = false;
            }
            return true;
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in STOP_DAQ");
            return false;
        }
    };

    // RESET_DAQ
    table_[TOFCommandCode::RESET_DAQ] = [&](const std::vector<int>&) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Resetting DAQ...");
            pyint_.getDAQ().reset();
            return true;
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RESET_DAQ");
            return false;
        }
    };

    // RUN_INIT_SYSTEM
    table_[TOFCommandCode::RUN_INIT_SYSTEM] = [&](const std::vector<int>&) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing init_system.py script...");
            return pyint_.runPetsysInitSystem("scripts.init_system");
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_INIT_SYSTEM");
            return false;
        }
    };

    // RUN_MAKE_BIAS_CALIB_TABLE
    table_[TOFCommandCode::RUN_MAKE_BIAS_CALIB_TABLE] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_bias_calibration_table.py script...");
            return pyint_.runPetsysMakeBiasCalibrationTable(
                "scripts.make_bias_calibration_table",
                config.getString("main", "bias_calibration_table"),
                {argv.size() > 0 ? argv[0] : 0},
                {argv.size() > 1 ? argv[1] : 0},
                {argv.size() > 2 ? argv[2] : 0},
                {}
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_MAKE_BIAS_CALIB_TABLE");
            return false;
        }
    };

    // RUN_MAKE_SIMPLE_BIAS_SET_TABLE
    table_[TOFCommandCode::RUN_MAKE_SIMPLE_BIAS_SET_TABLE] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_simple_bias_settings_table.py script...");
            return pyint_.runPetsysMakeSimpleBiasSettingsTable(
                "scripts.make_simple_bias_settings_table",
                config.getConfigFilePath(),
                argv.size() > 0 ? static_cast<float>(argv[0]) : 0.75f,
                argv.size() > 1 ? static_cast<float>(argv[1]) : 20.0f,
                argv.size() > 2 ? static_cast<float>(argv[2]) : 24.9f,
                argv.size() > 3 ? static_cast<float>(argv[3]) : 5.0f,
                config.getString("main", "bias_settings_table")
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_MAKE_SIMPLE_BIAS_SET_TABLE");
            return false;
        }
    };

    // RUN_MAKE_SIMPLE_CHANNEL_MAP
    table_[TOFCommandCode::RUN_MAKE_SIMPLE_CHANNEL_MAP] = [&](const std::vector<int>&) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_simple_channel_map.py script...");
            return pyint_.runPetsysMakeSimpleChannelMap(
                "scripts.make_simple_channel_map",
                config.getString("main", "channel_map")
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_MAKE_SIMPLE_CHANNEL_MAP");
            return false;
        }
    };

    // RUN_MAKE_SIMPLE_DISC_SET_TABLE
    table_[TOFCommandCode::RUN_MAKE_SIMPLE_DISC_SET_TABLE] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_simple_disc_settings_table.py script...");
            return pyint_.runPetsysMakeSimpleDiscSettingsTable(
                "scripts.make_simple_disc_settings_table",
                config.getConfigFilePath(),
                argv.size() > 0 ? argv[0] : 20,
                argv.size() > 1 ? argv[1] : 20,
                argv.size() > 2 ? argv[2] : 15,
                config.getString("main", "disc_calibration_table")
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_MAKE_SIMPLE_DISC_SET_TABLE");
            return false;
        }
    };

    // RUN_READ_TEMPERATURE_SENSORS
    table_[TOFCommandCode::RUN_READ_TEMPERATURE_SENSORS] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing read_temperature_sensors.py script...");
            return pyint_.runPetsysReadTemperatureSensors(
                "scripts.read_temperature_sensors",
                argv.size() > 0 ? static_cast<double>(argv[0]) : 0.0,
                argv.size() > 1 ? static_cast<double>(argv[1]) : 60.0,
                "/dev/null",
                argv.size() > 2 ? static_cast<bool>(argv[2]) : true,
                argv.size() > 3 ? static_cast<bool>(argv[3]) : false
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_READ_TEMPERATURE_SENSORS");
            return false;
        }
    };

    // RUN_ACQUIRE_THRESHOLD_CALIBRATION
    table_[TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_threshold_calibration.py script...");
            return pyint_.runPetsysAcquireThresholdCalibration(
                "scripts.acquire_threshold_calibration",
                config.getConfigFilePath(),
                config.getFileStem("main", "disc_calibration_table"),
                argv.size() > 0 ? argv[0] : 4,
                argv.size() > 1 ? argv[1] : 4,
                argv.size() > 2 ? static_cast<bool>(argv[2]) : false
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_ACQUIRE_THRESHOLD_CALIBRATION");
            return false;
        }
    };

    // RUN_ACQUIRE_QDC_CALIBRATION
    table_[TOFCommandCode::RUN_ACQUIRE_QDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_qdc_calibration.py script...");
            return pyint_.runPetsysAcquireQdcCalibration(
                "scripts.acquire_qdc_calibration",
                config.getConfigFilePath(),
                config.getFileStem("main", "qdc_calibration_table")
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_ACQUIRE_QDC_CALIBRATION");
            return false;
        }
    };

    // RUN_ACQUIRE_TDC_CALIBRATION
    table_[TOFCommandCode::RUN_ACQUIRE_TDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_tdc_calibration.py script...");
            return pyint_.runPetsysAcquireTdcCalibration(
                "scripts.acquire_tdc_calibration",
                config.getConfigFilePath(),
                config.getFileStem("main", "tdc_calibration_table")
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_ACQUIRE_TDC_CALIBRATION");
            return false;
        }
    };

    // RUN_ACQUIRE_SIPM_DATA
    table_[TOFCommandCode::RUN_ACQUIRE_SIPM_DATA] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_sipm_data.py script...");
            return pyint_.runPetsysAcquireSipmData(
                "scripts.acquire_sipm_data",
                config.getConfigFilePath(),
                "run_test",
                argv.size() > 0 ? static_cast<double>(argv[0]) : 60.0,
                "qdc",
                argv.size() > 1 ? static_cast<bool>(argv[1]) : false,
                ""
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_ACQUIRE_SIPM_DATA");
            return false;
        }
    };

    // RUN_PROCESS_THRESHOLD_CALIBRATION
    table_[TOFCommandCode::RUN_PROCESS_THRESHOLD_CALIBRATION] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Running threshold calibration...");
            return analyzer_.runPetsysProcessThresholdCalibration(
                config.getConfigFilePath(),
                config.getFileStem("main", "disc_calibration_table"),
                config.getString("main", "disc_calibration_table"),
                "disc_calibration.root"
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_PROCESS_THRESHOLD_CALIBRATION");
            return false;
        }
    };

    // RUN_PROCESS_TDC_CALIBRATION
    table_[TOFCommandCode::RUN_PROCESS_TDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Running TDC calibration...");
            bool doSorting = argv.size() > 0 ? (argv[0] != 0) : true;
            bool keepTmp   = argv.size() > 1 ? (argv[1] != 0) : false;
            float nominalM = argv.size() > 2 ? static_cast<float>(argv[2]) : 200.0f;
            return analyzer_.runPetsysProcessTdcCalibration(
                config.getConfigFilePath(),
                config.getFileStem("main", "tdc_calibration_table"),
                config.getFileStem("main", "tdc_calibration_table"),
                config.getFileStem("main", "tdc_calibration_table"),
                doSorting,
                keepTmp,
                nominalM
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_PROCESS_TDC_CALIBRATION");
            return false;
        }
    };

    // RUN_PROCESS_QDC_CALIBRATION
    table_[TOFCommandCode::RUN_PROCESS_QDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Running QDC calibration...");
            bool doSorting = argv.size() > 0 ? (argv[0] != 0) : true;
            bool keepTmp   = argv.size() > 1 ? (argv[1] != 0) : false;
            float nominalM = argv.size() > 2 ? static_cast<float>(argv[2]) : 200.0f;
            return analyzer_.runPetsysProcessQdcCalibration(
                config.getConfigFilePath(),
                config.getFileStem("main", "qdc_calibration_table"),
                config.getFileStem("main", "qdc_calibration_table"),
                config.getFileStem("main", "qdc_calibration_table"),
                doSorting,
                keepTmp,
                nominalM
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_PROCESS_QDC_CALIBRATION");
            return false;
        }
    };

    // RUN_CONVERT_RAW_TO_SINGLES
    table_[TOFCommandCode::RUN_CONVERT_RAW_TO_SINGLES] = [&](const std::vector<int>& argv) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Converting raw to singles...");
            PETSYS::FILE_TYPE fileType = argv.size() > 0 ? static_cast<PETSYS::FILE_TYPE>(argv[0]) : PETSYS::FILE_ROOT;
            long long eventFractionToWrite = argv.size() > 1 ? static_cast<long long>(argv[1]) : 1024;
            double fileSplitTime = argv.size() > 2 ? static_cast<double>(argv[2]) : 0.0;

            return analyzer_.runPetsysConvertRawToSingles(
                config.getConfigFilePath(),
                "run_test",
                "run_test_single.root",
                fileType,
                eventFractionToWrite,
                fileSplitTime
            );
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in RUN_CONVERT_RAW_TO_SINGLES");
            return false;
        }
    };

    // DUMMY_TEST
    table_[TOFCommandCode::DUMMY_TEST] = [&](const std::vector<int>&) {
        try {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Handling dummy command for testing");
    
            // Optional: simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
            return true; 
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in DUMMY_TEST");
            return false; // signal failure
        }
    };
}

GRAMS_TOF_CommandDispatch::~GRAMS_TOF_CommandDispatch() {
    try {
        if (daqRunning_) {
            pyint_.getDAQ().stop();
            if (daqThread_.joinable()) {
                daqThread_.join();
            }
        }
    } catch (...) {
        Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in destructor");
    }
}

void GRAMS_TOF_CommandDispatch::runDAQThread() {
    try {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] DAQ thread started");
        pyint_.getDAQ().run();
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] DAQ thread finished");
    } catch (...) {
        Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in DAQ thread");
    }
    std::lock_guard<std::mutex> lock(daqMutex_);
    daqRunning_ = false;
}

bool GRAMS_TOF_CommandDispatch::dispatch(TOFCommandCode code, const std::vector<int>& argv) {
    auto it = table_.find(code);
    if (it != table_.end()) {
        try {
            return it->second(argv);
        } catch (...) {
            Logger::instance().error("[GRAMS_TOF_CommandDispatch] Exception in dispatch for command {}", static_cast<int>(code));
            return false;
        }
    }
    Logger::instance().error("[GRAMS_TOF_CommandDispatch] Unknown command code: {}", static_cast<int>(code));
    return false;
}

