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

class GRAMS_TOF_CommandClient {
public:
    using CommandHandler = std::function<void(const GRAMS_TOF_CommandCodec::Packet&)>;

    // Client constructor takes IP and Port
    explicit GRAMS_TOF_CommandClient(const std::string& hub_ip, int port, CommandHandler handler);
    ~GRAMS_TOF_CommandClient();

    void start();
    void stop();

    GRAMS_TOF_CommandClient(const GRAMS_TOF_CommandClient&) = delete;
    GRAMS_TOF_CommandClient& operator=(const GRAMS_TOF_CommandClient&) = delete;

private:
    void run();  // Main client connection/read loop

    // Configuration
    std::string hub_ip_;
    int port_;
    CommandHandler handler_;

    // Threading and execution control
    std::thread client_thread_;
    std::atomic<bool> running_{false};

    // Connection State (Replaces the server's clientList_)
    std::unique_ptr<GRAMS_TOF_Client> hubConnection_;
    std::mutex connectionMutex_; // Protects access to hubConnection_

};
