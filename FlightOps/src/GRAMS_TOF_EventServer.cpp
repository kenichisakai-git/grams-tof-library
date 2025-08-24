#include "GRAMS_TOF_EventServer.h"
#include "GRAMS_TOF_Logger.h"
#include "GRAMS_TOF_CommandDefs.h"

#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <iostream>
#include <errno.h>

// Helper: write all data reliably
static ssize_t safeWrite(int sock, const uint8_t* data, size_t size) {
    size_t totalWritten = 0;
    while (totalWritten < size) {
        ssize_t n = write(sock, data + totalWritten, size - totalWritten);
        if (n < 0) {
            if (errno == EINTR) continue;   // interrupted, retry
            if (errno == EPIPE) return -1;  // client disconnected
            return n;
        }
        totalWritten += n;
    }
    return totalWritten;
}

GRAMS_TOF_EventServer::GRAMS_TOF_EventServer(int port) : port_(port) {}
GRAMS_TOF_EventServer::~GRAMS_TOF_EventServer() { stop(); }

void GRAMS_TOF_EventServer::start() {
    if (running_) return;
    running_ = true;
    server_thread_ = std::thread(&GRAMS_TOF_EventServer::run, this);
    heartbeat_thread_ = std::thread(&GRAMS_TOF_EventServer::heartbeatLoop, this);
}

void GRAMS_TOF_EventServer::stop() {
    if (!running_) return;
    running_ = false;

    if (server_thread_.joinable()) server_thread_.join();
    if (heartbeat_thread_.joinable()) heartbeat_thread_.join();

    std::lock_guard<std::mutex> lock(client_mutex_);
    if (client_fd_ >= 0) { close(client_fd_); client_fd_ = -1; }
    if (server_fd_ >= 0) { close(server_fd_); server_fd_ = -1; }
}

void GRAMS_TOF_EventServer::heartbeatLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        int fd_copy = -1;
        {
            std::lock_guard<std::mutex> lock(client_mutex_);
            fd_copy = client_fd_; // copy the client fd
        }
        if (fd_copy >= 0) {
            if (sendHeartbeat()) {
                Logger::instance().info("[EventServer] HEART_BEAT sent");
            } else {
                Logger::instance().error("[EventServer] Failed to send HEART_BEAT");
                std::lock_guard<std::mutex> lock(client_mutex_);
                close(client_fd_);
                client_fd_ = -1;
            }
        } else {
            Logger::instance().warn("[EventServer] No client connected, skipping HEART_BEAT");
        }
    }
}

void GRAMS_TOF_EventServer::run() {
    sockaddr_in address{};
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == 0) { Logger::instance().error("[EventServer] Socket failed"); return; }

    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        Logger::instance().error("[EventServer] Bind failed on port {}", port_);
        close(server_fd_); server_fd_ = -1; return;
    }
    if (listen(server_fd_, 3) < 0) {
        Logger::instance().error("[EventServer] Listen failed");
        close(server_fd_); server_fd_ = -1; return;
    }

    Logger::instance().info("[EventServer] Listening on port {}", port_);

    while (running_) {
        fd_set set;
        struct timeval timeout{1, 0};
        FD_ZERO(&set);
        FD_SET(server_fd_, &set);

        int rv = select(server_fd_ + 1, &set, nullptr, nullptr, &timeout);
        if (rv < 0) break;
        if (rv == 0) continue;

        int new_socket = accept(server_fd_, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) continue;

        std::lock_guard<std::mutex> lock(client_mutex_);
        if (client_fd_ >= 0) { Logger::instance().warn("[EventServer] Replacing existing client"); close(client_fd_); }
        client_fd_ = new_socket;
        Logger::instance().info("[EventServer] Client connected");
    }
}

bool GRAMS_TOF_EventServer::send(const GRAMS_TOF_CommandCodec::Packet& pkt) {
    std::lock_guard<std::mutex> lock(client_mutex_);
    if (!running_ || client_fd_ < 0) return false;

    auto bytes = GRAMS_TOF_CommandCodec::serialize(pkt);
    ssize_t n = safeWrite(client_fd_, bytes.data(), bytes.size());
    if (n < 0) {
        Logger::instance().error("[EventServer] Send failed: {}", std::strerror(errno));
        close(client_fd_);
        client_fd_ = -1;
        return false;
    }
    return true;
}

bool GRAMS_TOF_EventServer::sendCallback(const std::vector<int32_t>& args) {
    GRAMS_TOF_CommandCodec::Packet pkt;
    pkt.code = static_cast<uint16_t>(tof_bridge::toCommCode(TOFCommandCode::CALLBACK));
    pkt.argv = args;
    pkt.argc = static_cast<uint16_t>(args.size());
    return send(pkt);
}

bool GRAMS_TOF_EventServer::sendHeartbeat() {
    GRAMS_TOF_CommandCodec::Packet pkt;
    pkt.code = static_cast<uint16_t>(tof_bridge::toCommCode(TOFCommandCode::HEART_BEAT));
    pkt.argc = 0;
    pkt.argv.clear();
    return send(pkt);
}

