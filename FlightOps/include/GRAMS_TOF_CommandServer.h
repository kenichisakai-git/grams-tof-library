#pragma once

#include "GRAMS_TOF_CommandCodec.h"
#include "GRAMS_TOF_Client.h"
#include "GRAMS_TOF_FDManager.h"

#include <functional>
#include <thread>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>

class GRAMS_TOF_CommandServer {
public:
    using CommandHandler = std::function<void(const GRAMS_TOF_CommandCodec::Packet&)>;

    explicit GRAMS_TOF_CommandServer(int port, CommandHandler handler);
    ~GRAMS_TOF_CommandServer();

    void start();
    void stop();

    GRAMS_TOF_CommandServer(const GRAMS_TOF_CommandServer&) = delete;
    GRAMS_TOF_CommandServer& operator=(const GRAMS_TOF_CommandServer&) = delete;

private:
    void run();  // Main server loop

    int port_;
    CommandHandler handler_;
    std::thread server_thread_;
    std::atomic<bool> running_{false};

    std::map<int, std::unique_ptr<GRAMS_TOF_Client>> clientList_;
    std::mutex clientListMutex_;  // protect structural changes
};

