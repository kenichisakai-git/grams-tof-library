#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_Logger.h"
#include "FrameServer.h"
#include "UDPFrameServer.h"
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
#include <iostream>
#include <atomic>
#include <memory>

using namespace PETSYS;

static std::atomic<bool> globalUserStop(false);

static void catchUserStop(int signal) {
    fprintf(stderr, "Caught signal %d\n", signal);
    globalUserStop.store(true, std::memory_order_relaxed);
}

GRAMS_TOF_DAQManager::GRAMS_TOF_DAQManager(
    const std::string& socketPath,
    const std::string& shmName,
    int debugLevel,
    const std::string& daqType,
    const std::vector<std::string>& daqCards)
    : socketPath_(socketPath),
      shmName_(shmName),
      debugLevel_(debugLevel),
      daqType_(daqType),
      daqCardList_(daqCards),
      daqCardPortBits_(5),
      shmfd_(-1),
      shmPtr_(nullptr),
      frameServer_(nullptr),
      is_acq_running_(false)
{}

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
    if (daqCardList_.empty()) daqCardList_.push_back("/dev/psdaq0");
    if (daqCardList_.size() == 2) daqCardPortBits_ = 2;

    int fd = createListeningSocket();
    if (fd < 0) return false;

    GRAMS_TOF_FDManager::instance().setServerFD(ServerKind::DAQ, fd);
    Logger::instance().debug("[DAQManaer] FD addr = {}",
    GRAMS_TOF_FDManager::instance().debug_getServerFDAddress(ServerKind::DAQ));


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
        frameServer_ = DAQFrameServer::createFrameServer(
            daqCards_, daqCardPortBits_, shmName_.c_str(), shmfd_, shmPtr_, debugLevel_);
    } else {
        fprintf(stderr, "Unsupported DAQ type: %s\n", daqType_.c_str());
        return false;
    }

    return frameServer_ != nullptr;
}

bool GRAMS_TOF_DAQManager::run() {
    if (!frameServer_) return false;
  
    globalUserStop.store(false, std::memory_order_relaxed);

    pollSocket();
    return true;
}

void GRAMS_TOF_DAQManager::stop() {
    globalUserStop.store(true, std::memory_order_relaxed);
}

int GRAMS_TOF_DAQManager::createListeningSocket() {
    struct sockaddr_un address;
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket()");
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path), "%s", socketPath_.c_str());

    unlink(socketPath_.c_str());
    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) != 0) {
        perror("bind()");
        ::close(fd);
        return -1;
    }

    chmod(socketPath_.c_str(), 0660);
    if (listen(fd, 5) != 0) {
        perror("listen()");
        ::close(fd);
        return -1;
    }

    return fd;
}

void GRAMS_TOF_DAQManager::pollSocket() {
    int epoll_fd = epoll_create(10);
    if (epoll_fd == -1) { perror("epoll_create()"); return; }

    struct epoll_event event = {};
    event.data.fd = GRAMS_TOF_FDManager::instance().getServerFD(ServerKind::DAQ);
    event.events = EPOLLIN;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event) == -1) {
        perror("epoll_ctl()");
        ::close(epoll_fd);
        return;
    }
    // DEBUG
    // Use PETSYS::Client to manage DAQ connections.
    // Definitely don't use GRAMS_TOF_Client. 
    // It's only for the Event and Command servers. 
    // I lost some time before realizing that.
    std::map<int, std::unique_ptr<Client>> clientList;

    sigset_t mask, omask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    while (!globalUserStop.load(std::memory_order_relaxed)) {
        memset(&event, 0, sizeof(event));

        sigprocmask(SIG_BLOCK, &mask, &omask);
        int nReady = epoll_pwait(epoll_fd, &event, 1, 100, &omask);
        sigprocmask(SIG_SETMASK, &omask, nullptr);

        if (nReady <= 0) continue;

        int fd_in_event = event.data.fd;

        if (fd_in_event == GRAMS_TOF_FDManager::instance().getServerFD(ServerKind::DAQ)) {
            int new_fd = accept(fd_in_event, nullptr, nullptr);
            if (new_fd == -1 || new_fd <= 2) { 
                if (new_fd != -1) ::close(new_fd); 
                continue; 
            }

            fprintf(stderr, "[DAQManager] Got new client_fd=%d (server_fd=%d)\n",
                         new_fd, fd_in_event);

            event.data.fd = new_fd;
            event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &event) == -1) {
                perror("epoll_ctl add client_fd");
                ::close(new_fd);
                continue;
            }

            // New: Create PETSYS::Client, passing the socket and the FrameServer
            clientList[new_fd] = std::make_unique<Client>(new_fd, frameServer_); 

        } else {
            auto it = clientList.find(fd_in_event);
            if (it == clientList.end()) continue;

            Client* client = it->second.get();

            // New: Check for errors/hang-ups first
            if ((event.events & EPOLLHUP) || (event.events & EPOLLERR)) {
                 fprintf(stderr, "[DAQManager] Client hung up or error on client_fd=%d\n", fd_in_event);
                 goto client_cleanup;
            }

            // New: Use the generic handleRequest() for all commands
            if (event.events & EPOLLIN) {
                int actionStatus = client->handleRequest();
                if (actionStatus == -1) {
                    fprintf(stderr, "[DAQManager] Error handling request from client_fd=%d\n", fd_in_event);
                    goto client_cleanup;
                }
            }
            continue; // Go back to poll

        // New Cleanup Block
        client_cleanup:
            fprintf(stderr, "[DAQManager] Closing client_fd=%d\n", fd_in_event);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_in_event, nullptr);
            // Client destructor (called on erase) handles closing the socket
            clientList.erase(it);
        }
    }

    // Cleanup remaining clients
    for (auto& pair : clientList) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pair.first, nullptr);
    }
    clientList.clear();

    ::close(epoll_fd);
    //GRAMS_TOF_FDManager::instance().removeServerFD(ServerKind::DAQ);
    //unlink(socketPath_.c_str());
}


void GRAMS_TOF_DAQManager::cleanup() {
    if (frameServer_) delete frameServer_;
    for (auto* card : daqCards_) delete card;
    FrameServer::freeSharedMemory(shmName_.c_str(), shmfd_, shmPtr_);
}

