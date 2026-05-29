#include "survival/storage_fragmentation_guard.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Starting storage_fragmentation_guard_benchmark..." << std::endl;

    // Stable condition
    isha::StorageFragmentationMetrics normal = { 1, 5, 10, 15.0, 0 };
    bool normal_throttle = isha::StorageFragmentationGuard::isThrottlingRequired(normal);
    assert(!normal_throttle);

    // Wear condition
    isha::StorageFragmentationMetrics heavy = { 6, 52, 120, 120.0, 4 };
    bool heavy_throttle = isha::StorageFragmentationGuard::isThrottlingRequired(heavy);
    assert(heavy_throttle);

    auto mit = isha::StorageFragmentationGuard::evaluateMitigations(heavy);
    assert(mit.aggressive_cleanup == true);
    assert(mit.cache_compaction == true);
    assert(mit.consolidate_checkpoints == true);

    std::cout << "storage_fragmentation_guard_benchmark PASSED!" << std::endl;
    return 0;
}

