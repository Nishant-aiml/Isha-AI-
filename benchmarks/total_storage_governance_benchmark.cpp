#include <iostream>
#include <cassert>
#include "survival/total_storage_governance.hpp"

using namespace isha;

int main() {
    std::cout << "Starting total_storage_governance_benchmark..." << std::endl;

    FootprintMetrics metrics;
    metrics.apk_size_mb = 120.0;
    metrics.models_size_mb = 373.0;
    metrics.mmap_snapshots_size_mb = 10.0;
    metrics.caches_size_mb = 5.0;
    metrics.telemetry_size_mb = 2.0;
    metrics.temp_extraction_size_mb = 0.0;
    metrics.checkpoints_size_mb = 1.0;
    metrics.session_history_size_mb = 1.0;

    auto response = TotalStorageGovernance::evaluateTotalFootprint(metrics);
    assert(!response.limit_approached);

    // Total footprint past 900MB triggers storage limit response
    metrics.temp_extraction_size_mb = 450.0;
    response = TotalStorageGovernance::evaluateTotalFootprint(metrics);
    assert(response.limit_approached);
    assert(response.purge_stale_telemetry);
    assert(response.purge_stale_temp);
    assert(response.cleanup_obsolete_models);

    std::cout << "total_storage_governance_benchmark PASSED!" << std::endl;
    return 0;
}
