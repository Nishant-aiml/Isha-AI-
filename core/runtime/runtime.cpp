#include "runtime.hpp"
#include "event_bus.hpp"
#include "../logging/logger.hpp"

namespace isha {

Runtime::Runtime(const RuntimeConfig& config, const DeviceProfile& profile)
    : config_(config), profile_(profile), is_running_(false), is_hibernating_(false) {
    Logger::getInstance().setMinLevel(config_.default_log_level);
}

Runtime::~Runtime() {
    if (is_running_) {
        shutdown();
    }
}

void Runtime::start() {
    if (is_running_) return;
    is_running_ = true;
    is_hibernating_ = false;
    ISHA_LOG_INFO("Runtime", "Starting offline runtime skeleton for device tier: " + deviceTierToString(profile_.tier));
    EventBus::getInstance().publish({EventType::LIFECYCLE_STARTUP, "Runtime", "Startup completed"});
}

void Runtime::hibernate() {
    if (!is_running_ || is_hibernating_) return;
    is_hibernating_ = true;
    ISHA_LOG_INFO("Runtime", "Entering hibernation mode. Releasing resource handlers.");
    EventBus::getInstance().publish({EventType::LIFECYCLE_HIBERNATE, "Runtime", "Entering hibernation"});
}

void Runtime::wake() {
    if (!is_running_ || !is_hibernating_) return;
    is_hibernating_ = false;
    ISHA_LOG_INFO("Runtime", "Waking up runtime from hibernation.");
    EventBus::getInstance().publish({EventType::LIFECYCLE_WAKE, "Runtime", "Woken from hibernation"});
}

void Runtime::shutdown() {
    if (!is_running_) return;
    ISHA_LOG_INFO("Runtime", "Shutting down offline runtime.");
    EventBus::getInstance().publish({EventType::LIFECYCLE_SHUTDOWN, "Runtime", "Shutdown initiated"});
    is_running_ = false;
    is_hibernating_ = false;
}

std::string Runtime::processRequest(const std::string& query) {
    if (!is_running_) {
        ISHA_LOG_ERROR("Runtime", "Cannot process request: Runtime is not running.");
        return "ERROR: Runtime not running";
    }
    if (is_hibernating_) {
        wake();
    }
    ISHA_LOG_INFO("Runtime", "Processing request: " + query);
    return "MOCK_RESPONSE: Grounded answer to '" + query + "'";
}

} // namespace isha
