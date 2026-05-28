#include "storage_speed_classifier.hpp"
#include <chrono>
#include <fstream>
#include <vector>

namespace isha {

std::string StorageSpeedClassifier::tierToString(StorageSpeedTier tier) {
    switch (tier) {
        case StorageSpeedTier::EMMC_SLOW: return "eMMC Slow";
        case StorageSpeedTier::UFS2_MEDIUM: return "UFS 2.0/2.1 Medium";
        case StorageSpeedTier::UFS3_FAST: return "UFS 3.0+ Fast";
        default: return "UNKNOWN";
    }
}

StorageSpeedTier StorageSpeedClassifier::classifyStorage(const std::string& test_dir) {
    // Perform a tiny sequential write + read benchmark to detect speed category
    std::string test_file = test_dir + "/storage_speed_probe.bin";
    
    std::vector<char> dummy_data(1024 * 1024, 0xAA); // 1MB buffer
    
    auto start = std::chrono::steady_clock::now();
    
    std::ofstream out(test_file, std::ios::binary);
    if (!out) {
        // Fallback if directory permission is missing
        return StorageSpeedTier::EMMC_SLOW;
    }
    
    out.write(dummy_data.data(), dummy_data.size());
    out.flush();
    out.close();
    
    auto end = std::chrono::steady_clock::now();
    double write_duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Clean up
    std::remove(test_file.c_str());

    // eMMC write speed of 1MB typically takes > 10ms (100MB/s or slower)
    // UFS2 takes ~3-5ms (200-300MB/s)
    // UFS3 takes < 2ms (>500MB/s)
    if (write_duration_ms > 15.0) {
        return StorageSpeedTier::EMMC_SLOW;
    } else if (write_duration_ms > 5.0) {
        return StorageSpeedTier::UFS2_MEDIUM;
    } else {
        return StorageSpeedTier::UFS3_FAST;
    }
}

AdaptiveStoragePolicy StorageSpeedClassifier::getPolicy(StorageSpeedTier tier) {
    AdaptiveStoragePolicy policy;
    switch (tier) {
        case StorageSpeedTier::EMMC_SLOW:
            policy.mmap_prefetch_aggressive = false; // Never aggressively prefetch on eMMC to prevent lockups
            policy.optimal_batch_size = 8;           // Low thread pressure
            policy.telemetry_interval_ms = 60000;    // Long interval to avoid page swaps
            policy.enable_context_prefetch = false;
            policy.cache_cleanup_interval_sec = 1800; // Regular cleanup
            break;
            
        case StorageSpeedTier::UFS2_MEDIUM:
            policy.mmap_prefetch_aggressive = true;
            policy.optimal_batch_size = 16;
            policy.telemetry_interval_ms = 30000;
            policy.enable_context_prefetch = true;
            policy.cache_cleanup_interval_sec = 3600;
            break;
            
        case StorageSpeedTier::UFS3_FAST:
            policy.mmap_prefetch_aggressive = true;
            policy.optimal_batch_size = 32;
            policy.telemetry_interval_ms = 10000;
            policy.enable_context_prefetch = true;
            policy.cache_cleanup_interval_sec = 7200;
            break;
    }
    return policy;
}

} // namespace isha
