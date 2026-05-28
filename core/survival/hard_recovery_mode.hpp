#ifndef ISHA_AI_HARD_RECOVERY_MODE_HPP
#define ISHA_AI_HARD_RECOVERY_MODE_HPP

#include <string>

namespace isha {

struct HardFailureCounters {
    unsigned int safemode_failures;
    unsigned int startup_crashes;
    unsigned int corruption_recovery_failures;
    unsigned int watchdog_escalations;
    unsigned int mmap_reconstructions;
};

struct HardRecoveryConfig {
    bool hard_recovery_active;
    bool force_cpu_only;
    bool disable_optional_telemetry;
    bool minimal_scheduler_mode;
    bool minimal_background_activity;
    bool force_cache_purge;
    bool isolate_corrupt_model;
    bool recovery_only_startup_path;
};

class HardRecoveryMode {
public:
    // Decides if system should escalate to hard recovery containment mode
    static HardRecoveryConfig evaluateFailures(const HardFailureCounters& counters);
};

} // namespace isha

#endif // ISHA_AI_HARD_RECOVERY_MODE_HPP
