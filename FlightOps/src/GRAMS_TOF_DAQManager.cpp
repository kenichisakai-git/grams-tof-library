// GRAMS_TOF_DAQManager.cpp
#include "GRAMS_TOF_DAQManager.h"

#include "FrameServer.h"
#include "UDPFrameServer.h"
#include "Client.h"
#include "PFP_KX7.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>

using namespace PETSYS;

static bool globalUserStop = false;
static void catchUserStop(int signal) {
    fprintf(stderr, "Caught signal %d\n", signal);
    globalUserStop = true;
}

GRAMS_TOF_DAQManager::GRAMS_TOF_DAQManager(
    const std::string& socketPath,
    const std::string& shmName,
    int debugLevel,
    const std::string& daqType,
    const std::vector<std::string>& daqCards)
    : socketPath_(socketPath), shmName_(shmName), debugLevel_(debugLevel),
      daqType_(daqType), daqCardList_(daqCards), daqCardPortBits_(5),
      clientSocket_(-1), shmfd_(-1), shmPtr_(nullptr), frameServer_(nullptr),
      is_acq_running_(false) {}

GRAMS_TOF_DAQManager::~GRAMS_TOF_DAQManager() {
    stop();
    cleanup();
}

bool GRAMS_TOF_DAQManager::initialize() {
    signal(SIGTERM, catchUserStop);
    signal(SIGINT, catchUserStop);
    signal(SIGHUP, catchUserStop);

    if (daqCardList_.size() > 2) {
        fprintf(stderr, "Maximum number of DAQ cards (2) exceeded.\n");
        return false;
    }
    if (daqCardList_.empty()) {
        daqCardList_.push_back("/dev/psdaq0");
    }
    if (daqCardList_.size() == 2) daqCardPortBits_ = 2;

    clientSocket_ = createListeningSocket();
    if (clientSocket_ < 0) return false;

    FrameServer::allocateSharedMemory(shmName_.c_str(), shmfd_, shmPtr_);
    if ((shmfd_ == -1) || (shmPtr_ == nullptr)) return false;

    if (daqType_ == "GBE") {
        frameServer_ = UDPFrameServer::createFrameServer(shmName_.c_str(), shmfd_, shmPtr_, debugLevel_);
    } else if (daqType_ == "PFP_KX7") {
        for (const auto& path : daqCardList_) {
            AbstractDAQCard* card = PFP_KX7::openCard(path.c_str());
            if (!card) return false;
            daqCards_.push_back(card);
        }
        frameServer_ = DAQFrameServer::createFrameServer(daqCards_, daqCardPortBits_, shmName_.c_str(), shmfd_, shmPtr_, debugLevel_);
    } else {
        fprintf(stderr, "Unsupported DAQ type: %s\n", daqType_.c_str());
        return false;
    }

    return frameServer_ != nullptr;
}

bool GRAMS_TOF_DAQManager::run() {
    if (!frameServer_) return false;
    pollSocket();
    return true;
}

void GRAMS_TOF_DAQManager::stop() {
    globalUserStop = true;
}


void GRAMS_TOF_DAQManager::reset() {
    stop();
    cleanup();
    initialize();
    run();
}

int GRAMS_TOF_DAQManager::createListeningSocket() {
    struct sockaddr_un address;
    int socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket()");
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path), "%s", socketPath_.c_str());

    unlink(socketPath_.c_str());
    if (bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) != 0) {
        perror("bind()");
        return -1;
    }

    chmod(socketPath_.c_str(), 0660);
    if (listen(socket_fd, 5) != 0) {
        perror("listen()");
        return -1;
    }

    return socket_fd;
}

void GRAMS_TOF_DAQManager::pollSocket() {
    int epoll_fd = epoll_create(10);
    if (epoll_fd == -1) {
        perror("epoll_create()");
        return;
    }

    struct epoll_event event = {};
    event.data.fd = clientSocket_;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientSocket_, &event) == -1) {
        perror("epoll_ctl()");
        close(epoll_fd);
        return;
    }

    std::map<int, Client*> clientList;
    sigset_t mask, omask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    while (!globalUserStop) {
        memset(&event, 0, sizeof(event));

        sigprocmask(SIG_BLOCK, &mask, &omask);
        int nReady = epoll_pwait(epoll_fd, &event, 1, 100, &omask);
        sigprocmask(SIG_SETMASK, &omask, nullptr);

        if (nReady == -1) continue;

        if (event.data.fd == clientSocket_) {
            int client_fd = accept(clientSocket_, nullptr, nullptr);
            if (client_fd == -1) continue;

            fprintf(stderr, "Got new client: %d\n", client_fd);

            event.data.fd = client_fd;
            event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                perror("epoll_ctl add client_fd");
                close(client_fd);
                continue;
            }

            clientList[client_fd] = new Client(client_fd, frameServer_);
        } else {
            auto it = clientList.find(event.data.fd);
            if (it == clientList.end()) {
                // Unknown client fd — remove from epoll and close socket to clean up
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event.data.fd, nullptr);
                close(event.data.fd);
                continue;
            }
            Client* client = it->second;

            if ((event.events & EPOLLHUP) || (event.events & EPOLLERR) || (client->handleRequest() == -1)) {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event.data.fd, nullptr);
                delete client;
                clientList.erase(it);
                close(event.data.fd);
            }
        }
    }

    // Cleanup on exit: close all client sockets and free clients
    for (auto& pair : clientList) {
        close(pair.first);
        delete pair.second;
    }
    close(epoll_fd);
}

void GRAMS_TOF_DAQManager::cleanup() {
    if (frameServer_) delete frameServer_;
    for (auto* card : daqCards_) delete card;
    FrameServer::freeSharedMemory(shmName_.c_str(), shmfd_, shmPtr_);
    if (clientSocket_ != -1) {
        close(clientSocket_);
        unlink(socketPath_.c_str());
    }
}

