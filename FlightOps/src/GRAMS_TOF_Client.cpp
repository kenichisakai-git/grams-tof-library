#include "GRAMS_TOF_Client.h"
#include "GRAMS_TOF_FDManager.h"
#include <iostream>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <cstring>

GRAMS_TOF_Client::GRAMS_TOF_Client(int fd)
    : fd_(fd)
{
    if (fd_ >= 0) {
        // Set non-blocking
        int flags = fcntl(fd_, F_GETFL, 0);
        fcntl(fd_, F_SETFL, flags | O_NONBLOCK);

        GRAMS_TOF_FDManager::instance().addClientFD(fd_);
    } else {
        std::cerr << "[Client] Invalid FD passed to constructor\n";
    }
}

GRAMS_TOF_Client::~GRAMS_TOF_Client() {
    closeFD();
}

// Move constructor
GRAMS_TOF_Client::GRAMS_TOF_Client(GRAMS_TOF_Client&& other) noexcept
    : fd_(other.fd_), readBuffer_(std::move(other.readBuffer_))
{
    other.fd_ = -1;
}

// Move assignment
GRAMS_TOF_Client& GRAMS_TOF_Client::operator=(GRAMS_TOF_Client&& other) noexcept {
    if (this != &other) {
        closeFD();
        fd_ = other.fd_;
        readBuffer_ = std::move(other.readBuffer_);
        other.fd_ = -1;
    }
    return *this;
}

int GRAMS_TOF_Client::fd() const {
    return fd_;
}

// Send data
ssize_t GRAMS_TOF_Client::sendData(const void* data, size_t size) {
    if (fd_ < 0) return -1;
    return GRAMS_TOF_FDManager::instance().sendToClient(fd_, reinterpret_cast<const uint8_t*>(data), size);
}

ssize_t GRAMS_TOF_Client::sendData(const std::vector<uint8_t>& data) {
    return sendData(data.data(), data.size());
}

// Receive data (non-blocking, with internal buffer)
ssize_t GRAMS_TOF_Client::recvData(void* buffer, size_t size) {
    if (fd_ < 0) return -1;

    // Try to read available data into a temporary buffer
    uint8_t temp[1024];
    ssize_t n = GRAMS_TOF_FDManager::instance().recvData(fd_, temp, sizeof(temp));
    if (n > 0) {
        readBuffer_.insert(readBuffer_.end(), temp, temp + n);
    } else if (n < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            n = 0; // no data available
        } else {
            return -1; // other error
        }
    }

    // Copy as much as requested from internal buffer
    size_t toCopy = std::min(size, readBuffer_.size());
    if (toCopy > 0) {
        std::memcpy(buffer, readBuffer_.data(), toCopy);
        readBuffer_.erase(readBuffer_.begin(), readBuffer_.begin() + toCopy);
    }

    return static_cast<ssize_t>(toCopy);
}

// Close the client FD safely
void GRAMS_TOF_Client::closeFD() {
    if (fd_ >= 0) {
        GRAMS_TOF_FDManager::instance().removeClientFD(fd_);
        fd_ = -1;
        readBuffer_.clear();
    }
}

