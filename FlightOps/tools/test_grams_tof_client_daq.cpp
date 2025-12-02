#include <ncurses.h>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <atomic>
#include <chrono>
#include <iostream>
#include <vector>
#include <sstream> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <fcntl.h>
#include <iomanip>

#include "GRAMS_TOF_CommandCodec.h"

// ---------- Global variables ----------
std::mutex logMutex;
std::queue<std::string> logQueue;
std::atomic<bool> running{true};

// ---------- Logging helper ----------
void addLog(const std::string& msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    logQueue.push(msg);
}

void logPacketSent(const GRAMS_TOF_CommandCodec::Packet& pkt) {
    std::stringstream ss;
    ss << "[Client] Sent command 0x";
    ss << std::hex << std::uppercase << pkt.code;
    ss << " with " << std::dec << pkt.argc << " args";
    if (pkt.argc > 0) {
        ss << " [Args: ";
        for (size_t i = 0; i < pkt.argc; ++i) {
            ss << pkt.argv[i] << (i == pkt.argc - 1 ? "" : ", ");
        }
        ss << "]";
    }
    addLog(ss.str());
}

ssize_t readAll(int sock, char* buffer, size_t len) {
    size_t totalRead = 0;
    while (totalRead < len) {
        // Calculate remaining bytes to read
        ssize_t n = recv(sock, buffer + totalRead, len - totalRead, 0);

        if (n == 0) return 0;
        if (n < 0) {
            if (errno == EINTR) continue; // Retry if interrupted by signal
            return -1;
        }
        totalRead += n;
    }
    return totalRead;
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

// ---------- TCP Server helper ----------
int setupServer(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    if (listen(sock, 1) < 0)
        throw std::runtime_error("listen() failed");

    return sock;
}

// ---------- Command Server Thread ----------
void commandServerThread(int port, int& clientSock) {
    try {
        int serverSock = setupServer(port);
        addLog("[CommandServer] Listening on port " + std::to_string(port));

        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) {
            addLog("[CommandServer] accept() failed");
            close(serverSock);
            return;
        }

        addLog("[CommandServer] DAQ client connected");

        char buffer[1024];
        while (running) {
            ssize_t n = recv(clientSock, buffer, sizeof(buffer)-1, 0);

            if (n > 0) {
                std::stringstream ss;
                ss << "[CommandServer] Received " << std::dec << n << " bytes: ";
                ss << std::hex << std::uppercase;
                for (ssize_t i = 0; i < n; ++i) {
                    ss << std::setw(2) << std::setfill('0') << (int)(unsigned char)buffer[i] << " ";
                }
                addLog(ss.str());
            } else if (n == 0) {
                addLog("[CommandServer] Client disconnected");
                break;
            } else {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    addLog(std::string("[CommandServer] Recv error: ") + std::strerror(errno));
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        close(clientSock);
        clientSock = -1; // Important to reset global socket file descriptor
        close(serverSock);
    } catch (const std::exception& e) {
        addLog(std::string("[CommandServer] Exception: ") + e.what());
    }
}


// ---------- Event Server Thread ----------
void eventServerThread(int port, int& clientSock) {
    try {
        int serverSock = setupServer(port);
        addLog("[EventServer] Listening on port " + std::to_string(port));

        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) {
            addLog("[EventServer] accept() failed");
            close(serverSock);
            return;
        }

        addLog("[EventServer] DAQ client connected");

        char buffer[1024];
        while (running) {
            ssize_t n = recv(clientSock, buffer, sizeof(buffer)-1, 0); 

            if (n > 0) {
                // Log the raw bytes received for event data
                std::stringstream ss;
                ss << "[EventServer] Received " << std::dec << n << " bytes: ";
                ss << std::hex << std::uppercase;
                for (ssize_t i = 0; i < n; ++i) {
                    ss << std::setw(2) << std::setfill('0') << (int)(unsigned char)buffer[i] << " ";
                }
                addLog(ss.str());

            } else if (n == 0) {
                addLog("[EventServer] Client disconnected");
                break;
            } else {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    addLog(std::string("[EventServer] Recv error: ") + std::strerror(errno));
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        close(clientSock);
        clientSock = -1; // Important to reset global socket file descriptor
        close(serverSock);
    } catch (const std::exception& e) {
        addLog(std::string("[EventServer] Exception: ") + e.what());
    }
}


// ---------- Main ----------
int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int inputHeight = 3;
    int logHeight = rows - inputHeight;

    WINDOW* logWin = newwin(logHeight, cols, 0, 0);
    WINDOW* inputWin = newwin(inputHeight, cols, logHeight, 0);

    scrollok(logWin, TRUE);
    box(inputWin, 0, 0);
    wrefresh(logWin);
    wrefresh(inputWin);

    std::string inputStr;

    // Draw initial input window with prompt
    mvwprintw(inputWin, 1, 1, "> ");
    wmove(inputWin, 1, 3);
    wrefresh(inputWin);

    nodelay(inputWin, TRUE);
    keypad(inputWin, TRUE);

    int commandClientSock = -1;
    int eventClientSock = -1;

    std::thread tCommand(commandServerThread, 50007, std::ref(commandClientSock));
    std::thread tEvent(eventServerThread, 50006, std::ref(eventClientSock));

    while (running) {
        // --- Update log window ---
        {
            std::lock_guard<std::mutex> lock(logMutex);
            while (!logQueue.empty()) {
                std::string msg = logQueue.front();
                logQueue.pop();
                wprintw(logWin, "%s\n", msg.c_str());
            }
            wrefresh(logWin);  // refresh log window first
        }
    
        // --- Handle input ---
        int ch = wgetch(inputWin);
        if (ch != ERR) {
            if (ch == '\n') {
                if (!inputStr.empty()) {
                    if (commandClientSock >= 0) {
                        try {
                            auto pkt = GRAMS_TOF_CommandCodec::Packet();
 
                            std::vector<int> args;
                            std::vector<std::string> tokens;
                            std::istringstream iss(inputStr);
                            std::string token;
                            while (iss >> token) {
                                tokens.push_back(token);
                            }
                                
                            if (!tokens.empty()) {
                                // First token is command code
                                auto code = std::stoul(tokens[0], nullptr, 0);
                                // Remaining tokens are arguments
                                for (size_t i = 1; i < tokens.size(); ++i) {
                                    args.push_back(std::stoi(tokens[i]));
                                }
                               
                                auto pkt = buildPacket(code, args);
                                sendPacket(commandClientSock, pkt);
                                logPacketSent(pkt);
                            }
                        } catch (const std::exception& e) {
                            addLog(std::string("[Command] Failed to build packet: ") + e.what());
                        }
                    } else {
                        addLog("[Command] No client connected!");
                    }
                    inputStr.clear();
                }
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                if (!inputStr.empty()) inputStr.pop_back();
            } else if (isprint(ch)) {
                inputStr.push_back(ch);
            }
    
            // redraw input line
            wclear(inputWin);
            box(inputWin, 0, 0);
            mvwprintw(inputWin, 1, 1, "> %s", inputStr.c_str());
        }
    
        // Always move cursor to inputWin at the end
        wmove(inputWin, 1, 3 + inputStr.size());
        wrefresh(inputWin);
    
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    running = false;
    tCommand.join();
    tEvent.join();

    delwin(logWin);
    delwin(inputWin);
    endwin();

    std::cout << "Exiting test DAQ server." << std::endl;
    return 0;
}

