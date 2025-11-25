#include "GRAMS_TOF_CommandServer.h"
#include "GRAMS_TOF_Logger.h"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <vector>

GRAMS_TOF_CommandServer::GRAMS_TOF_CommandServer(int port, CommandHandler handler)
    : port_(port), handler_(std::move(handler)), running_(false) {}

GRAMS_TOF_CommandServer::~GRAMS_TOF_CommandServer() {
    stop();
}

void GRAMS_TOF_CommandServer::start() {
    if (running_) return;
    running_ = true;
    server_thread_ = std::thread(&GRAMS_TOF_CommandServer::run, this);
}

void GRAMS_TOF_CommandServer::stop() {
    if (!running_) return;
    running_ = false;

    if (server_thread_.joinable()) server_thread_.join();

    std::lock_guard<std::mutex> lock(clientListMutex_);
    for (auto& pair : clientList_) {
        pair.second->closeFD();
    }
    clientList_.clear();

    GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
}

void GRAMS_TOF_CommandServer::run() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        Logger::instance().error("[CommandServer] socket() failed: {}", std::strerror(errno));
        return;
    }

    GRAMS_TOF_FDManager::instance().setServerFD(ServerKind::COMMAND, fd);
    Logger::instance().debug("[CommandServer] FD addr = {}",
        GRAMS_TOF_FDManager::instance().debug_getServerFDAddress(ServerKind::COMMAND));

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        Logger::instance().error("[CommandServer] bind() failed on port {}: {}", port_, std::strerror(errno));
        GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
        return;
    }

    if (listen(fd, 10) < 0) {
        Logger::instance().error("[CommandServer] listen() failed: {}", std::strerror(errno));
        GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
        return;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        Logger::instance().error("[CommandServer] epoll_create1 failed: {}", std::strerror(errno));
        GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
        return;
    }

    struct epoll_event event{};
    event.data.fd = GRAMS_TOF_FDManager::instance().getServerFD(ServerKind::COMMAND);
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, GRAMS_TOF_FDManager::instance().getServerFD(ServerKind::COMMAND), &event) == -1) {
        Logger::instance().error("[CommandServer] epoll_ctl failed: {}", std::strerror(errno));
        GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
        ::close(epoll_fd);
        return;
    }

    sigset_t mask, omask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    const size_t buffer_size = 1024;
    uint8_t buffer[buffer_size];

    while (running_) {
        memset(&event, 0, sizeof(event));
        sigprocmask(SIG_BLOCK, &mask, &omask);
        int nReady = epoll_pwait(epoll_fd, &event, 1, 100, &omask);
        sigprocmask(SIG_SETMASK, &omask, nullptr);
        if (nReady <= 0) continue;

        int fd_in_event = event.data.fd;

        if (fd_in_event == GRAMS_TOF_FDManager::instance().getServerFD(ServerKind::COMMAND)) {
            int new_fd = accept(GRAMS_TOF_FDManager::instance().getServerFD(ServerKind::COMMAND), nullptr, nullptr);
            if (new_fd <= 2) { ::close(new_fd); continue; }

            event.data.fd = new_fd;
            event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &event) == -1) {
                Logger::instance().error("[CommandServer] epoll_ctl add client_fd failed: {}", std::strerror(errno));
                ::close(new_fd);
                continue;
            }

            Logger::instance().info("[CommandServer] Accepted client_fd={}", new_fd);
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
            ssize_t n = client.recvData(buffer, buffer_size);

            bool shouldClose = false;

            if (n <= 0 || (event.events & EPOLLHUP) || (event.events & EPOLLERR)) {
                Logger::instance().info("[CommandServer] Closing client_fd={}", fd_in_event);
                shouldClose = true;
            } else {
                GRAMS_TOF_CommandCodec::Packet pkt;
                if (GRAMS_TOF_CommandCodec::parse(std::vector<uint8_t>(buffer, buffer + n), pkt)) {

                    // Send ACK to client
                    GRAMS_TOF_CommandCodec::Packet ackPkt;
                    ackPkt.code = static_cast<uint16_t>(tof_bridge::toCommCode(TOFCommandCode::ACK));
                    ackPkt.argc = 1;
                    ackPkt.argv.clear();
                    client.sendData(GRAMS_TOF_CommandCodec::serialize(ackPkt).data(),
                                    GRAMS_TOF_CommandCodec::serialize(ackPkt).size());

                    Logger::instance().info("[CommandServer] ACK sent for command 0x{:04X}", static_cast<int>(pkt.code));

                    // --- Now handle the command ---
                    handler_(pkt);

                    // For one-shot connection, close after ACK
                    //shouldClose = trueu; //DEBUG for persistent model
                }
            }

            if (shouldClose) {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_in_event, nullptr);
                client.closeFD();
                std::lock_guard<std::mutex> lock(clientListMutex_);
                clientList_.erase(fd_in_event);
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
    GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::COMMAND);
}

