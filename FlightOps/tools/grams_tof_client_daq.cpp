#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_PythonIntegration.h"
#include "GRAMS_TOF_Analyzer.h"
#include "GRAMS_TOF_CommandClient.h"
#include "GRAMS_TOF_EventClient.h" 
#include "GRAMS_TOF_CommandDefs.h"       
#include "GRAMS_TOF_CommandDispatch.h"
#include "GRAMS_TOF_Logger.h"
#include "GRAMS_TOF_Config.h"
#include "GRAMS_TOF_FDManager.h"
#include "CLI11.hpp"
#include "GRAMS_TOF_CommandCodec.h"      

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <exception>

// Ensure stdin/stdout/stderr are valid (a common practice for service-style applications)
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

// --- MAIN APPLICATION START ---

int main(int argc, char* argv[]) {
    ensureStandardFDs();
    signal(SIGPIPE, SIG_IGN);
    
    CLI::App app{"GRAMS TOF DAQ Core Application (Client Host)"};

    bool noFpgaMode = false;
    int commandListenPort = 50007; // Port the DAQ *listens* for commands on (acts as server)
    int eventTargetPort   = 50006; // Port the DAQ *sends* events to (acts as client)
    std::string remoteEventHub = "127.0.0.1"; // Default remote IP for the Event Server

    app.add_flag("--no-fpga", noFpgaMode, "Skip DAQ initialization for testing without FPGA");
    app.add_option("--command-port", commandListenPort, "Command service port (Listening)");
    app.add_option("--event-port", eventTargetPort, "Remote Event Server port (Sending)");
    app.add_option("--event-ip", remoteEventHub, "Remote Event Server IP address");

    CLI11_PARSE(app, argc, argv);

    Logger::instance().setLogFile("log/daq_log.txt");

    try {
        GRAMS_TOF_Config::instance().setConfigFile("config/config.ini");
    } catch (const std::exception& e) {
        Logger::instance().error("[System] Config load error: {}", e.what());
        return 1;
    }

    // 1. DAQ manager (Core hardware/data functionality)
    GRAMS_TOF_DAQManager daq(
        "/tmp/d.sock",             // socketPath
        "/daqd_shm",               // shmName
        0,                         // debugLevel
        "GBE",                     // daqType
        {"/dev/psdaq0"}            // daqCards
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

    // Command/analysis components
    GRAMS_TOF_PythonIntegration pyint(daq);
    GRAMS_TOF_Analyzer analyzer;
    GRAMS_TOF_CommandDispatch dispatchTable(pyint, analyzer);
    
    const std::string local_host_ip = "127.0.0.1"; 

    // 2. Event Client (This component connects to a remote server and sends events)
    // It assumes a minimal handler for receiving any remote responses (e.g., acknowledgements)
    GRAMS_TOF_EventClient eventClient(
        remoteEventHub, 
        eventTargetPort, 
        [](const GRAMS_TOF_CommandCodec::Packet& pkt) { 
            Logger::instance().info("[EventClient] Received Event Packet Code: 0x{:04X}", static_cast<int>(pkt.code));
        }
    );
    eventClient.start();

    // 3. Command Client (This component acts as a server, listening for incoming commands)
    GRAMS_TOF_CommandClient commandClient(
        local_host_ip, 
        commandListenPort,
        // Command Handler Lambda - Captures 'eventClient' by reference [&]
        [&](const GRAMS_TOF_CommandCodec::Packet& pkt) {
            TOFCommandCode code = tof_bridge::toTOFCommand(
                static_cast<pgrams::communication::CommunicationCodes>(pkt.code)
            );

            const auto& argv = pkt.argv;
            if (!dispatchTable.dispatch(code, argv)) {
                Logger::instance().error("[CommandClient] Command failed or unknown: 0x{:04X}", static_cast<int>(code));
            }

            // --- CORE LOGIC: Send the CALLBACK event using the Event Client ---
            
            // 1. Construct the response packet
            GRAMS_TOF_CommandCodec::Packet callbackPkt;
            
            // CORRECTED: Using TOF_Callback based on the provided header information 
            // (TOFCommandCode::CALLBACK -> CommunicationCodes::TOF_Callback)
            callbackPkt.code = static_cast<uint16_t>(pgrams::communication::CommunicationCodes::TOF_Callback); 
            
            // The argument is the command code that was just executed
            callbackPkt.argv.push_back(static_cast<int32_t>(code));
            callbackPkt.argc = callbackPkt.argv.size();

            // 2. Send the packet using the Event Client
            if (eventClient.sendPacket(callbackPkt)) {
                Logger::instance().info("[EventClient] CALLBACK sent for command 0x{:04X} to {}", static_cast<int>(code), remoteEventHub);
            } else {
                Logger::instance().warn("[EventClient] Failed to send CALLBACK for command 0x{:04X}", static_cast<int>(code));
            }
        }
    );

    commandClient.start();

    Logger::instance().info("[System] Running DAQ Core and waiting for commands on port {}", commandListenPort);
    Logger::instance().info("[System] Event service (CLIENT) sending to {}:{}", remoteEventHub, eventTargetPort);
    
    // Blocking call to keep the application running
    Logger::instance().info("[System] Press Enter to quit");
    std::cin.get();

    // Shutdown sequence
    commandClient.stop();
    eventClient.stop(); 

    Logger::instance().info("[System] Exiting");
    return 0;
}
