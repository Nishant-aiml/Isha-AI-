#ifndef ISHA_AI_BACKGROUND_RESUME_LATENCY_POLICY_HPP
#define ISHA_AI_BACKGROUND_RESUME_LATENCY_POLICY_HPP

#include <string>

namespace isha {

struct WakeLatencyLogs {
    double tokenizer_wake_ms;
    double mmap_wake_ms;
    double checkpoint_restore_ms;
    double scheduler_resume_ms;
    double acceleration_reactivate_ms;
};

struct LatencyMitigations {
    bool latency_compliant;
    unsigned int active_batch_cap;
    unsigned int telemetry_frequency_ms;
    bool restrict_acceleration;
    bool scale_down_checkpoint_writes;
};

class BackgroundResumeLatencyPolicy {
public:
    static constexpr double MAX_ALLOWED_RESUME_LATENCY_MS = 2000.0; // 2 seconds hard budget limit

    // Compares total wake latencies against the 2 second target and scales down parameters on sluggish wakeups
    static LatencyMitigations evaluateResumeLatency(const WakeLatencyLogs& logs);
};

} // namespace isha

#endif // ISHA_AI_BACKGROUND_RESUME_LATENCY_POLICY_HPP
