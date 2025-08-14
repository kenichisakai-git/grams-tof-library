#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_PythonIntegration.h"
#include "GRAMS_TOF_Analyzer.h"
#include "GRAMS_TOF_CommandServer.h"
#include "GRAMS_TOF_CommandDefs.h"
#include "GRAMS_TOF_CommandDispatch.h"
#include "GRAMS_TOF_Logger.h"
#include "GRAMS_TOF_Config.h"
#include "CLI11.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    CLI::App app{"GRAMS TOF DAQ Server"};

    // CLI11 options
    bool noFpgaMode = false;
    int serverPort = 12345;
    
    app.add_flag("--no-fpga", noFpgaMode, "Skip DAQ initialization for testing without FPGA");
    app.add_option("--port", serverPort, "Server port");

    CLI11_PARSE(app, argc, argv);

    // Logger
    Logger::instance().setLogFile("log/daq_log.txt");

    // Config
    try {
        GRAMS_TOF_Config::instance().setConfigFile("config/config.ini");
    } catch (const std::exception& e) {
        Logger::instance().error("[System] Config load error: {}", e.what());
        return 1;
    }

    // DAQ manager
    GRAMS_TOF_DAQManager daq(
        "/tmp/d.sock",        // socketPath
        "/daqd_shm",          // shmName
        0,                    // debugLevel
        "GBE",                // daqType
        {"/dev/psdaq0"}       // daqCards
    );

    // Initialize DAQ if not skipped
    if (!noFpgaMode) {
        if (!daq.initialize()) {
            Logger::instance().info("[System] DAQ initialization failed.");
            return 1;
        }
    } else {
        Logger::instance().info("[System] Running in no-FPGA mode (DAQ init skipped)");
    }

    setenv("DEBUG", "1", 1);

    // Python integration & analysis
    GRAMS_TOF_PythonIntegration pyint(daq);
    GRAMS_TOF_Analyzer analyzer;
    GRAMS_TOF_CommandDispatch dispatchTable(pyint, analyzer);

    // Command server
    GRAMS_TOF_CommandServer server(
        serverPort,
        [&](const GRAMS_TOF_CommandCodec::Packet& pkt) {
            TOFCommandCode code = pkt.code;
            const auto& argv = pkt.argv;

            if (!dispatchTable.dispatch(code, argv)) {
                Logger::instance().error("[CommandServer] Command failed or unknown: {}", code);
            }
        }
    );

    server.start();

    Logger::instance().info("[System] Running DAQ and waiting for commands on port {}", serverPort);
    Logger::instance().info("[System] Press Enter to quit");
    std::cin.get();

    server.stop();
    Logger::instance().info("[System] Exiting");

    return 0;
}

