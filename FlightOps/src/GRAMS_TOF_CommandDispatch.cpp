#include "GRAMS_TOF_CommandDispatch.h"
#include "GRAMS_TOF_Logger.h"
#include "GRAMS_TOF_Config.h"

GRAMS_TOF_CommandDispatch::GRAMS_TOF_CommandDispatch(GRAMS_TOF_PythonIntegration& pyint, GRAMS_TOF_Analyzer& analyzer)
    : pyint_(pyint), analyzer_(analyzer), table_{} {

    auto& config = GRAMS_TOF_Config::instance();

    table_[TOFCommandCode::START_DAQ] = [&](const std::vector<int>&) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Starting DAQ...");

        std::lock_guard<std::mutex> lock(daqMutex_);
        if (daqRunning_) {
            Logger::instance().warn("[GRAMS_TOF_CommandDispatch] DAQ is already running.");
            return false;
        }

        daqRunning_ = true;
        daqThread_ = std::thread(&GRAMS_TOF_CommandDispatch::runDAQThread, this);
        return true;
    };

    table_[TOFCommandCode::STOP_DAQ] = [&](const std::vector<int>&) {
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
    };

    table_[TOFCommandCode::RESET_DAQ] = [&](const std::vector<int>&) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Resetting DAQ...");
        pyint_.getDAQ().reset();
        return true;
    };

    table_[TOFCommandCode::RUN_INIT_SYSTEM] = [&](const std::vector<int>&) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing init_system.py script...");
        return pyint.runPetsysInitSystem("scripts.init_system"); // module name
    };

    table_[TOFCommandCode::RUN_MAKE_BIAS_CALIB_TABLE] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_bias_calibration_table.py script...");
        return pyint.runPetsysMakeBiasCalibrationTable(
            "scripts.make_bias_calibration_table",              // module name
            config.getString("main", "bias_calibration_table"), // output file
            {argv.size() > 0 ? argv[0] : 0},                    // portIDs.
            {argv.size() > 1 ? argv[1] : 0},                    // slaveIDs
            {argv.size() > 2 ? argv[2] : 0},                    // slotIDs
            {}                                                  // no calibration files
        );
    };

    table_[TOFCommandCode::RUN_MAKE_SIMPLE_BIAS_SET_TABLE] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_simple_bias_settings_table.py script...");
        return pyint.runPetsysMakeSimpleBiasSettingsTable(
            "scripts.make_simple_bias_settings_table",             // module name
            config.getConfigFilePath(),                            // Configuration file
            argv.size() > 0 ? static_cast<float>(argv[0]) : 0.75,  // Bias channel offset
            argv.size() > 1 ? static_cast<float>(argv[1]) : 20,    // Pre-breakdown voltage
            argv.size() > 2 ? static_cast<float>(argv[2]) : 24.9,  // Breakdown voltage
            argv.size() > 3 ? static_cast<float>(argv[3]) : 5.0,   // Nominal overvoltage
            config.getString("main", "bias_settings_table")        // Output file  
        );
    };

    table_[TOFCommandCode::RUN_MAKE_SIMPLE_CHANNEL_MAP] = [&](const std::vector<int>&) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_simple_channel_map.py script...");
        return pyint.runPetsysMakeSimpleChannelMap(
            "scripts.make_simple_channel_map",       // module name
            config.getString("main", "channel_map")  // Output file
        );
    };

    table_[TOFCommandCode::RUN_MAKE_SIMPLE_DISC_SET_TABLE] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_simple_disc_settings_table.py script...");
        return pyint.runPetsysMakeSimpleDiscSettingsTable(
            "scripts.make_simple_disc_settings_table",         // module name
            config.getConfigFilePath(),                        // Configuration file
            argv.size() > 0 ? argv[0] : 20,                    // Discriminator T1 (DAC above zero)
            argv.size() > 1 ? argv[1] : 20,                    // Discriminator T2 (DAC above zero)
            argv.size() > 2 ? argv[2] : 15,                    // Discriminator E (DAC above zero)
            config.getString("main", "disc_calibration_table") // Output file
        );
    };

    table_[TOFCommandCode::RUN_READ_TEMPERATURE_SENSORS] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing read_temperature_sensors.py script...");
        return pyint.runPetsysReadTemperatureSensors(
            "scripts.read_temperature_sensors",                    // module name
            argv.size() > 0 ? static_cast<double>(argv[0]) : 0.0,  // Acquisition time (in seconds)
            argv.size() > 1 ? static_cast<double>(argv[1]) : 60.0, // Measurement interval (in seconds)
            "/dev/null",                                           // Output file
            argv.size() > 2 ? static_cast<bool>(argv[2]) : true,   // Check temperature stability when starting up
            argv.size() > 3 ? static_cast<bool>(argv[3]) : false   // Enable debug mode
        );
    };
    
    table_[TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_threshold_calibration.py script...");
        return pyint.runPetsysAcquireThresholdCalibration(
            "scripts.acquire_threshold_calibration",              // module name
            config.getConfigFilePath(),                           // Configuration file 
            config.getFileStem("main", "disc_calibration_table"), // Data file prefix
            argv.size() > 0 ? argv[0] : 4,                        // noise_reads
            argv.size() > 1 ? argv[1] : 4,                        // dark_reads
            argv.size() > 2 ? static_cast<bool>(argv[2]) : false  // Prompt user to set bias
        );
    };
    
    table_[TOFCommandCode::RUN_ACQUIRE_QDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_qdc_calibration.py script...");
        return pyint.runPetsysAcquireQdcCalibration(
            "scripts.acquire_qdc_calibration",                  // module name
            config.getConfigFilePath(),                         // Configuration file 
            config.getFileStem("main", "qdc_calibration_table") // Data filename (prefix)
        );
    };

    table_[TOFCommandCode::RUN_ACQUIRE_TDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_tdc_calibration.py script...");
        return pyint.runPetsysAcquireTdcCalibration(
            "scripts.acquire_tdc_calibration",                  // module name
            config.getConfigFilePath(),                         // Configuration file 
            config.getFileStem("main", "tdc_calibration_table") // Data filename (prefix)
        );
    };
    
    table_[TOFCommandCode::RUN_ACQUIRE_SIPM_DATA] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_sipm_data.py script...");
        return pyint.runPetsysAcquireSipmData(
            "scripts.acquire_sipm_data",                           // module name
            config.getConfigFilePath(),                            // Configuration file              
            "run_test",                                            // Data filename (prefix) will be determined by Hyebin
            argv.size() > 0 ? static_cast<double>(argv[0]) : 60.0, // Acquisition time (in seconds) 
            "qdc",                                                 // Acquisition mode (tot, qdc or mixed) 
            argv.size() > 1 ? static_cast<bool>(argv[1]) : false,  // Enable the hardware coincidence filter
            ""                                                     // Optional scan table with 1 or 2 parameters to vary 
        );
    };

    table_[TOFCommandCode::RUN_PROCESS_THRESHOLD_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Running threshold calibration...");
    
        std::string configFile = config.getConfigFilePath();                           // Configuration file
        std::string input      = config.getFileStem("main", "disc_calibration_table"); // Data file prefix
        std::string output     = config.getString("main", "disc_calibration_table");   // Output table file name
        std::string rootOutput = "disc_calibration.root";                              // Output ROOT file name
    
        return analyzer_.runPetsysProcessThresholdCalibration(configFile, input, output, rootOutput);
    };

    table_[TOFCommandCode::RUN_PROCESS_TDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Running TDC calibration...");
    
        std::string configFile   = config.getConfigFilePath();                          // Configuration file
        std::string inputPrefix  = config.getFileStem("main", "tdc_calibration_table"); // Data file prefix
        std::string outputPrefix = config.getFileStem("main", "tdc_calibration_table"); // Output table file prefix
        std::string tmpPrefix    = config.getFileStem("main", "tdc_calibration_table"); // Tmp prefix (=Output ?)
        bool doSorting    = argv.size() > 0 ? (argv[0] != 0) : true;                    // Do sorting
        bool keepTemporary= argv.size() > 1 ? (argv[1] != 0) : false;                   // Keep Temporary
        float nominalM    = argv.size() > 2 ? static_cast<float>(argv[2]) : 200.0f;     // Nominal M
    
        return analyzer_.runPetsysProcessTdcCalibration(
            configFile, inputPrefix, outputPrefix, tmpPrefix, doSorting, keepTemporary, nominalM);
    };
    
    table_[TOFCommandCode::RUN_PROCESS_QDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Running QDC calibration...");
    
        std::string configFile   = config.getConfigFilePath();                          // Configuration file
        std::string inputPrefix  = config.getFileStem("main", "qdc_calibration_table"); // Data file prefix  
        std::string outputPrefix = config.getFileStem("main", "qdc_calibration_table"); // Output table file prefix
        std::string tmpPrefix    = config.getFileStem("main", "qdc_calibration_table"); // Tmp prefix (=Output ?)
        bool doSorting    = argv.size() > 0 ? (argv[0] != 0) : true;                    // Do sorting
        bool keepTemporary= argv.size() > 1 ? (argv[1] != 0) : false;                   // Keep Temporary
        float nominalM    = argv.size() > 2 ? static_cast<float>(argv[2]) : 200.0f;     // Nominal M
    
        return analyzer_.runPetsysProcessQdcCalibration(
            configFile, inputPrefix, outputPrefix, tmpPrefix, doSorting, keepTemporary, nominalM);
    };
    
    table_[TOFCommandCode::RUN_CONVERT_RAW_TO_SINGLES] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Converting raw to singles...");
    
        std::string configFile   = config.getConfigFilePath();                          // Configuration file
        std::string inputPrefix  = "run_test";                                          // Data file prefix
        std::string outputFile   = inputPrefix + "_single.root";                        // Output file
    
        // Map FILE_TYPE enum to int argument; default to PETSYS::FILE_TEXT = 0 if not passed
        PETSYS::FILE_TYPE fileType = (argv.size() > 0) ? static_cast<PETSYS::FILE_TYPE>(argv[0]) : PETSYS::FILE_ROOT;
    
        // eventFractionToWrite, default 1024
        long long eventFractionToWrite = (argv.size() > 1) ? static_cast<long long>(argv[1]) : 1024; 
    
        // fileSplitTime as double; take int argument and convert, default 0.0
        double fileSplitTime = (argv.size() > 2) ? static_cast<double>(argv[2]) : 0.0;
    
        return analyzer_.runPetsysConvertRawToSingles(
            configFile, inputPrefix, outputFile, fileType, eventFractionToWrite, fileSplitTime);
    };
}

GRAMS_TOF_CommandDispatch::~GRAMS_TOF_CommandDispatch() {
    if (daqRunning_) {
        pyint_.getDAQ().stop();
        if (daqThread_.joinable()) {
            daqThread_.join();
        }
    }
}

void GRAMS_TOF_CommandDispatch::runDAQThread() {
    pyint_.getDAQ().run();

    // Once run() returns, mark as not running
    std::lock_guard<std::mutex> lock(daqMutex_);
    daqRunning_ = false;
}

bool GRAMS_TOF_CommandDispatch::dispatch(TOFCommandCode code, const std::vector<int>& argv) {
    auto it = table_.find(code);
    if (it != table_.end()) {
        return it->second(argv);
    }
    Logger::instance().error("[GRAMS_TOF_CommandDispatch] Unknown command code: {}", static_cast<int>(code));
    return false;
}

