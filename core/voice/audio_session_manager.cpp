#include "audio_session_manager.hpp"
#include "../runtime/event_bus.hpp"

namespace isha {

bool AudioSessionManager::requestMicrophoneFocus() {
    std::lock_guard<std::mutex> lock(mutex_);
    has_focus_.store(true);
    mic_active_.store(true);
    
    EventBus::getInstance().publish({
        EventType::AUDIO_FOCUS_GAINED,
        "AudioSessionManager",
        "route=" + current_route_
    });
    return true;
}

void AudioSessionManager::abandonMicrophoneFocus() {
    std::lock_guard<std::mutex> lock(mutex_);
    has_focus_.store(false);
    mic_active_.store(false);
    
    EventBus::getInstance().publish({
        EventType::AUDIO_FOCUS_LOST,
        "AudioSessionManager",
        "abandoned"
    });
}

void AudioSessionManager::setPauseOnFocusLoss(bool pause) {
    pause_on_loss_.store(pause);
}

bool AudioSessionManager::getPauseOnFocusLoss() const {
    return pause_on_loss_.load();
}

void AudioSessionManager::setRoute(const std::string& route) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_route_ != route) {
        current_route_ = route;
        EventBus::getInstance().publish({
            EventType::AUDIO_ROUTE_CHANGED,
            "AudioSessionManager",
            "new_route=" + route
        });
    }
}

std::string AudioSessionManager::getRoute() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_route_;
}

bool AudioSessionManager::isMicrophoneActive() const {
    return mic_active_.load();
}

bool AudioSessionManager::hasAudioFocus() const {
    return has_focus_.load();
}

void AudioSessionManager::simulateFocusLoss() {
    std::lock_guard<std::mutex> lock(mutex_);
    has_focus_.store(false);
    if (pause_on_loss_.load()) {
        mic_active_.store(false);
    }
    
    EventBus::getInstance().publish({
        EventType::AUDIO_FOCUS_LOST,
        "AudioSessionManager",
        "simulated_loss"
    });
}

void AudioSessionManager::simulateFocusGain() {
    std::lock_guard<std::mutex> lock(mutex_);
    has_focus_.store(true);
    mic_active_.store(true);
    
    EventBus::getInstance().publish({
        EventType::AUDIO_FOCUS_GAINED,
        "AudioSessionManager",
        "simulated_gain"
    });
}

void AudioSessionManager::simulateRouteChanged(const std::string& new_route) {
    setRoute(new_route);
}

} // namespace isha
