#include "GRAMS_TOF_DAQController.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <cstring>

namespace {
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
}

// Helper to initialize logging and configuration
void GRAMS_TOF_DAQController::setupSystemFiles() {
    ensureStandardFDs();
    signal(SIGPIPE, SIG_IGN);
    Logger::instance().setLogFile(config_.logFile);
    try {
        GRAMS_TOF_Config::instance().setConfigFile(config_.configFile);
    } catch (const std::exception& e) {
        Logger::instance().error("[System] Config load error: {}", e.what());
        throw; // Re-throw to indicate fatal configuration error
    }
}

// Constructor Implementation
GRAMS_TOF_DAQController::GRAMS_TOF_DAQController(const Config& config)
    : config_(config),
      daq_("/tmp/d.sock", "/daqd_shm", 0, "GBE", {"/dev/psdaq0"}),
      pyint_(daq_),
      analyzer_(),
      dispatchTable_(pyint_, analyzer_)
{
    setupSystemFiles();
    setenv("DEBUG", "1", 1);
}

// Initialization Implementation (Hardware Setup)
bool GRAMS_TOF_DAQController::initialize() {
    Logger::instance().info("[System] Initializing DAQ with Config: IP={}, CP={}, EP={}, NoFPGA={}",
                            config_.remoteEventHub, config_.commandListenPort,
                            config_.eventTargetPort, config_.noFpgaMode);

    if (!config_.noFpgaMode) {
        if (!daq_.initialize()) {
            Logger::instance().error("[System] DAQ initialization failed.");
            return false;
        }
    } else {
        Logger::instance().info("[System] Running in no-FPGA mode (DAQ init skipped)");
    }

    eventClient_ = std::make_unique<GRAMS_TOF_EventClient>(
        config_.remoteEventHub,
        config_.eventTargetPort,
        [](const GRAMS_TOF_CommandCodec::Packet& pkt) {
            Logger::instance().info("[EventClient] Received Event Packet Code: 0x{:04X}", static_cast<int>(pkt.code));
        }
    );

    commandClient_ = std::make_unique<GRAMS_TOF_CommandClient>(
        config_.remoteCommandHub,    
        config_.commandListenPort,
        std::bind(&GRAMS_TOF_DAQController::handleIncomingCommand, this, std::placeholders::_1)
    );

    // Start network services
    eventClient_->start();
    commandClient_->start();

    Logger::instance().info("[System] Event service (CLIENT) sending to {}:{}", config_.remoteEventHub, config_.eventTargetPort);
    Logger::instance().info("[System] Command service (SERVER) listening on port {}", config_.commandListenPort);

    return true;
}

void GRAMS_TOF_DAQController::handleIncomingCommand(const GRAMS_TOF_CommandCodec::Packet& pkt) {
    TOFCommandCode code = tof_bridge::toTOFCommand(
        static_cast<pgrams::communication::CommunicationCodes>(pkt.code)
    );

    const auto& argv = pkt.argv;
    if (!dispatchTable_.dispatch(code, argv)) {
        Logger::instance().error("[CommandClient] Command failed or unknown: 0x{:04X}", static_cast<int>(code));
    }

    // --- CORE LOGIC: Send the CALLBACK event using the Event Client ---
    GRAMS_TOF_CommandCodec::Packet callbackPkt;
    callbackPkt.code = static_cast<uint16_t>(pgrams::communication::CommunicationCodes::TOF_Callback);
    callbackPkt.argv.push_back(static_cast<int32_t>(code));
    callbackPkt.argc = callbackPkt.argv.size();

    if (eventClient_->sendPacket(callbackPkt)) {
        Logger::instance().info("[EventClient] CALLBACK sent for command 0x{:04X} to {}",
                                static_cast<int>(code), config_.remoteEventHub);
    } else {
        Logger::instance().warn("[EventClient] Failed to send CALLBACK for command 0x{:04X}",
                                static_cast<int>(code));
    }
}

void GRAMS_TOF_DAQController::run() {
    Logger::instance().info("[System] GRAMS_TOF_DAQController running...");

    while (keepRunning_) {
        // Sleep briefly to avoid busy-waiting and allow other threads to run
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    commandClient_->stop();
    eventClient_->stop();

    Logger::instance().info("[System] GRAMS_TOF_DAQController exited run loop and shut down services.");
}

void GRAMS_TOF_DAQController::stop() {
    Logger::instance().info("[System] Received STOP signal. Shutting down TOF DAQ...");
    keepRunning_ = false;
}
