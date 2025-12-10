#include "GRAMS_TOF_DAQController.h"
#include "CLI11.hpp" 
#include "GRAMS_TOF_Logger.h" 
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {

    CLI::App app{"GRAMS TOF DAQ Core Application (Client Host)"};
    GRAMS_TOF_DAQController::Config config;

    app.add_flag("--no-fpga", config.noFpgaMode, "Skip DAQ initialization for testing without FPGA");
    app.add_option("--command-port", config.commandListenPort, "Command service port (Listening)");
    app.add_option("--event-port", config.eventTargetPort, "Remote Event Server port (Sending)");
    app.add_option("--event-ip", config.remoteEventHub, "Remote Event Server IP address");
    app.add_option("--config-file", config.configFile, "Path to the config.ini file");
    app.add_option("--log-file", config.logFile, "Path to the DAQ log file");

    try {
        CLI11_PARSE(app, argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    if (config.configFile.empty()) {
        if (!GRAMS_TOF_Config::loadDefaultConfig()) {
            throw std::runtime_error("Configuration file not specified and default GLIB path failed to load.");
        } else config.configFile =  GRAMS_TOF_Config::instance().getConfigFilePath();
    }

    std::unique_ptr<GRAMS_TOF_DAQController> daqController;
    try {
        daqController = std::make_unique<GRAMS_TOF_DAQController>(config);
        if (!daqController->initialize()) return 1;

        Logger::instance().info("[System] Press Enter to quit");

        std::thread runThread([&](){ daqController->run(); });
        std::cin.get(); // Blocking wait for user input to trigger shutdown
        
        // 4. Shutdown
        daqController->stop();
        if (runThread.joinable()) {
            runThread.join();
        }

    } catch (const std::exception& e) {
        // We use Logger::instance() which should be set up by the constructor
        Logger::instance().error("[System] Fatal Error during setup/run: {}", e.what());
        return 1;
    } catch (...) {
        Logger::instance().error("[System] Unknown Fatal Error.");
        return 1;
    }

    Logger::instance().info("[System] Exiting");
    return 0;
}

