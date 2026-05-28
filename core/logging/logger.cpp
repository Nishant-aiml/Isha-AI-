#include "logger.hpp"
#include <chrono>
#include <iomanip>
#include <ctime>

namespace isha {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : current_level_(LogLevel::INFO) {}

void Logger::setMinLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_level_ = level;
}

void Logger::log(LogLevel level, const std::string& module, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < current_level_) return;

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // Thread-safe localtime conversion for Windows
    struct tm time_info;
#if defined(_WIN32)
    localtime_s(&time_info, &time_t_now);
#else
    localtime_r(&time_t_now, &time_info);
#endif

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;

    std::cout << "[" << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S")
              << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
              << "[" << levelToString(level) << "] "
              << "[" << module << "] "
              << message << std::endl;
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::DIAG:  return "DIAG";
        default: return "UNKNOWN";
    }
}

} // namespace isha
