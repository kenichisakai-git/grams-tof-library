#include "GRAMS_TOF_CommandDispatch.h"
#include "GRAMS_TOF_Logger.h"

GRAMS_TOF_CommandDispatch::GRAMS_TOF_CommandDispatch(GRAMS_TOF_PythonIntegration& pyint, GRAMS_TOF_Analyzer& analyzer)
    : pyint_(pyint), analyzer_(analyzer), table_{} {

    table_[TOFCommandCode::START_DAQ] = [&](const std::vector<int>&) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Starting DAQ...");
        pyint_.getDAQ().run();
        return true;
    };
    
    table_[TOFCommandCode::STOP_DAQ] = [&](const std::vector<int>&) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Stopping DAQ...");
        pyint_.getDAQ().stop();
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
            "scripts.make_bias_calibration_table",  // module name
            "/tmp/bias_output.txt",                 // output file
            {argv.size() > 0 ? argv[0] : 0},        // portIDs.
            {argv.size() > 1 ? argv[1] : 0},        // slaveIDs
            {argv.size() > 2 ? argv[2] : 0},        // slotIDs
            {}                                      // no calibration files
        );
    };

    table_[TOFCommandCode::RUN_MAKE_SIMPLE_BIAS_SET_TABLE] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_simple_bias_settings_table.py script...");
        return pyint.runPetsysMakeSimpleBiasSettingsTable(
            "scripts.make_simple_bias_settings_table",             // module name
            "",                                                    // Configuration file
            argv.size() > 0 ? static_cast<float>(argv[0]) : 0.0f,  // Bias channel offset
            argv.size() > 1 ? static_cast<float>(argv[1]) : 0.0f,  // Pre-breakdown voltage
            argv.size() > 2 ? static_cast<float>(argv[2]) : 0.0f,  // Breakdown voltage
            argv.size() > 3 ? static_cast<float>(argv[3]) : 0.0f,  // Nominal overvoltage
            ""                                                     // Output file  
        );
    };

    table_[TOFCommandCode::RUN_MAKE_SIMPLE_CHANNEL_MAP] = [&](const std::vector<int>&) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_simple_channel_map.py script...");
        return pyint.runPetsysMakeSimpleChannelMap(
            "scripts.make_simple_channel_map",  // module name
            ""                                  // Output file
        );
    };

    table_[TOFCommandCode::RUN_MAKE_SIMPLE_DISC_SET_TABLE] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing make_simple_disc_settings_table.py script...");
        return pyint.runPetsysMakeSimpleDiscSettingsTable(
            "scripts.make_simple_disc_settings_table",  // module name
            "",                                         // Configuration file
            argv.size() > 0 ? argv[0] : 0,              // Discriminator T1 (DAC above zero)
            argv.size() > 1 ? argv[1] : 0,              // Discriminator T2 (DAC above zero)
            argv.size() > 2 ? argv[2] : 0,              // Discriminator E (DAC above zero)
            ""                                          // Output file
        );
    };

    table_[TOFCommandCode::RUN_READ_TEMPERATURE_SENSORS] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing read_temperature_sensors.py script...");
        return pyint.runPetsysReadTemperatureSensors(
            "scripts.read_temperature_sensors",
            argv.size() > 0 ? static_cast<double>(argv[0]) : 1.0,
            argv.size() > 1 ? static_cast<double>(argv[1]) : 0.2,
            "",
            argv.size() > 2 ? static_cast<bool>(argv[2]) : false,
            argv.size() > 3 ? static_cast<bool>(argv[3]) : false
        );
    };
    
    table_[TOFCommandCode::RUN_ACQUIRE_THRESHOLD_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_threshold_calibration.py script...");
        return pyint.runPetsysAcquireThresholdCalibration(
            "scripts.acquire_threshold_calibration",
            "",  
            "",  
            argv.size() > 0 ? argv[0] : 1,
            argv.size() > 1 ? argv[1] : 1,
            argv.size() > 2 ? static_cast<bool>(argv[2]) : false
        );
    };
    
    table_[TOFCommandCode::RUN_ACQUIRE_QDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_qdc_calibration.py script...");
        return pyint.runPetsysAcquireQdcCalibration(
            "scripts.acquire_qdc_calibration",
            "",  
            ""   
        );
    };

    table_[TOFCommandCode::RUN_ACQUIRE_TDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_tdc_calibration.py script...");
        return pyint.runPetsysAcquireTdcCalibration(
            "scripts.acquire_tdc_calibration",
            "", 
            ""  
        );
    };
    
    table_[TOFCommandCode::RUN_ACQUIRE_SIPM_DATA] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Executing acquire_sipm_data.py script...");
        return pyint.runPetsysAcquireSipmData(
            "scripts.acquire_sipm_data",
            "",                                      
            "",                                     
            argv.size() > 0 ? static_cast<double>(argv[0]) : 1.0,  // acquisitionTime (float/double)
            "tot",                                    
            argv.size() > 1 ? static_cast<bool>(argv[1]) : false, // hwTrigger (bool)
            ""                                          
        );
    };

    table_[TOFCommandCode::RUN_PROCESS_THRESHOLD_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Running threshold calibration...");
    
        std::string config     = "config.tsv";
        std::string input      = "input_prefix";
        std::string output     = "output.tsv";
        std::string rootOutput = "output.root";
    
        return analyzer_.runPetsysProcessThresholdCalibration(config, input, output, rootOutput);
    };

    table_[TOFCommandCode::RUN_PROCESS_TDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Running TDC calibration...");
    
        std::string config       = "tdc_config.tsv";
        std::string inputPrefix  = "tdc_input_prefix";
        std::string outputPrefix = "tdc_output_prefix";
        std::string tmpPrefix    = "tdc_tmp_prefix";
    
        bool doSorting    = argv.size() > 0 ? (argv[0] != 0) : true;
        bool keepTemporary= argv.size() > 1 ? (argv[1] != 0) : false;
        float nominalM    = argv.size() > 2 ? static_cast<float>(argv[2]) : 200.0f;
    
        return analyzer_.runPetsysProcessTdcCalibration(
            config, inputPrefix, outputPrefix, tmpPrefix, doSorting, keepTemporary, nominalM);
    };
    
    table_[TOFCommandCode::RUN_PROCESS_QDC_CALIBRATION] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Running QDC calibration...");
    
        std::string config       = "qdc_config.tsv";
        std::string inputPrefix  = "qdc_input_prefix";
        std::string outputPrefix = "qdc_output_prefix";
        std::string tmpPrefix    = "qdc_tmp_prefix";
    
        bool doSorting    = argv.size() > 0 ? (argv[0] != 0) : true;
        bool keepTemporary= argv.size() > 1 ? (argv[1] != 0) : false;
        float nominalM    = argv.size() > 2 ? static_cast<float>(argv[2]) : 200.0f;
    
        return analyzer_.runPetsysProcessQdcCalibration(
            config, inputPrefix, outputPrefix, tmpPrefix, doSorting, keepTemporary, nominalM);
    };
    
    table_[TOFCommandCode::RUN_CONVERT_RAW_TO_SINGLES] = [&](const std::vector<int>& argv) {
        Logger::instance().warn("[GRAMS_TOF_CommandDispatch] Converting raw to singles...");
    
        std::string config       = "convert_config.tsv";
        std::string inputPrefix  = "convert_input_prefix";
        std::string outputFile   = "convert_output.txt";
    
        // Map FILE_TYPE enum to int argument; default to PETSYS::FILE_TEXT = 0 if not passed
        PETSYS::FILE_TYPE fileType = (argv.size() > 0) ? static_cast<PETSYS::FILE_TYPE>(argv[0]) : PETSYS::FILE_TEXT;
    
        // eventFractionToWrite, default 1024
        long long eventFractionToWrite = (argv.size() > 1) ? static_cast<long long>(argv[1]) : 1024;
    
        // fileSplitTime as double; take int argument and convert, default 0.0
        double fileSplitTime = (argv.size() > 2) ? static_cast<double>(argv[2]) : 0.0;
    
        return analyzer_.runPetsysConvertRawToSingles(
            config, inputPrefix, outputFile, fileType, eventFractionToWrite, fileSplitTime);
    };
}

bool GRAMS_TOF_CommandDispatch::dispatch(TOFCommandCode code, const std::vector<int>& argv) const {
    auto it = table_.find(code);
    if (it != table_.end()) {
        return it->second(argv);
    }
    Logger::instance().error("[GRAMS_TOF_CommandDispatch] Unknown command code: {}", static_cast<int>(code));
    return false;
}

