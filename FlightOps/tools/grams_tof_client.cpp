#include "GRAMS_TOF_CommandCodec.h"

#include <CLI11.hpp>
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
    CLI::App app{"GRAMS TOF Client"};

    std::string codeStr;
    std::vector<int> args;
    bool corruptPacket = false;
    std::string serverIP = "127.0.0.1";
    int serverPort = 12345;

    // Define CLI options
    app.add_option("code", codeStr, "Command code (hex like 0x1A or decimal)")->required();
    app.add_option("args", args, "Command arguments (integers)")->expected(-1);
    app.add_flag("--corrupt", corruptPacket, "Corrupt CRC for testing");
    app.add_option("--ip", serverIP, "Server IP address");
    app.add_option("--port", serverPort, "Server port");

    CLI11_PARSE(app, argc, argv);

    // Parse command code
    uint16_t rawCode = 0;
    try {
        rawCode = (codeStr.find("0x") == 0 || codeStr.find("0X") == 0)
                    ? std::stoul(codeStr, nullptr, 16)
                    : std::stoul(codeStr, nullptr, 10);
    } catch (...) {
        std::cerr << "Invalid command code: " << codeStr << "\n";
        return 1;
    }

    // Build packet
    GRAMS_TOF_CommandCodec::Packet pkt;
    pkt.code = static_cast<TOFCommandCode>(rawCode);

    pkt.argv.clear();
    for (auto a : args) {
        pkt.argv.push_back(static_cast<int32_t>(a)); // match Packet::argv type
    }
    pkt.argc = static_cast<uint16_t>(pkt.argv.size());

    std::cout << "[Client] Auto argc = " << pkt.argv.size() << "\n";

    // Serialize
    auto data = GRAMS_TOF_CommandCodec::serialize(pkt);

    // Optional corruption
    if (corruptPacket && data.size() >= 10) {
        std::cout << "[Client] Corrupting packet CRC for testing...\n";
        data[data.size() - 6] ^= 0xFF;
        data[data.size() - 5] ^= 0xFF;
    }

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct timeval timeout{5, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address: " << serverIP << "\n";
        close(sock);
        return 1;
    }

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    // Send packet
    size_t totalSent = 0;
    while (totalSent < data.size()) {
        ssize_t sent = send(sock, data.data() + totalSent, data.size() - totalSent, 0);
        if (sent < 0) { perror("send"); close(sock); return 1; }
        totalSent += sent;
    }
    std::cout << "Sent " << totalSent << " bytes (code = 0x" << std::hex << rawCode
              << ", argc = " << std::dec << pkt.argv.size() << ")\n";

    // Wait for ACK/NACK
    std::vector<uint8_t> buffer(1024);
    ssize_t received = recv(sock, buffer.data(), buffer.size(), 0);
    if (received < 0) {
        perror("recv");
        close(sock);
        return 1;
    } else if (received == 0) {
        std::cerr << "[Client] Server closed connection\n";
        close(sock);
        return 1;
    }

    // Deserialize ACK/NACK
    try {
        GRAMS_TOF_CommandCodec::Packet ackPacket;
        bool ok = GRAMS_TOF_CommandCodec::parse(
            std::vector<uint8_t>(buffer.begin(), buffer.begin() + received),
            ackPacket
        );

        if (!ok) {
            std::cerr << "[Client] Failed to parse ACK/NACK packet\n";
        } else {
            std::cout << "[Client] Received packet: code = 0x" << std::hex << ackPacket.code
                      << ", argc = " << std::dec << ackPacket.argv.size() << "\n";
            if (!ackPacket.argv.empty()) {
                int origCode = ackPacket.argv[0];
                int status = ackPacket.argv.size() > 1 ? ackPacket.argv[1] : -1;
                std::cout << "[Client] Original code = 0x" << std::hex << origCode
                          << ", Status = " << (status ? "ACK" : "NACK") << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to deserialize ACK/NACK: " << e.what() << "\n";
    }

    close(sock);
    return 0;
}

