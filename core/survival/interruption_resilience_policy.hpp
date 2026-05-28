#ifndef ISHA_AI_INTERRUPTION_RESILIENCE_POLICY_HPP
#define ISHA_AI_INTERRUPTION_RESILIENCE_POLICY_HPP

#include <string>

namespace isha {

enum class InterruptionEvent {
    INCOMING_CALL,
    NOTIFICATION_RECEIVED,
    BLUETOOTH_STATE_CHANGE,
    CAMERA_SESSION_START,
    SCREEN_ROTATION,
    SPLIT_SCREEN_ENGAGED,
    BATTERY_SAVER_ENABLED,
    AUDIO_FOCUS_LOSS
};

struct InterruptionResult {
    bool interruption_handled;
    bool pause_decode;
    bool serialize_immediate_checkpoint;
    bool scheduler_backoff_active;
    unsigned int pacing_delay_ms;
    bool enter_cooldown;
};

class InterruptionResiliencePolicy {
public:
    static std::string eventToString(InterruptionEvent event);

    // Processes critical phone interruption hooks safely and returns action parameters
    static InterruptionResult handleInterruption(InterruptionEvent event);
};

} // namespace isha

#endif // ISHA_AI_INTERRUPTION_RESILIENCE_POLICY_HPP
