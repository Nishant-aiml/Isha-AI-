#ifndef ISHA_AI_RUNTIME_CONFIG_HPP
#define ISHA_AI_RUNTIME_CONFIG_HPP

#include "../logging/logger.hpp"

namespace isha {

struct RuntimeConfig {
    LogLevel default_log_level = LogLevel::INFO;
    bool enable_telemetry = false;
    unsigned int session_expiration_seconds = 86400; // 24 hours
    unsigned int inactive_unload_timeout_seconds = 90; // 90 seconds
};

} // namespace isha

#endif // ISHA_AI_RUNTIME_CONFIG_HPP
