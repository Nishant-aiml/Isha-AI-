#ifndef ISHA_AI_STORAGE_FRAGMENTATION_GUARD_HPP
#define ISHA_AI_STORAGE_FRAGMENTATION_GUARD_HPP

namespace isha {

struct StorageFragmentationMetrics {
    int extraction_churn_cycles;
    int fragmented_cache_files;
    int temp_allocation_churn_count;
    double nand_write_latency_ms;
    int repeated_partial_extraction_failures;
};

struct FragmentationMitigation {
    bool aggressive_cleanup;
    bool cache_compaction;
    bool purge_staging;
    bool truncate_telemetry;
    bool consolidate_checkpoints;
};

class StorageFragmentationGuard {
public:
    // Checks if storage optimization or churn throttling is required
    static bool isThrottlingRequired(const StorageFragmentationMetrics& metrics);

    // Determines active mitigation responses
    static FragmentationMitigation evaluateMitigations(const StorageFragmentationMetrics& metrics);
};


} // namespace isha

#endif // ISHA_AI_STORAGE_FRAGMENTATION_GUARD_HPP
