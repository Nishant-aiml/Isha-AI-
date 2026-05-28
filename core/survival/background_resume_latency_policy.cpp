#include "background_resume_latency_policy.hpp"

namespace isha {

LatencyMitigations BackgroundResumeLatencyPolicy::evaluateResumeLatency(const WakeLatencyLogs& logs) {
    LatencyMitigations mitigations;
    
    double total_wake_latency = logs.tokenizer_wake_ms + 
                                logs.mmap_wake_ms + 
                                logs.checkpoint_restore_ms + 
                                logs.scheduler_resume_ms + 
                                logs.acceleration_reactivate_ms;

    if (total_wake_latency > MAX_ALLOWED_RESUME_LATENCY_MS) {
        mitigations.latency_compliant = false;
        mitigations.active_batch_cap = 2;                  // Drop active thread scheduling allocations
        mitigations.telemetry_frequency_ms = 60000;         // Reduce disk logs to prevent disk blocking
        mitigations.restrict_acceleration = true;           // CPU fallback is fast to load; avoid GPU wake wait
        mitigations.scale_down_checkpoint_writes = true;    // Write backups less frequently
    } else {
        mitigations.latency_compliant = true;
        mitigations.active_batch_cap = 16;
        mitigations.telemetry_frequency_ms = 5000;
        mitigations.restrict_acceleration = false;
        mitigations.scale_down_checkpoint_writes = false;
    }

    return mitigations;
}

} // namespace isha
