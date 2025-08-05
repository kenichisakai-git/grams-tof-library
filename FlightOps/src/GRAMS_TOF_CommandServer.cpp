#include "GRAMS_TOF_CommandServer.h"
#include "GRAMS_TOF_CommandCodec.h"
#include "GRAMS_TOF_Logger.h"
#include <iostream>
#include <sstream>
#include <cstring>      // for memset
#include <netinet/in.h> // for sockaddr_in
#include <unistd.h>     // for close()

GRAMS_TOF_CommandServer::GRAMS_TOF_CommandServer(int port, CommandHandler handler)
    : port_(port), handler_(std::move(handler)), running_(false) {}

GRAMS_TOF_CommandServer::~GRAMS_TOF_CommandServer() {
    stop();
}

void GRAMS_TOF_CommandServer::start() {
    if (!running_) {
        running_ = true;
        server_thread_ = std::thread(&GRAMS_TOF_CommandServer::run, this);
    }
}

void GRAMS_TOF_CommandServer::stop() {
    if (running_) {
        running_ = false;
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }
}

void GRAMS_TOF_CommandServer::run() {
    int server_fd, new_socket;
    sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        Logger::instance().error("[CommandServer] Socket failed");
        return;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        Logger::instance().error("[CommandServer] Bind failed on port {}", port_);
        close(server_fd); return;
    }
    if (listen(server_fd, 3) < 0) {
        Logger::instance().error("[CommandServer] Listen failed");
        close(server_fd); return;
    }
    Logger::instance().info("[CommandServer] Listening on port {}", port_);

    while (running_) {
        fd_set set;
        struct timeval timeout = {1, 0};  // 1 sec timeout
        FD_ZERO(&set);
        FD_SET(server_fd, &set);
        int rv = select(server_fd + 1, &set, NULL, NULL, &timeout);
        if (rv == 0) continue; // timeout
        if (rv < 0) break;     // error

        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) continue;     // accept failed

        char buffer[512] = {0};
        ssize_t valread = read(new_socket, buffer, sizeof(buffer));
        if (valread > 0) {
            std::vector<uint8_t> data(buffer, buffer + valread);
            GRAMS_TOF_CommandCodec::Packet pkt;
            if (GRAMS_TOF_CommandCodec::parse(data, pkt)) {
                handler_(pkt); 
            } else {
                Logger::instance().error("[CommandServer] Failed to parse incoming packet");
            }
        }
        close(new_socket);
    }
    close(server_fd);
}

