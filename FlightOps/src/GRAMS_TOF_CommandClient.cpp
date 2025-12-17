#include "GRAMS_TOF_CommandClient.h"

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
#include <stdexcept>
#include <algorithm> // For std::min

// Define the maximum expected argument count to prevent buffer overflow attacks
static constexpr uint16_t MAX_ARGC = 32; 
static constexpr size_t MIN_PACKET_SIZE = 14;

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
// Main client connection/read loop (FIXED for TCP stream reading)
// ------------------------
void GRAMS_TOF_CommandClient::run() {
    sigset_t mask, omask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    const size_t buffer_size = 1024;
    uint8_t temp_read_buffer[buffer_size]; // Buffer for immediate recv() calls
    std::vector<uint8_t> incoming_buffer;  // Buffer to accumulate fragmented packets

    // Outer loop for connection establishment and recovery
    while (running_) {
        int connected_fd = -1;
        incoming_buffer.clear(); // Clear buffer for new connection

        // --- 1. Establish Connection ---
        // ... (Connection setup code remains the same: Lines 76-120) ...
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
            std::this_thread::sleep_for(std::chrono::seconds(5)); 
            continue;
        }

        Logger::instance().info("[CommandClient] Successfully connected to Hub on FD={}", connected_fd);

        // Store the single connection object
        {
            std::lock_guard<std::mutex> lock(connectionMutex_);
            hubConnection_ = std::make_unique<GRAMS_TOF_Client>(connected_fd);
        }
        
        // --- 2. Setup Epoll ---
        // ... (Epoll setup code remains the same: Lines 122-144) ...
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            Logger::instance().error("[CommandClient] epoll_create1 failed: {}", std::strerror(errno));
            std::lock_guard<std::mutex> lock(connectionMutex_);
            if (hubConnection_) hubConnection_->closeFD();
            hubConnection_.reset();
            continue;
        }

        struct epoll_event event{};
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

        // --- 3. Persistent Read Loop (FIXED) ---
        int fd_in_event = connected_fd;
        bool connection_active = true;

        while (running_ && connection_active) {
            struct epoll_event readyEvent{};
            sigprocmask(SIG_BLOCK, &mask, &omask);
            // Wait up to 100ms for data
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
            bool shouldClose = false;

            // --- A. Read any AVAILABLE data from the socket ---
            // Use the small temp buffer for a single read
            ssize_t n_read = client.recvData(temp_read_buffer, buffer_size);

            if (n_read <= 0 || (readyEvent.events & EPOLLHUP) || (readyEvent.events & EPOLLERR)) {
                Logger::instance().info("[CommandClient] Connection lost (FD={})", fd_in_event);
                shouldClose = true;
            } else {
                // 1. Append newly read data to the persistent buffer
                incoming_buffer.insert(incoming_buffer.end(), temp_read_buffer, temp_read_buffer + n_read);

                // 2. Process all complete packets currently in the buffer
                while (incoming_buffer.size() >= MIN_PACKET_SIZE) {
                    
                    // Peek the Argc field (at byte offset 6)
                    // The Argc field is Big Endian (based on your Codec file)
                    uint16_t argc = (static_cast<uint16_t>(incoming_buffer[6]) << 8) | incoming_buffer[7];
                    
                    if (argc > MAX_ARGC) {
                        Logger::instance().error("[CommandClient] Argc={} exceeds max allowed {}. Corrupt data.", argc, MAX_ARGC);
                        shouldClose = true;
                        break; 
                    }
                    
                    // Calculate the total expected size
                    size_t expectedSize = MIN_PACKET_SIZE + (static_cast<size_t>(argc) * 4); 

                    if (incoming_buffer.size() < expectedSize) {
                        // Not enough data for the full packet. Break and wait for the next EPOLLIN event.
                        Logger::instance().debug("[CommandClient] Fragmented packet. Needed={}, Current={}", expectedSize, incoming_buffer.size());
                        break; 
                    }

                    // --- B. Extract, Parse, and Process the guaranteed full packet ---
                    
                    std::vector<uint8_t> packet_data(incoming_buffer.begin(), incoming_buffer.begin() + expectedSize);

                    // 1. Remove the successfully processed packet from the buffer
                    incoming_buffer.erase(incoming_buffer.begin(), incoming_buffer.begin() + expectedSize);                   
 
                    GRAMS_TOF_CommandCodec::Packet pkt;
                    if (GRAMS_TOF_CommandCodec::parse(packet_data, pkt)) {

                        // 2. Send ACK back to the Hub
                        GRAMS_TOF_CommandCodec::Packet ackPkt;
                        ackPkt.code = pkt.code;
                        ackPkt.argc = 1;
                        ackPkt.argv.push_back(GRAMS_TOF_CommandCodec::getPacketSize(pkt));

                        auto serializedAck = GRAMS_TOF_CommandCodec::serialize(ackPkt);
                        if (client.sendData(serializedAck.data(), serializedAck.size()) <= 0) {
                            Logger::instance().error("[CommandClient] Failed to send ACK, closing FD={}", fd_in_event);
                            shouldClose = true;
                            break; 
                        } else {
                            Logger::instance().debug("[CommandClient] ACK sent for Command Code: 0x{:04X}, Bytes: {}", 
                                                     pkt.code, serializedAck.size());                       
                        }

                        // 3. Execute the handler
                        handler_(pkt);
 
                        // 3. Remove the successfully processed packet from the buffer
                        //incoming_buffer.erase(incoming_buffer.begin(), incoming_buffer.begin() + expectedSize);

                    } else {
                        Logger::instance().error("[CommandClient] Failed to parse complete packet. Corrupt stream.", fd_in_event);
                        shouldClose = true;
                        break; 
                    }
                } // End of while (incoming_buffer.size() >= 14)
            }

            if (shouldClose) {
                // Cleanup the closed socket and break the inner loop to force connection retry
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_in_event, nullptr);
                client.closeFD();
                
                std::lock_guard<std::mutex> lock(connectionMutex_);
                hubConnection_.reset();
                incoming_buffer.clear(); // Ensure buffer is empty before retry
                
                connection_active = false; // Break inner while loop
            }
        } // End inner while loop

        ::close(epoll_fd);

    } // End outer while loop (Connection Recovery)

    GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
}
