#pragma once
#include "GRAMS_TOF_CommandCodec.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

class GRAMS_TOF_EventServer {
public:
    GRAMS_TOF_EventServer(int port);
    ~GRAMS_TOF_EventServer();

    void start();
    void stop();

    bool sendCallback(const std::vector<int32_t>& args);
    bool sendHeartbeat();

private:
    void run();               // accept clients
    void heartbeatLoop();     // internal heartbeat
    bool send(const GRAMS_TOF_CommandCodec::Packet& pkt);

    int port_;
    int server_fd_ = -1;
    int client_fd_ = -1;

    std::thread server_thread_;
    std::thread heartbeat_thread_;
    std::atomic<bool> running_{false};
    std::mutex client_mutex_;
};

