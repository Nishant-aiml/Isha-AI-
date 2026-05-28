#include "camera_session_manager.hpp"
#include "../logging/logger.hpp"

namespace isha {

CameraSessionManager::CameraSessionManager() {
    ISHA_LOG_INFO("CameraSession", "Initialized.");
}

bool CameraSessionManager::openSession() {
    std::lock_guard<std::mutex> lock(mutex_);
    CameraState current = state_.load(std::memory_order_acquire);

    // Only allow opening from IDLE
    if (current != CameraState::IDLE) {
        ISHA_LOG_WARN("CameraSession", "Cannot open session from state: " + stateToString(current));
        return false;
    }

    state_.store(CameraState::OPENING, std::memory_order_release);
    ISHA_LOG_INFO("CameraSession", "Session opening...");

    // Simulated open: transition directly to ACTIVE
    state_.store(CameraState::ACTIVE, std::memory_order_release);
    session_count_.fetch_add(1, std::memory_order_relaxed);
    ISHA_LOG_INFO("CameraSession", "Session active. Total sessions: " +
                  std::to_string(session_count_.load()));
    return true;
}

void CameraSessionManager::closeSession() {
    std::lock_guard<std::mutex> lock(mutex_);
    CameraState current = state_.load(std::memory_order_acquire);

    if (current == CameraState::IDLE) {
        ISHA_LOG_DEBUG("CameraSession", "Already idle, nothing to close.");
        return;
    }

    ISHA_LOG_INFO("CameraSession", "Closing session from state: " + stateToString(current));
    state_.store(CameraState::IDLE, std::memory_order_release);
}

void CameraSessionManager::pauseSession() {
    std::lock_guard<std::mutex> lock(mutex_);
    CameraState current = state_.load(std::memory_order_acquire);

    if (current != CameraState::ACTIVE) {
        ISHA_LOG_WARN("CameraSession", "Cannot pause from state: " + stateToString(current));
        return;
    }

    state_.store(CameraState::PAUSED, std::memory_order_release);
    ISHA_LOG_INFO("CameraSession", "Session paused (background transition).");
}

bool CameraSessionManager::resumeSession() {
    std::lock_guard<std::mutex> lock(mutex_);
    CameraState current = state_.load(std::memory_order_acquire);

    if (current != CameraState::PAUSED) {
        ISHA_LOG_WARN("CameraSession", "Cannot resume from state: " + stateToString(current));
        return false;
    }

    state_.store(CameraState::ACTIVE, std::memory_order_release);
    ISHA_LOG_INFO("CameraSession", "Session resumed (foreground transition).");
    return true;
}

void CameraSessionManager::handlePermissionRevoke() {
    std::lock_guard<std::mutex> lock(mutex_);
    CameraState current = state_.load(std::memory_order_acquire);

    ISHA_LOG_WARN("CameraSession", "Permission revoked from state: " + stateToString(current));
    state_.store(CameraState::PERMISSION_REVOKED, std::memory_order_release);
    revoke_count_.fetch_add(1, std::memory_order_relaxed);
}

void CameraSessionManager::handleResourceConflict() {
    std::lock_guard<std::mutex> lock(mutex_);
    CameraState current = state_.load(std::memory_order_acquire);

    ISHA_LOG_WARN("CameraSession", "Resource conflict from state: " + stateToString(current));
    state_.store(CameraState::RESOURCE_CONFLICT, std::memory_order_release);
}

bool CameraSessionManager::tryRecoverSession() {
    std::lock_guard<std::mutex> lock(mutex_);
    CameraState current = state_.load(std::memory_order_acquire);

    // Only recover from ERROR or RESOURCE_CONFLICT
    if (current != CameraState::ERROR && current != CameraState::RESOURCE_CONFLICT) {
        ISHA_LOG_WARN("CameraSession", "Cannot recover from state: " + stateToString(current));
        return false;
    }

    ISHA_LOG_INFO("CameraSession", "Attempting recovery from: " + stateToString(current));

    // Simulated recovery: go to IDLE so openSession() can be called again
    state_.store(CameraState::IDLE, std::memory_order_release);
    recovery_count_.fetch_add(1, std::memory_order_relaxed);
    ISHA_LOG_INFO("CameraSession", "Recovery successful. Total recoveries: " +
                  std::to_string(recovery_count_.load()));
    return true;
}

CameraState CameraSessionManager::getState() const {
    return state_.load(std::memory_order_acquire);
}

std::string CameraSessionManager::stateToString(CameraState state) const {
    switch (state) {
        case CameraState::IDLE:                return "IDLE";
        case CameraState::OPENING:             return "OPENING";
        case CameraState::ACTIVE:              return "ACTIVE";
        case CameraState::PAUSED:              return "PAUSED";
        case CameraState::ERROR:               return "ERROR";
        case CameraState::PERMISSION_REVOKED:  return "PERMISSION_REVOKED";
        case CameraState::RESOURCE_CONFLICT:   return "RESOURCE_CONFLICT";
        default:                               return "UNKNOWN";
    }
}

} // namespace isha
