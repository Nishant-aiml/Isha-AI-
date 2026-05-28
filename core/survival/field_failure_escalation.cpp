#include "field_failure_escalation.hpp"

namespace isha {

EscalationPolicy FieldFailureEscalation::evaluateFailureRates(const EscalationCounters& counters) {
    EscalationPolicy policy;
    
    // Check if any repeated severe loop exists (threshold 3 incidents)
    bool repeated_safemode = counters.safemode_count >= 3;
    bool repeated_thermal = counters.thermal_collapse_count >= 3;
    bool repeated_anrs = counters.anr_count >= 3;
    bool repeated_startup = counters.startup_failure_count >= 3;
    bool repeated_watchdog = counters.watchdog_escalation_count >= 3;
    bool repeated_corruption = counters.corruption_recovery_count >= 3;
    bool repeated_background = counters.background_failure_count >= 3;

    bool critical = repeated_safemode || repeated_thermal || repeated_anrs || 
                    repeated_startup || repeated_watchdog || repeated_corruption || 
                    repeated_background;

    if (critical) {
        policy.is_field_critical = true;
        policy.disable_acceleration = true;      // Absolute safe CPU fallback
        policy.limit_context_size = 256;          // Bare minimum context
        policy.limit_batch_size = 1;              // Force single token serial processing
        policy.telemetry_interval_ms = 300000;    // Shut down telemetry noise (5 min)
        policy.watchdog_polling_ms = 10000;       // Slow down watchdog activity
        policy.force_survival_mode = true;
        policy.recommend_reboot = (counters.anr_count >= 5 || counters.thermal_collapse_count >= 5);
    } else {
        policy.is_field_critical = false;
        policy.disable_acceleration = false;
        policy.limit_context_size = 2048;
        policy.limit_batch_size = 16;
        policy.telemetry_interval_ms = 5000;
        policy.watchdog_polling_ms = 100;
        policy.force_survival_mode = false;
        policy.recommend_reboot = false;
    }

    return policy;
}

} // namespace isha
