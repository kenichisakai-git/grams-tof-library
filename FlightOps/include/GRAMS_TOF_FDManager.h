#pragma once

#include <mutex>
#include <map>
#include <vector>
#include <memory>
#include <cstdint>
#include <atomic>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstddef>

class SafeFD {
public:
    explicit SafeFD(int fd = -1) : fd_(fd) {}
    ~SafeFD() { closeFD(); }

    // Atomically set FD and close the old one (if valid)
    void setFD(int fd) {
        int old_fd = fd_.exchange(fd, std::memory_order_acq_rel);
        if (old_fd > 2) ::close(old_fd);
    }

    int getFD() const {
        return fd_.load(std::memory_order_acquire);
    }

    void closeFD() {
        int old_fd = fd_.exchange(-1, std::memory_order_acq_rel);
        if (old_fd > 2) ::close(old_fd);
    }

    // Thread-safe send - serializes concurrent sends on same fd
    ssize_t sendData(const uint8_t* data, size_t size) {
        std::lock_guard<std::mutex> lock(send_mtx_);
        int fd = fd_.load(std::memory_order_acquire);
        if (fd <= 2) return -1;

#ifdef MSG_NOSIGNAL
        int flags = MSG_NOSIGNAL;
#else
        int flags = 0;
#endif
        size_t total = 0;
        while (total < size) {
            ssize_t n = ::send(fd, data + total, size - total, flags);
            if (n < 0) {
                if (errno == EINTR) continue;
                return -1;
            }
            total += static_cast<size_t>(n);
        }
        return static_cast<ssize_t>(total);
    }

    // return pointer for debug (address of object)
    const void* debug_address() const noexcept { return static_cast<const void*>(this); }

private:
    std::atomic<int> fd_{-1};
    std::mutex send_mtx_;
};


// Manager can manage multiple server sockets; add more kinds as needed
enum class ServerKind : int {
    COMMAND = 0,
    EVENT   = 1,
    DAQ     = 2,
    COUNT   = 3
};


class GRAMS_TOF_FDManager {
public:
    static GRAMS_TOF_FDManager& instance() {
        static GRAMS_TOF_FDManager inst;
        return inst;
    }

    // per-kind server FD ops
    void setServerFD(ServerKind kind, int fd);
    int  getServerFD(ServerKind kind) const;
    void removeServerFD(ServerKind kind);

    // clients (shared across kinds)
    void addClientFD(int fd);
    void removeClientFD(int fd);
    std::vector<int> getAllClientFDs() const;

    // sends
    ssize_t sendToServer(ServerKind kind, const uint8_t* data, size_t size);
    ssize_t sendToClient(int fd, const uint8_t* data, size_t size);
    ssize_t sendToAllClients(const uint8_t* data, size_t size);

    // recv
    ssize_t recvData(int fd, uint8_t* buffer, size_t size);

    // debug helpers
    const void* debug_getServerFDAddress(ServerKind kind) const;

    // close
    void closeAll();

private:
    GRAMS_TOF_FDManager() = default;
    ~GRAMS_TOF_FDManager() { closeAll(); }
    GRAMS_TOF_FDManager(const GRAMS_TOF_FDManager&) = delete;
    GRAMS_TOF_FDManager& operator=(const GRAMS_TOF_FDManager&) = delete;

    // server fds array
    SafeFD serverFDs_[static_cast<int>(ServerKind::COUNT)];
    // clients map protected by mtx_
    mutable std::mutex mtx_;
    std::map<int, std::unique_ptr<SafeFD>> clientFDs_;
};

