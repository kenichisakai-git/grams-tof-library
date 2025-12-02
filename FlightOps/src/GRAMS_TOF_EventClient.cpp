#include "GRAMS_TOF_EventClient.h"

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
#include <utility>
#include <stdexcept>

/**
 * @brief Constructs a GRAMS_TOF_EventClient.
 * @param hub_ip The IP address of the Hub (Event Server).
 * @param port The port of the Event Server.
 * @param handler The function to call when an event packet is received.
 */
GRAMS_TOF_EventClient::GRAMS_TOF_EventClient(const std::string& hub_ip, int port, PacketHandler handler)
    : hub_ip_(hub_ip), port_(port), handler_(std::move(handler)), running_(false) {}

/**
 * @brief Destructor, ensures stop() is called.
 */
GRAMS_TOF_EventClient::~GRAMS_TOF_EventClient() {
    stop();
}

/**
 * @brief Starts the client thread.
 */
void GRAMS_TOF_EventClient::start() {
    if (running_) return;
    running_ = true;
    client_thread_ = std::thread(&GRAMS_TOF_EventClient::run, this);
}

/**
 * @brief Stops the client thread and closes the connection gracefully.
 */
void GRAMS_TOF_EventClient::stop() {
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

    // Clean up FD Manager entry
    GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::EVENT);
}

/**
 * @brief Sends a single packet to the remote Event Hub.
 * This method is called by the DAQ Core application to send CALLBACKs, HEART_BEATs, etc.
 * @param packet The packet to transmit.
 * @return true on success, false otherwise.
 */
bool GRAMS_TOF_EventClient::sendPacket(const GRAMS_TOF_CommandCodec::Packet& packet) {
    // 1. Serialize the packet for transmission
    std::vector<uint8_t> serialized_data = GRAMS_TOF_CommandCodec::serialize(packet);
    if (serialized_data.empty()) {
        Logger::instance().error("[EventClient] Failed to serialize packet.");
        return false;
    }

    // 2. Acquire lock to safely access the connection pointer
    std::lock_guard<std::mutex> lock(connectionMutex_);
    
    // 3. Check if the connection is currently established
    if (!hubConnection_) {
        Logger::instance().warn("[EventClient] Attempted to send packet but connection is down or reconnecting.");
        return false;
    }

    // 4. Send the data using the underlying client object
    if (hubConnection_->sendData(serialized_data)) {
        return true;
    } else {
        // FIX: Using fd() instead of getFD() based on GRAMS_TOF_Client.h
        Logger::instance().error("[EventClient] Failed to send {} bytes (Code: 0x{:04X}) on FD={}", 
            serialized_data.size(), 
            packet.code, 
            hubConnection_->fd()); // <-- CORRECTED METHOD CALL
        return false;
    }
}

// ------------------------
// Main client connection/read loop
// ------------------------
/**
 * @brief The main loop responsible for connecting, monitoring, and processing incoming events.
 * Handles connection recovery if the link is lost.
 */
void GRAMS_TOF_EventClient::run() {
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
            Logger::instance().error("[EventClient] socket() failed: {}", std::strerror(errno));
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        // Set the FD for monitoring
        GRAMS_TOF_FDManager::instance().setServerFD(ServerKind::EVENT, connected_fd);

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port_);

        if (inet_pton(AF_INET, hub_ip_.c_str(), &address.sin_addr) <= 0) {
            Logger::instance().error("[EventClient] Invalid address {}:{}", hub_ip_, port_);
            ::close(connected_fd);
            GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::EVENT);
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        Logger::instance().info("[EventClient] Attempting to connect to Hub {}:{}", hub_ip_, port_);
        if (connect(connected_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            Logger::instance().error("[EventClient] connect() failed: {}", std::strerror(errno));
            ::close(connected_fd);
            GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::EVENT);
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait longer before retrying connection
            continue;
        }

        Logger::instance().info("[EventClient] Successfully connected to Hub on FD={}", connected_fd);

        // Store the single connection object
        {
            std::lock_guard<std::mutex> lock(connectionMutex_);
            // The GRAMS_TOF_Client constructor should take ownership of the FD
            hubConnection_ = std::make_unique<GRAMS_TOF_Client>(connected_fd); 
        }

        // --- 2. Setup Epoll ---
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            Logger::instance().error("[EventClient] epoll_create1 failed: {}", std::strerror(errno));
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
            Logger::instance().error("[EventClient] epoll_ctl add connected_fd failed: {}", std::strerror(errno));
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
            // Use a short timeout to check the running_ flag frequently
            int nReady = epoll_pwait(epoll_fd, &readyEvent, 1, 100, &omask);
            sigprocmask(SIG_SETMASK, &omask, nullptr);

            if (nReady <= 0) continue;

            // Safety check for connection status
            GRAMS_TOF_Client* clientPtr = nullptr;
            {
                std::lock_guard<std::mutex> lock(connectionMutex_);
                if (hubConnection_) clientPtr = hubConnection_.get();
            }
            if (!clientPtr) {
                connection_active = false; // Connection was reset by stop() or an error
                continue;
            }

            auto& client = *clientPtr;
            ssize_t n = client.recvData(buffer, buffer_size);

            bool shouldClose = false;

            if (n <= 0 || (readyEvent.events & EPOLLHUP) || (readyEvent.events & EPOLLERR)) {
                // Connection closed by server or error occurred
                Logger::instance().info("[EventClient] Connection lost (FD={})", fd_in_event);
                shouldClose = true;
            } else {
                GRAMS_TOF_CommandCodec::Packet pkt;
                if (GRAMS_TOF_CommandCodec::parse(std::vector<uint8_t>(buffer, buffer + n), pkt)) {
                    // Pass the received packet to the external handler
                    handler_(pkt);
                } else {
                    Logger::instance().error("[EventClient] Failed to parse incoming packet from Hub.");
                }
            }

            if (shouldClose) {
                // Cleanup the closed socket and break the inner loop to force connection retry
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_in_event, nullptr);
                client.closeFD();

                std::lock_guard<std::mutex> lock(connectionMutex_);
                hubConnection_.reset();

                connection_active = false; // Break inner while loop to trigger connection retry
            }
        } // End inner while loop (Persistent Read)

        ::close(epoll_fd);

    } // End outer while loop (Connection Recovery)

    GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::EVENT);
}
