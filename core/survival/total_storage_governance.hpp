#ifndef ISHA_AI_TOTAL_STORAGE_GOVERNANCE_HPP
#define ISHA_AI_TOTAL_STORAGE_GOVERNANCE_HPP

#include <string>

namespace isha {

struct FootprintMetrics {
    double apk_size_mb;
    double models_size_mb;
    double mmap_snapshots_size_mb;
    double caches_size_mb;
    double telemetry_size_mb;
    double temp_extraction_size_mb;
    double checkpoints_size_mb;
    double session_history_size_mb;
};

struct GovernanceResponse {
    bool limit_approached;
    bool purge_stale_telemetry;
    bool purge_stale_temp;
    bool compress_inactive_sessions;
    bool cleanup_obsolete_models;
    bool reduce_cache_retention;
    bool cleanup_abandoned_recovery;
};

class TotalStorageGovernance {
public:
    static constexpr double TOTAL_RUNTIME_STORAGE_CAP_MB = 1000.0; // 1GB hard allocation cap

    // Checks cumulative disk usage and returns appropriate reclamation triggers
    static GovernanceResponse evaluateTotalFootprint(const FootprintMetrics& metrics);
};

} // namespace isha

#endif // ISHA_AI_TOTAL_STORAGE_GOVERNANCE_HPP
