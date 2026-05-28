#include <iostream>
#include <cassert>
#include "survival/memory_drift_trend_guard.hpp"

using namespace isha;

int main() {
    std::cout << "Starting memory_drift_trend_guard_benchmark..." << std::endl;

    MemoryTrend trend;
    trend.rss_history_mb = {120.0, 125.0, 122.0, 126.0}; // Minor fluctuations
    trend.fragmentation_history = {0.1, 0.1, 0.1, 0.1};
    trend.current_mmap_pressure_mb = 100.0;
    trend.accumulated_cache_mb = 15.0;
    trend.accumulated_checkpoints_mb = 2.0;
    trend.scheduler_latency_drift_ms = 0.0;

    auto action = MemoryDriftTrendGuard::evaluateTrend(trend);
    assert(!action.drift_detected);

    // Monotonic significant memory drift triggers action
    trend.rss_history_mb = {120.0, 140.0, 160.0, 180.0};
    action = MemoryDriftTrendGuard::evaluateTrend(trend);
    assert(action.drift_detected);
    assert(action.trigger_cache_cleanup);
    assert(action.escalate_degradation);

    std::cout << "memory_drift_trend_guard_benchmark PASSED!" << std::endl;
    return 0;
}
