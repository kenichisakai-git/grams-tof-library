#pragma once

#include <sys/types.h> 
#include <cstddef> 
#include <cstdint>
#include <vector>

class GRAMS_TOF_Client {
public:
    explicit GRAMS_TOF_Client(int fd);
    ~GRAMS_TOF_Client();

    // Non-copyable
    GRAMS_TOF_Client(const GRAMS_TOF_Client&) = delete;
    GRAMS_TOF_Client& operator=(const GRAMS_TOF_Client&) = delete;

    // Movable
    GRAMS_TOF_Client(GRAMS_TOF_Client&& other) noexcept;
    GRAMS_TOF_Client& operator=(GRAMS_TOF_Client&& other) noexcept;

    // Get client FD
    int fd() const;

    // Send/recv through FDManager
    ssize_t sendData(const void* data, size_t size);
    ssize_t sendData(const std::vector<uint8_t>& data);
    ssize_t recvData(void* buffer, size_t size);

    // Close the FD safely
    void closeFD();

private:
    int fd_;
    std::vector<uint8_t> readBuffer_; 
};

