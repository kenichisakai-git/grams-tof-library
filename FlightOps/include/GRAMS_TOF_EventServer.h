#pragma once

#include "GRAMS_TOF_CommandCodec.h"
#include "GRAMS_TOF_Logger.h"
#include "GRAMS_TOF_FDManager.h"

#include <thread>
#include <atomic>
#include <map>
#include <memory>
#include <vector>
#include <mutex>

class GRAMS_TOF_Client; // forward declaration

class GRAMS_TOF_EventServer {
public:
    explicit GRAMS_TOF_EventServer(int port);
    ~GRAMS_TOF_EventServer();

    void start();
    void stop();

    bool sendCallback(const std::vector<int32_t>& args);
    bool sendHeartbeat();

private:
    void run();               // Accept clients and handle events
    void heartbeatLoop();     // Internal heartbeat loop
    bool send(const GRAMS_TOF_CommandCodec::Packet& pkt);

    int port_;
    std::thread server_thread_;
    std::thread heartbeat_thread_;
    std::atomic<bool> running_{false};

    std::map<int, std::unique_ptr<GRAMS_TOF_Client>> clientList_;
    std::mutex clientListMutex_;  // only needed for adding/removing clients
};

