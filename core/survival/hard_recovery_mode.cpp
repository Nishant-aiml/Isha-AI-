#include "hard_recovery_mode.hpp"

namespace isha {

HardRecoveryConfig HardRecoveryMode::evaluateFailures(const HardFailureCounters& counters) {
    HardRecoveryConfig config;
    
    // Containment threshold trigger (5 or more repeated failures represents a recovery loop)
    bool loop_detected = (counters.safemode_failures >= 5) || 
                         (counters.startup_crashes >= 5) || 
                         (counters.corruption_recovery_failures >= 5) || 
                         (counters.watchdog_escalations >= 5) || 
                         (counters.mmap_reconstructions >= 5);

    if (loop_detected) {
        config.hard_recovery_active = true;
        config.force_cpu_only = true;                 // Break loops by forcing CPU only
        config.disable_optional_telemetry = true;
        config.minimal_scheduler_mode = true;         // Zero speculative queues
        config.minimal_background_activity = true;
        config.force_cache_purge = true;              // Purge dirty caches
        config.isolate_corrupt_model = true;          // Lock out GGUF model that fails to load
        config.recovery_only_startup_path = true;      // Fast boots directly to minimal safe UI
    } else {
        config.hard_recovery_active = false;
        config.force_cpu_only = false;
        config.disable_optional_telemetry = false;
        config.minimal_scheduler_mode = false;
        config.minimal_background_activity = false;
        config.force_cache_purge = false;
        config.isolate_corrupt_model = false;
        config.recovery_only_startup_path = false;
    }

    return config;
}

} // namespace isha
