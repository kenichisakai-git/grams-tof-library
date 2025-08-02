#pragma once
#include "GRAMS_TOF_CommandCodec.h"
#include <functional>
#include <string>
#include <thread>
#include <atomic>

class GRAMS_TOF_CommandServer {
public:
    using CommandHandler = std::function<void(const GRAMS_TOF_CommandCodec::Packet&)>;
 
    // Starts a TCP server on the given port and sets the handler for commands.
    GRAMS_TOF_CommandServer(int port, CommandHandler handler);

    ~GRAMS_TOF_CommandServer();

    void start(); // Start the server thread (if not running)
    void stop();  // Stop the server thread and block for its finish

    // Not copyable/movable for safety:
    GRAMS_TOF_CommandServer(const GRAMS_TOF_CommandServer&) = delete;
    GRAMS_TOF_CommandServer& operator=(const GRAMS_TOF_CommandServer&) = delete;
private:
    void run();   // Main server thread function

    int port_;
    CommandHandler handler_;
    std::thread server_thread_;
    std::atomic<bool> running_;
};

