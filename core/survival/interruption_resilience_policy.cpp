#include "interruption_resilience_policy.hpp"

namespace isha {

std::string InterruptionResiliencePolicy::eventToString(InterruptionEvent event) {
    switch (event) {
        case InterruptionEvent::INCOMING_CALL: return "INCOMING_CALL";
        case InterruptionEvent::NOTIFICATION_RECEIVED: return "NOTIFICATION_RECEIVED";
        case InterruptionEvent::BLUETOOTH_STATE_CHANGE: return "BLUETOOTH_STATE_CHANGE";
        case InterruptionEvent::CAMERA_SESSION_START: return "CAMERA_SESSION_START";
        case InterruptionEvent::SCREEN_ROTATION: return "SCREEN_ROTATION";
        case InterruptionEvent::SPLIT_SCREEN_ENGAGED: return "SPLIT_SCREEN_ENGAGED";
        case InterruptionEvent::BATTERY_SAVER_ENABLED: return "BATTERY_SAVER_ENABLED";
        case InterruptionEvent::AUDIO_FOCUS_LOSS: return "AUDIO_FOCUS_LOSS";
        default: return "UNKNOWN";
    }
}

InterruptionResult InterruptionResiliencePolicy::handleInterruption(InterruptionEvent event) {
    InterruptionResult result;
    result.interruption_handled = true;
    result.pause_decode = false;
    result.serialize_immediate_checkpoint = false;
    result.scheduler_backoff_active = false;
    result.pacing_delay_ms = 0;
    result.enter_cooldown = false;

    switch (event) {
        case InterruptionEvent::INCOMING_CALL:
            // Immediate pause and checkpoint serialization before OEM kill threat spikes
            result.pause_decode = true;
            result.serialize_immediate_checkpoint = true;
            result.scheduler_backoff_active = true;
            result.pacing_delay_ms = 5000; // Hold queue for 5 seconds
            result.enter_cooldown = true;
            break;
            
        case InterruptionEvent::CAMERA_SESSION_START:
            // Camera consumes immense GPU/DSP memory: pause and drop accelerator
            result.pause_decode = true;
            result.serialize_immediate_checkpoint = true;
            result.enter_cooldown = true;
            break;

        case InterruptionEvent::AUDIO_FOCUS_LOSS:
            // Slow down active decoding pacing slightly
            result.scheduler_backoff_active = true;
            result.pacing_delay_ms = 500;
            break;

        case InterruptionEvent::BATTERY_SAVER_ENABLED:
            result.scheduler_backoff_active = true;
            result.pacing_delay_ms = 200;
            result.enter_cooldown = true;
            break;

        case InterruptionEvent::SCREEN_ROTATION:
        case InterruptionEvent::SPLIT_SCREEN_ENGAGED:
            // UI geometry updates: serialize checkpoint to survive configuration changes
            result.serialize_immediate_checkpoint = true;
            break;

        default:
            break;
    }

    return result;
}

} // namespace isha
