#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <string>

enum LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void init(const std::string& logFilePath, LogLevel level = INFO) {
        std::lock_guard<std::mutex> lock(logMutex);
        if (!logFile.is_open()) {
            logFile.open(logFilePath, std::ios_base::app); // 以追加模式打开文件
            if (!logFile.is_open()) {
                std::cerr << "Failed to open log file: " << logFilePath << std::endl;
                return;
            }
        }
        currentLogLevel = level;
    }

    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex);
        currentLogLevel = level;
    }

    void log(const std::string& message, LogLevel level, const char* file, int line) {
        std::lock_guard<std::mutex> lock(logMutex);
        if (level < currentLogLevel) {
            return;
        }

        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm now_tm = *std::localtime(&now_time_t);
        std::ostringstream timestamp;
        timestamp << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

        std::string fileName = file;
        size_t lastSlash = fileName.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            fileName = fileName.substr(lastSlash + 1);
        }

        std::ostringstream logMessage;
        logMessage << "[" << timestamp.str() << "] [" << getLevelString(level) << "] [" << fileName << ":" << line << "] " << message;

        //std::cout << logMessage.str() << std::endl;

        if (logFile.is_open()) {
            logFile << logMessage.str() << std::endl;
        }
    }

private:
    Logger() : currentLogLevel(INFO) {}
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile;
    std::mutex logMutex;
    LogLevel currentLogLevel;

    const char* getLevelString(LogLevel level) const {
        switch (level) {
            case DEBUG: return "DEBUG";
            case INFO: return "INFO";
            case WARN: return "WARN";
            case ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
};

#define LOG(message, level) Logger::getInstance().log(message, level, __FILE__, __LINE__)
#define LOG_INFO(message) LOG(message, LogLevel::INFO)
#define LOG_ERROR(message) LOG(message, LogLevel::ERROR)
#define LOG_DEBUG(message) LOG(message, LogLevel::DEBUG)
#define LOG_WARN(message) LOG(message, LogLevel::WARN)
