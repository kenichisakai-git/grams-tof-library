#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include <mutex>

#include <fmt/format.h>

namespace LogColor {
    constexpr auto RESET   = "\033[0m";
    constexpr auto RED     = "\033[31m";
    constexpr auto GREEN   = "\033[32m";
    constexpr auto YELLOW  = "\033[33m";
    constexpr auto BLUE    = "\033[34m";
    constexpr auto GRAY    = "\033[90m";
    constexpr auto WHITE   = "\033[97m"; 
    constexpr auto CYAN    = "\033[36m";   
}

class Logger {
public:
    enum class Level { Info, Warning, Error, Debug, Detail };

    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void setLogFile(const std::string& logfile) {
        std::lock_guard<std::mutex> lock(mutex_);
        logFile_ = std::make_unique<std::ofstream>(logfile);
    }

    template<typename... Args>
    void log(Level level, const std::string& fmt_str, Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        const char* color = getColor(level);
        const char* label = getLabel(level);

        std::ostringstream formatted;
        formatted << label << ": " << fmt::format(fmt_str, std::forward<Args>(args)...) << "\n";

        // Output to terminal
        std::cout << color << formatted.str() << LogColor::RESET;

        // Output to file
        if (logFile_ && logFile_->is_open()) {
            (*logFile_) << formatted.str();
        }
    }

    // Shorthand
    template<typename... Args>
    void info(const std::string& fmt, Args&&... args) {
        log(Level::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const std::string& fmt, Args&&... args) {
        log(Level::Warning, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const std::string& fmt, Args&&... args) {
        log(Level::Error, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const std::string& fmt, Args&&... args) {
        log(Level::Debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void detail(const std::string& fmt, Args&&... args) {
        log(Level::Detail, fmt, std::forward<Args>(args)...);
    }

private:
    Logger() = default;

    const char* getColor(Level level) const {
        using namespace LogColor;
        switch (level) {
            case Level::Info:    return WHITE; //GRAY;
            case Level::Warning: return YELLOW;
            case Level::Error:   return RED;
            case Level::Debug:   return GREEN;
            case Level::Detail:  return CYAN; //BLUE;
        }
        return RESET;
    }

    const char* getLabel(Level level) const {
        switch (level) {
            case Level::Info:    return "[INFO]";
            case Level::Warning: return "[WARN]";
            case Level::Error:   return "[ERROR]";
            case Level::Debug:   return "[DEBUG]";
            case Level::Detail:  return "[DETAIL]";
        }
        return "[LOG]";
    }

    std::unique_ptr<std::ofstream> logFile_;
    std::mutex mutex_;
};

