#include "ui_starvation_guard.hpp"

namespace isha {

StarvationAction UIStarvationGuard::evaluateStarvation(const StarvationMetrics& metrics) {
    StarvationAction action;
    
    // Evaluate if user UI main-thread latency bounds are violated (stalls > 100ms represent visible lag/jank)
    bool starved = (metrics.frame_stall_ms > 100.0) || 
                   metrics.excessive_jni_contention || 
                   metrics.ui_scheduler_starved || 
                   metrics.decode_monopolized || 
                   (metrics.touch_response_delay_ms > 150.0) || 
                   (metrics.render_loop_stall_ms > 100.0);

    if (starved) {
        action.starvation_threat_active = true;
        action.capped_batch_size = 2;                  // Cap active batches immediately to free compute cores
        action.telemetry_frequency_ms = 60000;         // Slow logging to background thread bounds
        action.pause_background_work = true;           // Suspend low priority jobs
        action.scheduler_cool_down_ms = 1000;          // Insert 1 second pacing delay to cool thread schedulers
        action.force_cpu_fallback = true;              // Fallback to CPU to avoid competing for accelerator contexts
        action.checkpoint_frequency_ms = 300000;       // Limit state checkpoint writes to 5-minute intervals
    } else {
        action.starvation_threat_active = false;
        action.capped_batch_size = 16;
        action.telemetry_frequency_ms = 5000;
        action.pause_background_work = false;
        action.scheduler_cool_down_ms = 0;
        action.force_cpu_fallback = false;
        action.checkpoint_frequency_ms = 30000;
    }

    return action;
}

} // namespace isha
