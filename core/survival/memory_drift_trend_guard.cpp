#include "memory_drift_trend_guard.hpp"

namespace isha {

DriftAction MemoryDriftTrendGuard::evaluateTrend(const MemoryTrend& trend) {
    DriftAction action;
    action.drift_detected = false;
    action.trigger_cache_cleanup = false;
    action.reduce_telemetry = false;
    action.scale_down_scheduler = false;
    action.clean_checkpoints = false;
    action.escalate_degradation = false;

    // Detect monotonic memory leak / RSS drift (size threshold minimum 4 datapoints)
    if (trend.rss_history_mb.size() >= 4) {
        bool monotonic_growth = true;
        for (size_t i = 1; i < trend.rss_history_mb.size(); ++i) {
            if (trend.rss_history_mb[i] <= trend.rss_history_mb[i - 1]) {
                monotonic_growth = false;
                break;
            }
        }
        
        // Ensure the growth is significant, not simple minor fluctuations
        double total_drift = trend.rss_history_mb.back() - trend.rss_history_mb.front();
        if (monotonic_growth && total_drift > 50.0) { // Significant growth > 50MB
            action.drift_detected = true;
            action.trigger_cache_cleanup = true;     // Flush KV and model layers
            action.reduce_telemetry = true;
            action.scale_down_scheduler = true;
            action.clean_checkpoints = true;
            action.escalate_degradation = true;      // Escalate to next safety tier to shrink context limits
        }
    }

    // Secondary checks: abnormal cache/checkpoint sizes
    if (trend.accumulated_cache_mb > 250.0 || trend.accumulated_checkpoints_mb > 100.0) {
        action.drift_detected = true;
        action.trigger_cache_cleanup = true;
        action.clean_checkpoints = true;
    }

    return action;
}

} // namespace isha
