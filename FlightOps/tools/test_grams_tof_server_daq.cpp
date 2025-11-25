#include "GRAMS_TOF_CommandCodec.h"
#include "GRAMS_TOF_CommandDefs.h"
#include <CLI11.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <iomanip>

// --- Function to parse command code ---
uint16_t parseCommandCode(const std::string& codeStr) {
    try {
        return (codeStr.find("0x") == 0 || codeStr.find("0X") == 0)
            ? std::stoul(codeStr, nullptr, 16)
            : std::stoul(codeStr, nullptr, 10);
    } catch (...) {
        throw std::runtime_error("Invalid command code: " + codeStr);
    }
}

// --- Function to build a packet ---
GRAMS_TOF_CommandCodec::Packet buildPacket(uint16_t code, const std::vector<int>& args) {
    GRAMS_TOF_CommandCodec::Packet pkt;
    pkt.code = code;
    pkt.argv.clear();
    for (auto a : args) pkt.argv.push_back(static_cast<int32_t>(a));
    pkt.argc = static_cast<uint16_t>(pkt.argv.size());
    return pkt;
}

// --- Function to connect a TCP socket ---
int connectSocket(const std::string& ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) throw std::runtime_error("Failed to create socket");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        close(sock);
        throw std::runtime_error("Invalid IP address: " + ip);
    }

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        throw std::runtime_error("Failed to connect to " + ip + ":" + std::to_string(port));
    }

    return sock;
}

// --- Function to send a packet ---
void sendPacket(int sock, const GRAMS_TOF_CommandCodec::Packet& pkt) {
    auto data = GRAMS_TOF_CommandCodec::serialize(pkt);
    size_t totalSent = 0;
    while (totalSent < data.size()) {
        ssize_t sent = send(sock, data.data() + totalSent, data.size() - totalSent, 0);
        if (sent < 0) throw std::runtime_error("Send failed");
        totalSent += sent;
    }
}

// --- Function to receive a packet ---
GRAMS_TOF_CommandCodec::Packet receivePacket(int sock, int timeoutSec = 5) {
    struct timeval tv{};
    tv.tv_sec = timeoutSec;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::vector<uint8_t> buffer(1024);
    ssize_t n = recv(sock, buffer.data(), buffer.size(), 0);

    if (n < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            throw std::runtime_error("Receive timed out");
        } else {
            throw std::runtime_error("Receive failed");
        }
    } else if (n == 0) {
        throw std::runtime_error("Server closed connection");
    }

    GRAMS_TOF_CommandCodec::Packet pkt;
    if (!GRAMS_TOF_CommandCodec::parse({buffer.begin(), buffer.begin() + n}, pkt)) {
        throw std::runtime_error("Failed to parse packet");
    }
    return pkt;
}

// --- Wait for ACK/NACK with timeout ---
GRAMS_TOF_CommandCodec::Packet waitForAck(int sock, int maxMinutes = 60) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        try {
            return receivePacket(sock, 10);
        } catch (const std::exception& e) {
            if (std::string(e.what()) == "Receive timed out") {
                auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
                                   std::chrono::steady_clock::now() - start);
                if (elapsed.count() >= maxMinutes) {
                    throw std::runtime_error("Timeout waiting for ACK (exceeded maxMinutes)");
                }
                std::cout << "[Client] Still waiting for ACK... "
                          << elapsed.count() << " minutes elapsed\n";
                continue;
            } else {
                throw;
            }
        }
    }
}

// --- Event listener thread with timestamps ---
void eventListenerThread(int eventSock, std::atomic<bool>& running) {
    std::vector<uint8_t> buffer(1024);
    while (running) {
        ssize_t n = recv(eventSock, buffer.data(), buffer.size(), 0);
        if (n <= 0) break;

        GRAMS_TOF_CommandCodec::Packet pkt;
        if (GRAMS_TOF_CommandCodec::parse({buffer.begin(), buffer.begin() + n}, pkt)) {
            auto now = std::chrono::system_clock::now();
            auto t_c = std::chrono::system_clock::to_time_t(now);
            std::cout << "[" << std::put_time(std::localtime(&t_c), "%F %T") << "] ";

            if (pkt.code == static_cast<uint16_t>(TOFCommandCode::HEART_BEAT)) {
                std::cout << "[EventListener] HEART_BEAT received\n";
            } else if (pkt.code == static_cast<uint16_t>(TOFCommandCode::CALLBACK)) {
                std::cout << "[EventListener] CALLBACK received, args:";
                for (auto a : pkt.argv) std::cout << " " << a;
                std::cout << "\n";
            }
        }
    }
}

// --- Main client logic ---
int runClient(const std::string& serverIP, int serverPort, int eventPort,
              uint16_t cmdCode, const std::vector<int>& args,
              bool no_wait = false) {

    // Connect to EventServer
    int eventSock = -1;
    for (int attempt = 0; attempt < 5; ++attempt) {
        try {
            eventSock = connectSocket(serverIP, eventPort);
            std::cout << "[Client] Connected to EventServer on attempt " << (attempt+1) << "\n";
            break;
        } catch (const std::exception& e) {
            std::cerr << "[Client] EventServer connection failed: " << e.what()
                      << " (attempt " << (attempt+1) << "/5)\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    if (eventSock < 0) {
        throw std::runtime_error("Could not connect to EventServer after retries");
    }

    std::atomic<bool> running{true};
    std::thread evtThread(eventListenerThread, eventSock, std::ref(running));

    // Connect to CommandServer
    int cmdSock = connectSocket(serverIP, serverPort);

    // Build and send command
    auto pkt = buildPacket(cmdCode, args);
    sendPacket(cmdSock, pkt);
    std::cout << "[Client] Sent command 0x" << std::hex << cmdCode
              << " with " << std::dec << args.size() << " args\n";

    // Receive ACK/NACK
    try {
        auto ackPkt = waitForAck(cmdSock, 60);
        std::cout << "[Client] Received ACK/NACK packet, code=0x" << std::hex << ackPkt.code
                  << ", argc=" << std::dec << ackPkt.argv.size() << "\n";
        if (!ackPkt.argv.empty()) {
            int origCode = ackPkt.argv[0];
            int status = ackPkt.argv.size() > 1 ? ackPkt.argv[1] : -1;
            std::cout << "[Client] Original code=0x" << std::hex << origCode
                      << ", Status=" << (status ? "ACK" : "NACK") << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[Client] " << e.what() << "\n";
    }

    close(cmdSock);

    if (!no_wait) {
        std::cout << "[Client] Press Enter to quit and stop event listener...\n";
        std::cin.get();
    } else {
        // brief grace period to allow immediate events to arrive
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop event listener safely
    running = false;
    shutdown(eventSock, SHUT_RD);
    if (evtThread.joinable()) evtThread.join();
    close(eventSock);

    return 0;
}

// --- main() ---
int main(int argc, char* argv[]) {
    CLI::App app{"GRAMS TOF Client"};

    std::string codeStr;
    std::vector<int> args;
    std::string serverIP = "127.0.0.1";
    // --- PORTS MODIFIED TO MATCH network.cfg [TOF] ---
    int serverPort = 50007; // Command server port (comport)
    int eventPort  = 50006; // Event server port (telport: CALLBACK/HEART_BEAT)
    // -------------------------------------------------

    bool no_wait = false;

    app.add_option("code", codeStr, "Command code")->required();
    app.add_option("args", args, "Command arguments")->expected(-1);
    app.add_option("--ip", serverIP, "Server IP");
    app.add_option("--server-port", serverPort, "Command server port");
    app.add_option("--event-port", eventPort, "Event server port");
    app.add_flag("--no-wait,-n", no_wait, "Do not wait for Enter; exit automatically after ACK");

    CLI11_PARSE(app, argc, argv);

    try {
        uint16_t cmdCode = parseCommandCode(codeStr);
        return runClient(serverIP, serverPort, eventPort, cmdCode, args, no_wait);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

