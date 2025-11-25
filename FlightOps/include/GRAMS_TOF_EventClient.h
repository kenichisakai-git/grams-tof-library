#pragma once

#include "GRAMS_TOF_CommandCodec.h"
#include "GRAMS_TOF_Client.h"
#include "GRAMS_TOF_FDManager.h"

#include <functional>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>

class GRAMS_TOF_EventClient {
public:
    // Handler for incoming event packets (HEART_BEAT, CALLBACK, etc.)
    using PacketHandler = std::function<void(const GRAMS_TOF_CommandCodec::Packet&)>;

    // Client constructor takes Hub IP and Port
    explicit GRAMS_TOF_EventClient(const std::string& hub_ip, int port, PacketHandler handler);
    ~GRAMS_TOF_EventClient();

    void start();
    void stop();

    bool sendPacket(const GRAMS_TOF_CommandCodec::Packet& packet);

    GRAMS_TOF_EventClient(const GRAMS_TOF_EventClient&) = delete;
    GRAMS_TOF_EventClient& operator=(const GRAMS_TOF_EventClient&) = delete;

private:
    void run();  // Main client connection/read loop

    // Configuration
    std::string hub_ip_;
    int port_;
    PacketHandler handler_;

    // Threading and execution control
    std::thread client_thread_;
    std::atomic<bool> running_{false};

    // Connection State (Single connection to the Event Server)
    std::unique_ptr<GRAMS_TOF_Client> hubConnection_;
    std::mutex connectionMutex_; // Protects structural access to hubConnection_
};
