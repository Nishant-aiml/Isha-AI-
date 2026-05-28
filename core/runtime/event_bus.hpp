#ifndef ISHA_AI_EVENT_BUS_HPP
#define ISHA_AI_EVENT_BUS_HPP

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <mutex>

namespace isha {

enum class EventType {
    LIFECYCLE_STARTUP,
    LIFECYCLE_HIBERNATE,
    LIFECYCLE_WAKE,
    LIFECYCLE_SHUTDOWN,
    MEMORY_PRESSURE_WARNING,
    MEMORY_PRESSURE_CRITICAL,
    MODEL_LOADING,
    MODEL_ACTIVE,
    MODEL_UNLOADING,
    MODEL_UNLOADED,
    MODEL_FAILED,
    SESSION_EXPIRED,
    SESSION_CLEANED,
    WATCHDOG_ALERT,
    INFERENCE_TIMEOUT,
    RETRIEVAL_COMPLETE,
    CONTEXT_OVERFLOW,
    AUDIO_FOCUS_LOST,
    AUDIO_FOCUS_GAINED,
    AUDIO_ROUTE_CHANGED,
    AUDIO_INTERRUPT_TRIGGERED,
    AUDIO_PIPELINE_STALL
};

inline std::string eventTypeToString(EventType type) {
    switch (type) {
        case EventType::LIFECYCLE_STARTUP: return "LIFECYCLE_STARTUP";
        case EventType::LIFECYCLE_HIBERNATE: return "LIFECYCLE_HIBERNATE";
        case EventType::LIFECYCLE_WAKE: return "LIFECYCLE_WAKE";
        case EventType::LIFECYCLE_SHUTDOWN: return "LIFECYCLE_SHUTDOWN";
        case EventType::MEMORY_PRESSURE_WARNING: return "MEMORY_PRESSURE_WARNING";
        case EventType::MEMORY_PRESSURE_CRITICAL: return "MEMORY_PRESSURE_CRITICAL";
        case EventType::MODEL_LOADING: return "MODEL_LOADING";
        case EventType::MODEL_ACTIVE: return "MODEL_ACTIVE";
        case EventType::MODEL_UNLOADING: return "MODEL_UNLOADING";
        case EventType::MODEL_UNLOADED: return "MODEL_UNLOADED";
        case EventType::MODEL_FAILED: return "MODEL_FAILED";
        case EventType::SESSION_EXPIRED: return "SESSION_EXPIRED";
        case EventType::SESSION_CLEANED: return "SESSION_CLEANED";
        case EventType::WATCHDOG_ALERT: return "WATCHDOG_ALERT";
        case EventType::INFERENCE_TIMEOUT: return "INFERENCE_TIMEOUT";
        case EventType::RETRIEVAL_COMPLETE: return "RETRIEVAL_COMPLETE";
        case EventType::CONTEXT_OVERFLOW: return "CONTEXT_OVERFLOW";
        case EventType::AUDIO_FOCUS_LOST: return "AUDIO_FOCUS_LOST";
        case EventType::AUDIO_FOCUS_GAINED: return "AUDIO_FOCUS_GAINED";
        case EventType::AUDIO_ROUTE_CHANGED: return "AUDIO_ROUTE_CHANGED";
        case EventType::AUDIO_INTERRUPT_TRIGGERED: return "AUDIO_INTERRUPT_TRIGGERED";
        case EventType::AUDIO_PIPELINE_STALL: return "AUDIO_PIPELINE_STALL";
        default: return "UNKNOWN";
    }
}

struct Event {
    EventType type;
    std::string sender;
    std::string data;
};

using EventCallback = std::function<void(const Event&)>;

class EventBus {
public:
    static EventBus& getInstance();

    void subscribe(EventType type, const std::string& listener_name, EventCallback callback);
    void unsubscribe(EventType type, const std::string& listener_name);
    void publish(const Event& event);

private:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    std::map<EventType, std::vector<std::pair<std::string, EventCallback>>> subscribers_;
    std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_EVENT_BUS_HPP
