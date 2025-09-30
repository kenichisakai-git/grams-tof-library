#include "GRAMS_TOF_FDManager.h"
#include <sys/socket.h>
#include <errno.h>

// Server operations
void GRAMS_TOF_FDManager::setServerFD(ServerKind kind, int fd) {
    int idx = static_cast<int>(kind);
    serverFDs_[idx].setFD(fd);
}

int GRAMS_TOF_FDManager::getServerFD(ServerKind kind) const {
    int idx = static_cast<int>(kind);
    return serverFDs_[idx].getFD();
}

void GRAMS_TOF_FDManager::removeServerFD(ServerKind kind) {
    int idx = static_cast<int>(kind);
    serverFDs_[idx].closeFD();
}

// Client operations
void GRAMS_TOF_FDManager::addClientFD(int fd) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (clientFDs_.count(fd) == 0) {
        clientFDs_[fd] = std::make_unique<SafeFD>(fd);
    }
}

void GRAMS_TOF_FDManager::removeClientFD(int fd) {
    std::lock_guard<std::mutex> lock(mtx_);
    clientFDs_.erase(fd);
}

std::vector<int> GRAMS_TOF_FDManager::getAllClientFDs() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<int> res;
    res.reserve(clientFDs_.size());
    for (auto const& p : clientFDs_) res.push_back(p.first);
    return res;
}

// sends
ssize_t GRAMS_TOF_FDManager::sendToServer(ServerKind kind, const uint8_t* data, size_t size) {
    int idx = static_cast<int>(kind);
    return serverFDs_[idx].sendData(data, size);
}

ssize_t GRAMS_TOF_FDManager::sendToClient(int fd, const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = clientFDs_.find(fd);
    if (it == clientFDs_.end()) return -1;
    return it->second->sendData(data, size);
}

ssize_t GRAMS_TOF_FDManager::sendToAllClients(const uint8_t* data, size_t size) {
    std::vector<SafeFD*> copy;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        copy.reserve(clientFDs_.size());
        for (auto& p : clientFDs_) copy.push_back(p.second.get());
    }
    ssize_t count = 0;
    for (auto* s : copy) if (s->sendData(data, size) > 0) ++count;
    return count;
}

// recv
ssize_t GRAMS_TOF_FDManager::recvData(int fd, uint8_t* buffer, size_t size) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = clientFDs_.find(fd);
    if (it == clientFDs_.end()) return -1;

    int real_fd = it->second->getFD();
    if (real_fd <= 2) return -1;

    // Non-blocking recv
    ssize_t n = ::recv(real_fd, buffer, size, 0);
    if (n < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0; // no data available now
        } else if (errno == EINTR) {
            return 0; // interrupted, return 0 for now
        } else {
            return -1; // actual error
        }
    }
    return n; // number of bytes received (can be 0 if connection closed)
}

// debug
const void* GRAMS_TOF_FDManager::debug_getServerFDAddress(ServerKind kind) const {
    int idx = static_cast<int>(kind);
    return serverFDs_[idx].debug_address();
}

// close
void GRAMS_TOF_FDManager::closeAll() {
    for (int i = 0; i < static_cast<int>(ServerKind::COUNT); ++i) serverFDs_[i].closeFD();
    std::lock_guard<std::mutex> lock(mtx_);
    clientFDs_.clear();
}



