#include "storage_fragmentation_guard.hpp"

namespace isha {

bool StorageFragmentationGuard::isThrottlingRequired(const StorageFragmentationMetrics& metrics) {
    // Flag throttle requirement if write speeds degrade or allocation churn is excessive
    if (metrics.nand_write_latency_ms > 100.0) {
        return true;
    }
    if (metrics.extraction_churn_cycles > 5) {
        return true;
    }
    if (metrics.fragmented_cache_files > 50) {
        return true;
    }
    if (metrics.temp_allocation_churn_count > 100) {
        return true;
    }
    if (metrics.repeated_partial_extraction_failures > 3) {
        return true;
    }
    return false;
}

FragmentationMitigation StorageFragmentationGuard::evaluateMitigations(const StorageFragmentationMetrics& metrics) {
    FragmentationMitigation mit = { false, false, false, false, false };
    if (isThrottlingRequired(metrics)) {
        mit.aggressive_cleanup = true;
        mit.cache_compaction = true;
        mit.purge_staging = true;
        mit.truncate_telemetry = true;
        mit.consolidate_checkpoints = true;
    }
    return mit;
}


} // namespace isha
