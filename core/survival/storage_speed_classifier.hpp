#ifndef ISHA_AI_STORAGE_SPEED_CLASSIFIER_HPP
#define ISHA_AI_STORAGE_SPEED_CLASSIFIER_HPP

#include <string>

namespace isha {

enum class StorageSpeedTier {
    EMMC_SLOW,
    UFS2_MEDIUM,
    UFS3_FAST
};

struct AdaptiveStoragePolicy {
    bool mmap_prefetch_aggressive;
    unsigned int optimal_batch_size;
    unsigned int telemetry_interval_ms;
    bool enable_context_prefetch;
    unsigned int cache_cleanup_interval_sec;
};

class StorageSpeedClassifier {
public:
    static std::string tierToString(StorageSpeedTier tier);

    // Measure disk performance and classify storage tier
    static StorageSpeedTier classifyStorage(const std::string& test_dir);

    // Derive policy based on classified tier
    static AdaptiveStoragePolicy getPolicy(StorageSpeedTier tier);
};

} // namespace isha

#endif // ISHA_AI_STORAGE_SPEED_CLASSIFIER_HPP
