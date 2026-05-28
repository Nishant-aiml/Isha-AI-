#ifndef ISHA_AI_MEMORY_DRIFT_TREND_GUARD_HPP
#define ISHA_AI_MEMORY_DRIFT_TREND_GUARD_HPP

#include <string>
#include <vector>

namespace isha {

struct MemoryTrend {
    std::vector<double> rss_history_mb;
    std::vector<double> fragmentation_history;
    double current_mmap_pressure_mb;
    double accumulated_cache_mb;
    double accumulated_checkpoints_mb;
    double scheduler_latency_drift_ms;
};

struct DriftAction {
    bool drift_detected;
    bool trigger_cache_cleanup;
    bool reduce_telemetry;
    bool scale_down_scheduler;
    bool clean_checkpoints;
    bool escalate_degradation;
};

class MemoryDriftTrendGuard {
public:
    // Examines memory usage histories to identify monotonic growth patterns
    static DriftAction evaluateTrend(const MemoryTrend& trend);
};

} // namespace isha

#endif // ISHA_AI_MEMORY_DRIFT_TREND_GUARD_HPP
