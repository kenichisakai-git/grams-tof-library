#include "GRAMS_TOF_EventServer.h"
#include "GRAMS_TOF_Client.h"
#include "GRAMS_TOF_CommandCodec.h"
#include "GRAMS_TOF_Logger.h"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include <chrono>

GRAMS_TOF_EventServer::GRAMS_TOF_EventServer(int port)
    : port_(port), running_(false) {}

GRAMS_TOF_EventServer::~GRAMS_TOF_EventServer() {
    stop();
}

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

    std::lock_guard<std::mutex> lock(clientListMutex_);
    for (auto& pair : clientList_) {
        pair.second->closeFD();
    }
    clientList_.clear();

    GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::EVENT);
}

// ------------------------
// Heartbeat loop
// ------------------------
void GRAMS_TOF_EventServer::heartbeatLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::vector<std::pair<int, GRAMS_TOF_Client*>> clientsCopy;
        {
            std::lock_guard<std::mutex> lock(clientListMutex_);
            for (auto& pair : clientList_) {
                clientsCopy.emplace_back(pair.first, pair.second.get());
            }
        }

        for (auto& [fd, client] : clientsCopy) {
            GRAMS_TOF_CommandCodec::Packet pkt;
            pkt.code = static_cast<uint16_t>(tof_bridge::toCommCode(TOFCommandCode::HEART_BEAT));
            pkt.argc = 0;
            pkt.argv.clear();

            if (client->sendData(GRAMS_TOF_CommandCodec::serialize(pkt).data(),
                                 GRAMS_TOF_CommandCodec::serialize(pkt).size()) <= 0) {
                Logger::instance().error("[EventServer] HEART_BEAT failed, closing client fd={}", fd);
                std::lock_guard<std::mutex> lock(clientListMutex_);
                client->closeFD();
                clientList_.erase(fd);
            }
        }
    }
}

// ------------------------
// Main server loop
// ------------------------
void GRAMS_TOF_EventServer::run() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        Logger::instance().error("[EventServer] Socket creation failed: {}", std::strerror(errno));
        return;
    }

    GRAMS_TOF_FDManager::instance().setServerFD(ServerKind::EVENT, fd);
    Logger::instance().debug("[EventServer] FD addr = {}",
        GRAMS_TOF_FDManager::instance().debug_getServerFDAddress(ServerKind::EVENT));

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        Logger::instance().error("[EventServer] Bind failed on port {}: {}", port_, std::strerror(errno));
        GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::EVENT);
        return;
    }

    if (listen(fd, 5) < 0) {
        Logger::instance().error("[EventServer] Listen failed: {}", std::strerror(errno));
        GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::EVENT);
        return;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        Logger::instance().error("[EventServer] epoll_create1 failed: {}", std::strerror(errno));
        GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::EVENT);
        return;
    }

    struct epoll_event event{};
    event.data.fd = GRAMS_TOF_FDManager::instance().getServerFD(ServerKind::EVENT);
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, GRAMS_TOF_FDManager::instance().getServerFD(ServerKind::EVENT), &event) == -1) {
        Logger::instance().error("[EventServer] epoll_ctl failed: {}", std::strerror(errno));
        GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::EVENT);
        ::close(epoll_fd);
        return;
    }

    sigset_t mask, omask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    while (running_) {
        struct epoll_event readyEvent{};
        sigprocmask(SIG_BLOCK, &mask, &omask);
        int nReady = epoll_pwait(epoll_fd, &readyEvent, 1, 100, &omask);
        sigprocmask(SIG_SETMASK, &omask, nullptr);

        if (nReady <= 0) continue;

        int fd_in_event = readyEvent.data.fd;

        if (fd_in_event == GRAMS_TOF_FDManager::instance().getServerFD(ServerKind::EVENT)) {
            int new_fd = accept(GRAMS_TOF_FDManager::instance().getServerFD(ServerKind::EVENT), nullptr, nullptr);
            if (new_fd <= 2) { ::close(new_fd); continue; }

            event.data.fd = new_fd;
            event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &event) == -1) {
                Logger::instance().error("[EventServer] epoll_ctl add client_fd failed: {}", std::strerror(errno));
                ::close(new_fd);
                continue;
            }

            Logger::instance().info("[EventServer] Client connected (fd={})", new_fd);
            std::lock_guard<std::mutex> lock(clientListMutex_);
            clientList_[new_fd] = std::make_unique<GRAMS_TOF_Client>(new_fd);
        } else {
            std::unique_ptr<GRAMS_TOF_Client>* clientPtr = nullptr;
            {
                std::lock_guard<std::mutex> lock(clientListMutex_);
                auto it = clientList_.find(fd_in_event);
                if (it != clientList_.end()) clientPtr = &it->second;
            }

            if (!clientPtr) continue;

            auto& client = **clientPtr;
            uint8_t buffer[1024];
            ssize_t n = client.recvData(buffer, sizeof(buffer));

            if (n <= 0 || (readyEvent.events & EPOLLHUP) || (readyEvent.events & EPOLLERR)) {
                Logger::instance().info("[EventServer] Closing client_fd={}", fd_in_event);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_in_event, nullptr);
                client.closeFD();
                std::lock_guard<std::mutex> lock(clientListMutex_);
                clientList_.erase(fd_in_event);
                continue;
            }

            GRAMS_TOF_CommandCodec::Packet pkt;
            if (GRAMS_TOF_CommandCodec::parse(std::vector<uint8_t>(buffer, buffer + n), pkt)) {
                // Handle packet if needed
            }
        }
    }

    // Cleanup remaining clients
    {
        std::lock_guard<std::mutex> lock(clientListMutex_);
        for (auto& pair : clientList_) {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pair.first, nullptr);
            pair.second->closeFD();
        }
        clientList_.clear();
    }

    ::close(epoll_fd);
    GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::EVENT);
}

// ------------------------
// Send packet to all clients
// ------------------------
bool GRAMS_TOF_EventServer::send(const GRAMS_TOF_CommandCodec::Packet& pkt) {
    bool success = true;
    auto serialized = GRAMS_TOF_CommandCodec::serialize(pkt);

    std::vector<std::pair<int, GRAMS_TOF_Client*>> clientsCopy;
    {
        std::lock_guard<std::mutex> lock(clientListMutex_);
        for (auto& pair : clientList_) clientsCopy.emplace_back(pair.first, pair.second.get());
    }

    for (auto& [fd, client] : clientsCopy) {
        if (client->sendData(serialized.data(), serialized.size()) <= 0) {
            Logger::instance().error("[EventServer] Send failed on fd {}", fd);
            std::lock_guard<std::mutex> lock(clientListMutex_);
            client->closeFD();
            clientList_.erase(fd);
            success = false;
        }
    }

    return success;
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

