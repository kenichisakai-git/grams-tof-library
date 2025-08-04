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
        std::cerr << "Usage: " << argv[0] << " <code (hex|dec)> [arg1 arg2 ...] [--corrupt]\n";
        std::cerr << "Example: " << argv[0] << " 0x5000 0 1\n";
        std::cerr << "Example with corruption: " << argv[0] << " 0x5000 0 1 --corrupt\n";
        std::cerr << "\nNote: The program automatically counts the number of arguments after the command code\n";
        std::cerr << "      (excluding the '--corrupt' flag) and sets argc accordingly.\n";
        std::cerr << "      So argc corresponds to how many arguments you supply after the code.\n";
        return 1;
    }

    std::vector<std::string> positionalArgs;
    bool corruptPacket = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--corrupt") {
            corruptPacket = true;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        } else {
            positionalArgs.push_back(arg);
        }
    }

    if (positionalArgs.empty()) {
        std::cerr << "Usage: " << argv[0] << " <code (hex|dec)> [arg1 arg2 ...] [--corrupt]\n";
        return 1;
    }

    // Now the first positional argument is the command code
    std::string codeStr = positionalArgs[0];
    uint16_t rawCode = 0;
    try {
        rawCode = (codeStr.find("0x") == 0 || codeStr.find("0X") == 0)
                    ? std::stoul(codeStr, nullptr, 16)
                    : std::stoul(codeStr, nullptr, 10);
    } catch (...) {
        std::cerr << "Invalid command code: " << codeStr << "\n";
        return 1;
    }

    GRAMS_TOF_CommandCodec::Packet pkt;
    pkt.code = static_cast<TOFCommandCode>(rawCode);

    // Parse integer arguments from positionalArgs[1] onwards
    for (size_t i = 1; i < positionalArgs.size(); ++i) {
        try {
            pkt.argv.push_back(std::stoi(positionalArgs[i]));
        } catch (...) {
            std::cerr << "Invalid integer argument: " << positionalArgs[i] << "\n";
            return 1;
        }
    }

    std::cout << "[Client] Auto argc = " << pkt.argv.size() << " inferred from command-line.\n";

    // Serialize
    auto data = GRAMS_TOF_CommandCodec::serialize(pkt);

    // If corrupt flag set, corrupt CRC field (assuming CRC is 2 bytes before footer)
    if (corruptPacket) {
        std::cout << "[Client] Corrupting packet CRC for testing...\n";
        if (data.size() >= 10) {
            // Flip some bits in the CRC field (2 bytes at offset data.size() - 6 and -5)
            data[data.size() - 6] ^= 0xFF;
            data[data.size() - 5] ^= 0xFF;
        }
    }

    // Send
    std::cout << "[Debug] Creating socket...\n";
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 seconds timeout
    timeout.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address\n";
        close(sock);
        return 1;
    }

    std::cout << "[Debug] Connecting to server...\n";
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }


    std::cout << "[Debug] Connected. Sending data...\n";
    size_t totalSent = 0;
    while (totalSent < data.size()) {
        ssize_t sent = send(sock, data.data() + totalSent, data.size() - totalSent, 0);
        if (sent < 0) {
            perror("send");
            close(sock);
            return 1;
        }
        totalSent += sent;
    }
    std::cout << "Sent " << totalSent << " bytes (code = 0x" << std::hex << rawCode
              << ", argc = " << std::dec << pkt.argv.size() << ")\n";

    close(sock);
    return 0;
}

