#ifndef ISHA_AI_UI_STARVATION_GUARD_HPP
#define ISHA_AI_UI_STARVATION_GUARD_HPP

#include <string>

namespace isha {

struct StarvationMetrics {
    double frame_stall_ms;
    bool excessive_jni_contention;
    bool ui_scheduler_starved;
    bool decode_monopolized;
    double touch_response_delay_ms;
    double render_loop_stall_ms;
};

struct StarvationAction {
    bool starvation_threat_active;
    unsigned int capped_batch_size;
    unsigned int telemetry_frequency_ms;
    bool pause_background_work;
    unsigned int scheduler_cool_down_ms;
    bool force_cpu_fallback;
    unsigned int checkpoint_frequency_ms;
};

class UIStarvationGuard {
public:
    // Analyzes UI scheduler state and JNI latency parameters
    static StarvationAction evaluateStarvation(const StarvationMetrics& metrics);
};

} // namespace isha

#endif // ISHA_AI_UI_STARVATION_GUARD_HPP
