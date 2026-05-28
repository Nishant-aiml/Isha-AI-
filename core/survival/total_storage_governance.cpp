#include "total_storage_governance.hpp"

namespace isha {

GovernanceResponse TotalStorageGovernance::evaluateTotalFootprint(const FootprintMetrics& metrics) {
    GovernanceResponse response;
    
    double total_footprint = metrics.apk_size_mb + 
                             metrics.models_size_mb + 
                             metrics.mmap_snapshots_size_mb + 
                             metrics.caches_size_mb + 
                             metrics.telemetry_size_mb + 
                             metrics.temp_extraction_size_mb + 
                             metrics.checkpoints_size_mb + 
                             metrics.session_history_size_mb;

    // Check if total disk size approaches 90% of our hard cap (900MB)
    if (total_footprint > TOTAL_RUNTIME_STORAGE_CAP_MB * 0.9) {
        response.limit_approached = true;
        response.purge_stale_telemetry = true;
        response.purge_stale_temp = true;
        response.compress_inactive_sessions = true;
        response.cleanup_obsolete_models = true;
        response.reduce_cache_retention = true;
        response.cleanup_abandoned_recovery = true;
    } else if (total_footprint > TOTAL_RUNTIME_STORAGE_CAP_MB * 0.7) {
        // Warning threshold (700MB) -> clean temp files and telemetry safely
        response.limit_approached = true;
        response.purge_stale_telemetry = true;
        response.purge_stale_temp = true;
        response.compress_inactive_sessions = false;
        response.cleanup_obsolete_models = false;
        response.reduce_cache_retention = false;
        response.cleanup_abandoned_recovery = true;
    } else {
        response.limit_approached = false;
        response.purge_stale_telemetry = false;
        response.purge_stale_temp = false;
        response.compress_inactive_sessions = false;
        response.cleanup_obsolete_models = false;
        response.reduce_cache_retention = false;
        response.cleanup_abandoned_recovery = false;
    }

    return response;
}

} // namespace isha
