#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_PythonIntegration.h"
#include "GRAMS_TOF_Analyzer.h"
#include "GRAMS_TOF_CommandServer.h"
#include "GRAMS_TOF_EventServer.h"
#include "GRAMS_TOF_CommandDefs.h"
#include "GRAMS_TOF_CommandDispatch.h"
#include "GRAMS_TOF_Logger.h"
#include "GRAMS_TOF_Config.h"
#include "GRAMS_TOF_FDManager.h"
#include "CLI11.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

// Ensure stdin/stdout/stderr are valid
void ensureStandardFDs() {
    for (int fd = 0; fd <= 2; ++fd) {
        if (fcntl(fd, F_GETFD) == -1 && errno == EBADF) {
            int newfd = open("/dev/null", O_RDWR);
            if (newfd != fd) {
                dup2(newfd, fd);
                close(newfd);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    ensureStandardFDs();
    signal(SIGPIPE, SIG_IGN);  // Ignore broken pipe signals
    CLI::App app{"GRAMS TOF DAQ Server"};

    bool noFpgaMode = false;
    // --- PORTS MODIFIED TO MATCH network.cfg [TOF] ---
    int serverPort = 50007; // Command server port (comport)
    int eventPort  = 50006; // Event server port (telport: CALLBACK/HEART_BEAT)
    // -------------------------------------------------

    app.add_flag("--no-fpga", noFpgaMode, "Skip DAQ initialization for testing without FPGA");
    app.add_option("--server-port", serverPort, "Command server port");
    app.add_option("--event-port", eventPort, "Event server port (CALLBACK/HEART_BEAT)");

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

    // Event server
    GRAMS_TOF_EventServer eventServer(eventPort);
    eventServer.start();

    // Command server
    GRAMS_TOF_CommandServer server(
        serverPort,
        [&](const GRAMS_TOF_CommandCodec::Packet& pkt) {
            TOFCommandCode code = tof_bridge::toTOFCommand(
                static_cast<pgrams::communication::CommunicationCodes>(pkt.code)
            );

            const auto& argv = pkt.argv;
            if (!dispatchTable.dispatch(code, argv)) {
                Logger::instance().error("[CommandServer] Command failed or unknown: {}", code);
            }

            // Send CALLBACK after each command
            if (eventServer.sendCallback({static_cast<int32_t>(code)})) {
                Logger::instance().info("[EventServer] CALLBACK sent for command 0x{:04X}", static_cast<int>(code));
            } else {
                Logger::instance().warn("[EventServer] Failed to send CALLBACK for command 0x{:04X}", static_cast<int>(code));
            }
        }
    );

    server.start();

    Logger::instance().info("[System] Running DAQ and waiting for commands on port {}", serverPort);
    Logger::instance().info("[System] Event server running on port {}", eventPort);
    Logger::instance().info("[System] Press Enter to quit");
    std::cin.get();

    // Shutdown
    server.stop();
    eventServer.stop();

    Logger::instance().info("[System] Exiting");
    return 0;
}

