#include "survival_priority_mode.hpp"

namespace isha {

ResourcePolicy SurvivalPriorityMode::evaluateSystemMetrics(const DeviceMetrics& metrics) {
    ResourcePolicy policy;
    
    // Check if any critical trigger is hit
    bool trigger_hit = (metrics.thermal_temp_c > 41.0) || 
                       (metrics.fragmentation_ratio > 0.6) || 
                       metrics.anr_risk_threat || 
                       metrics.scheduler_stalled || 
                       metrics.storage_stalled || 
                       (metrics.battery_drain_rate > 10.0) || 
                       metrics.oem_restrictions_tight;

    if (trigger_hit) {
        policy.survival_mode_active = true;
        policy.optimal_batch_size = 4;           // Minimal thread pressure to reduce thermal load
        policy.max_context_cells = 256;          // Drop context size to minimize RAM footprint
        policy.telemetry_interval_ms = 60000;    // Drastically scale down logging
        policy.scheduler_cool_down_ms = 1500;    // Insert pacing delay to prevent thread saturation
        policy.use_acceleration = false;         // Force CPU fallback for safety
    } else {
        policy.survival_mode_active = false;
        policy.optimal_batch_size = 16;
        policy.max_context_cells = 2048;
        policy.telemetry_interval_ms = 5000;
        policy.scheduler_cool_down_ms = 0;
        policy.use_acceleration = true;
    }
    
    return policy;
}

} // namespace isha
