#pragma once

#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_PythonIntegration.h"
#include "GRAMS_TOF_Analyzer.h"
#include "GRAMS_TOF_CommandClient.h"
#include "GRAMS_TOF_EventClient.h"
#include "GRAMS_TOF_CommandDispatch.h"
#include "GRAMS_TOF_Logger.h"
#include "GRAMS_TOF_Config.h"
#include "GRAMS_TOF_CommandCodec.h"

#include <string>
#include <atomic>
#include <memory> 

class GRAMS_TOF_DAQController {
public:
    // Structure to hold the configuration passed from the command line
    struct Config {
        bool noFpgaMode = false;
        int commandListenPort = 50007;
        int eventTargetPort = 50006;
        std::string remoteEventHub = "127.0.0.1";
        std::string remoteCommandHub = "127.0.0.1";
        //std::string configFile = "/home/ksakai/work/source/grams-tof-library/00build/00install/config/config.ini"; 
        std::string configFile = ""; 
        std::string logFile = "log/daq_log.txt";
    };

    GRAMS_TOF_DAQController(const Config& config);
    bool initialize();
    void run();
    void stop();

private:
    Config config_;

    // Thread-safe flag to signal the run loop to terminate
    std::atomic<bool> keepRunning_ = true;

    // 1. Core Hardware/Data Manager
    GRAMS_TOF_DAQManager daq_;

    // 2. Command/Analysis Components
    GRAMS_TOF_PythonIntegration pyint_;
    GRAMS_TOF_Analyzer analyzer_;
    GRAMS_TOF_CommandDispatch dispatchTable_;

    // 3. Networking Clients
    std::unique_ptr<GRAMS_TOF_EventClient> eventClient_;
    std::unique_ptr<GRAMS_TOF_CommandClient> commandClient_;

    // Helper to initialize logging and configuration
    void setupSystemFiles();

    // Helper for the command handler logic
    void handleIncomingCommand(const GRAMS_TOF_CommandCodec::Packet& pkt);
};

