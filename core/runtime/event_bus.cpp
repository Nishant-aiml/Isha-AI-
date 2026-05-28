#include "event_bus.hpp"
#include "../logging/logger.hpp"

namespace isha {

EventBus& EventBus::getInstance() {
    static EventBus instance;
    return instance;
}

void EventBus::subscribe(EventType type, const std::string& listener_name, EventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_[type].push_back({listener_name, callback});
    ISHA_LOG_DEBUG("EventBus", "Listener '" + listener_name + "' subscribed to event: " + eventTypeToString(type));
}

void EventBus::unsubscribe(EventType type, const std::string& listener_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscribers_.find(type);
    if (it != subscribers_.end()) {
        auto& list = it->second;
        for (auto list_it = list.begin(); list_it != list.end(); ) {
            if (list_it->first == listener_name) {
                list_it = list.erase(list_it);
                ISHA_LOG_DEBUG("EventBus", "Listener '" + listener_name + "' unsubscribed from event: " + eventTypeToString(type));
            } else {
                ++list_it;
            }
        }
    }
}

void EventBus::publish(const Event& event) {
    std::vector<EventCallback> callbacks_to_invoke;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscribers_.find(event.type);
        if (it != subscribers_.end()) {
            for (const auto& pair : it->second) {
                callbacks_to_invoke.push_back(pair.second);
            }
        }
    }

    ISHA_LOG_DEBUG("EventBus", "Publishing event: " + eventTypeToString(event.type) + " from '" + event.sender + "'");
    for (const auto& cb : callbacks_to_invoke) {
        if (cb) {
            cb(event);
        }
    }
}

} // namespace isha
