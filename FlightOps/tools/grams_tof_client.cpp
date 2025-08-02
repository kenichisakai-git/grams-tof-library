#include "GRAMS_TOF_CommandCodec.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <code (hex|dec)> [arg1 arg2 ...]\n";
        std::cerr << "Example: " << argv[0] << " 0x5000 42 100\n";
        return 1;
    }

    // Parse command code (hex or decimal)
    uint16_t rawCode = 0;
    std::stringstream ss;
    ss << std::hex << argv[1];
    ss >> rawCode;

    GRAMS_TOF_CommandCodec::Packet pkt;
    pkt.code = static_cast<TOFCommandCode>(rawCode);

    // Parse integer arguments
    for (int i = 2; i < argc; ++i) {
        try {
            pkt.argv.push_back(std::stoi(argv[i]));
        } catch (...) {
            std::cerr << "Invalid integer argument: " << argv[i] << "\n";
            return 1;
        }
    }

    // Serialize
    auto data = GRAMS_TOF_CommandCodec::serialize(pkt);

    // Send
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    ssize_t sent = send(sock, data.data(), data.size(), 0);
    if (sent < 0) {
        perror("send");
    } else {
        std::cout << "Sent " << sent << " bytes for code 0x" << std::hex << rawCode << "\n";
    }

    close(sock);
    return 0;
}

