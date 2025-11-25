#include "GRAMS_TOF_CommandClient.h"

// Assuming these includes handle definitions for:
// GRAMS_TOF_CommandCodec, GRAMS_TOF_Client, GRAMS_TOF_FDManager, Logger, ServerKind, and TOFCommandCode
#include "GRAMS_TOF_Logger.h"
#include "GRAMS_TOF_FDManager.h"
#include "GRAMS_TOF_CommandCodec.h"
#include "GRAMS_TOF_Client.h"
#include "GRAMS_TOF_CommandDefs.h" 

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <memory>

GRAMS_TOF_CommandClient::GRAMS_TOF_CommandClient(const std::string& hub_ip, int port, CommandHandler handler)
    : hub_ip_(hub_ip), port_(port), handler_(std::move(handler)), running_(false) {}

GRAMS_TOF_CommandClient::~GRAMS_TOF_CommandClient() {
    stop();
}

void GRAMS_TOF_CommandClient::start() {
    if (running_) return;
    running_ = true;
    client_thread_ = std::thread(&GRAMS_TOF_CommandClient::run, this);
}

void GRAMS_TOF_CommandClient::stop() {
    if (!running_) return;
    running_ = false;

    // Gracefully close the connection to unblock the run loop
    std::unique_ptr<GRAMS_TOF_Client> client_to_close = nullptr;
    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        if (hubConnection_) {
            client_to_close = std::move(hubConnection_);
        }
    }
    if (client_to_close) {
        // Closing the FD here forces the blocking epoll_pwait in run() to return/fail
        client_to_close->closeFD();
    }

    if (client_thread_.joinable()) client_thread_.join();

    GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
}

// ------------------------
// Main client connection/read loop
// ------------------------
void GRAMS_TOF_CommandClient::run() {
    sigset_t mask, omask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    const size_t buffer_size = 1024;
    uint8_t buffer[buffer_size];

    // Outer loop for connection establishment and recovery
    while (running_) {
        int connected_fd = -1;

        // --- 1. Establish Connection (Client connect logic) ---

        {
            // Clear any old connection before attempting a new one
            std::lock_guard<std::mutex> lock(connectionMutex_);
            if (hubConnection_) hubConnection_.reset();
        }

        connected_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (connected_fd < 0) {
            Logger::instance().error("[CommandClient] socket() failed: {}", std::strerror(errno));
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        // Set the FD for monitoring
        GRAMS_TOF_FDManager::instance().setServerFD(ServerKind::COMMAND, connected_fd);

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port_);

        if (inet_pton(AF_INET, hub_ip_.c_str(), &address.sin_addr) <= 0) {
            Logger::instance().error("[CommandClient] Invalid address {}:{}", hub_ip_, port_);
            ::close(connected_fd);
            GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        Logger::instance().info("[CommandClient] Attempting to connect to Hub {}:{}", hub_ip_, port_);
        if (connect(connected_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            Logger::instance().error("[CommandClient] connect() failed: {}", std::strerror(errno));
            ::close(connected_fd);
            GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait longer before retrying connection
            continue;
        }

        Logger::instance().info("[CommandClient] Successfully connected to Hub on FD={}", connected_fd);

        // Store the single connection object
        {
            std::lock_guard<std::mutex> lock(connectionMutex_);
            hubConnection_ = std::make_unique<GRAMS_TOF_Client>(connected_fd);
        }

        // --- 2. Setup Epoll ---
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            Logger::instance().error("[CommandClient] epoll_create1 failed: {}", std::strerror(errno));
            // Close connection and retry loop
            std::lock_guard<std::mutex> lock(connectionMutex_);
            if (hubConnection_) hubConnection_->closeFD();
            hubConnection_.reset();
            continue;
        }

        struct epoll_event event{};
        // Add the *connected* FD to epoll
        event.data.fd = connected_fd;
        event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connected_fd, &event) == -1) {
            Logger::instance().error("[CommandClient] epoll_ctl add connected_fd failed: {}", std::strerror(errno));
            ::close(epoll_fd);
            std::lock_guard<std::mutex> lock(connectionMutex_);
            if (hubConnection_) hubConnection_->closeFD();
            hubConnection_.reset();
            continue;
        }

        // --- 3. Persistent Read Loop ---
        int fd_in_event = connected_fd;
        bool connection_active = true;

        while (running_ && connection_active) {
            struct epoll_event readyEvent{};
            sigprocmask(SIG_BLOCK, &mask, &omask);
            int nReady = epoll_pwait(epoll_fd, &readyEvent, 1, 100, &omask);
            sigprocmask(SIG_SETMASK, &omask, nullptr);

            if (nReady <= 0) continue;

            // Safety check for connection status
            std::unique_ptr<GRAMS_TOF_Client>* clientPtr = nullptr;
            {
                std::lock_guard<std::mutex> lock(connectionMutex_);
                if (hubConnection_) clientPtr = &hubConnection_;
            }
            if (!clientPtr) {
                connection_active = false;
                continue;
            }

            auto& client = **clientPtr;
            ssize_t n = client.recvData(buffer, buffer_size);

            bool shouldClose = false;

            if (n <= 0 || (readyEvent.events & EPOLLHUP) || (readyEvent.events & EPOLLERR)) {
                Logger::instance().info("[CommandClient] Connection lost (FD={})", fd_in_event);
                shouldClose = true;
            } else {
                GRAMS_TOF_CommandCodec::Packet pkt;
                if (GRAMS_TOF_CommandCodec::parse(std::vector<uint8_t>(buffer, buffer + n), pkt)) {

                    // 1. Execute the handler (This function will execute the actual command)
                    handler_(pkt);

                    // 2. Send ACK back to the Hub
                    GRAMS_TOF_CommandCodec::Packet ackPkt;
                    ackPkt.code = pkt.code;
                    ackPkt.argc = 1;
                    ackPkt.argv.clear();
                    ackPkt.argv.push_back(GRAMS_TOF_CommandCodec::getPacketSize(pkt));

                    auto serializedAck = GRAMS_TOF_CommandCodec::serialize(ackPkt);
                    //for (auto b : serializedAck) Logger::instance().debug("{:02X}", b);

                    if (client.sendData(serializedAck.data(), serializedAck.size()) <= 0) {
                        Logger::instance().error("[CommandClient] Failed to send ACK, closing FD={}", fd_in_event);
                        shouldClose = true;
                    }
                } else {
                    Logger::instance().error("[CommandClient] Failed to parse received packet from FD={}", fd_in_event);
                    // Decide if a parsing failure should close the connection; typically, yes.
                    // shouldClose = true;
                }
            }

            if (shouldClose) {
                // Cleanup the closed socket and break the inner loop to force connection retry
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_in_event, nullptr);
                client.closeFD();

                std::lock_guard<std::mutex> lock(connectionMutex_);
                hubConnection_.reset();

                connection_active = false; // Break inner while loop
            }
        } // End inner while loop

        ::close(epoll_fd);

    } // End outer while loop (Connection Recovery)

    GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
}
