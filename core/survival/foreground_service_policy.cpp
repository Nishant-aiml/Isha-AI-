#include "foreground_service_policy.hpp"

namespace isha {

std::string ForegroundServicePolicy::bucketToString(AppStandbyBucket bucket) {
    switch (bucket) {
        case AppStandbyBucket::ACTIVE: return "ACTIVE";
        case AppStandbyBucket::WORKING_SET: return "WORKING_SET";
        case AppStandbyBucket::FREQUENT: return "FREQUENT";
        case AppStandbyBucket::RARE: return "RARE";
        case AppStandbyBucket::RESTRICTED: return "RESTRICTED";
        default: return "UNKNOWN";
    }
}

ExecutionParameters ForegroundServicePolicy::evaluateConstraints(const BackgroundConstraints& constraints) {
    ExecutionParameters params;
    
    // Default safe modes
    params.run_as_foreground_service = true;
    params.persistence_notification_active = true;
    params.telemetry_frequency_ms = 10000;
    params.context_size_cells = 2048;
    params.allow_heavy_inference = true;

    // 1. Check Doze Mode restrictions
    if (constraints.is_doze_mode_active) {
        params.allow_heavy_inference = false;
        params.context_size_cells = 256;      // Drastically scale context down to survive memory sweeps
        params.telemetry_frequency_ms = 120000; // Drop telemetry writes to avoid wakeup triggers
    }

    // 2. Check OEM background kill threats (e.g. Xiaomi, Huawei, Samsung A-series)
    if (constraints.is_oem_background_kill_aggressive) {
        params.telemetry_frequency_ms = 60000;
        params.context_size_cells = 512;       // Shrink size to bypass low memory targets
    }

    // 3. Handle restricted buckets
    if (constraints.standby_bucket == AppStandbyBucket::RARE || 
        constraints.standby_bucket == AppStandbyBucket::RESTRICTED) {
        params.allow_heavy_inference = false;
        params.context_size_cells = 256;
    }

    return params;
}

} // namespace isha
