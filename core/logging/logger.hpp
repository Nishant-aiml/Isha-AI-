#ifndef ISHA_AI_LOGGER_HPP
#define ISHA_AI_LOGGER_HPP

#include <string>
#include <mutex>
#include <iostream>

#ifdef ERROR
#undef ERROR
#endif

namespace isha {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    DIAG
};

class Logger {
public:
    static Logger& getInstance();

    void setMinLevel(LogLevel level);
    void log(LogLevel level, const std::string& module, const std::string& message);

private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string levelToString(LogLevel level);

    LogLevel current_level_;
    std::mutex mutex_;
};

} // namespace isha

#define ISHA_LOG_DEBUG(module, msg) isha::Logger::getInstance().log(isha::LogLevel::DEBUG, module, msg)
#define ISHA_LOG_INFO(module, msg)  isha::Logger::getInstance().log(isha::LogLevel::INFO, module, msg)
#define ISHA_LOG_WARN(module, msg)  isha::Logger::getInstance().log(isha::LogLevel::WARN, module, msg)
#define ISHA_LOG_ERROR(module, msg) isha::Logger::getInstance().log(isha::LogLevel::ERROR, module, msg)
#define ISHA_LOG_DIAG(module, msg)  isha::Logger::getInstance().log(isha::LogLevel::DIAG, module, msg)

#endif // ISHA_AI_LOGGER_HPP
