#include <iostream>
#include <cassert>
#include "survival/first_token_consistency_policy.hpp"

using namespace isha;

int main() {
    std::cout << "Starting first_token_consistency_benchmark..." << std::endl;

    StartupWakeMetrics metrics;
    metrics.cold_start_latency_ms = 1200.0;
    metrics.warm_start_latency_ms = 300.0;
    metrics.degraded_latency_ms = 1800.0;
    metrics.tokenizer_wake_ms = 150.0;
    metrics.mmap_wake_ms = 200.0;
    metrics.first_decode_variance_ms = 100.0;

    auto adjustments = FirstTokenConsistencyPolicy::evaluateConsistency(metrics);
    assert(!adjustments.enable_aggressive_preload);
    assert(adjustments.initial_batch_size == 16);

    // Highly variable, slow startup triggers adjustments
    metrics.first_decode_variance_ms = 600.0;
    adjustments = FirstTokenConsistencyPolicy::evaluateConsistency(metrics);
    assert(adjustments.enable_aggressive_preload);
    assert(adjustments.enable_mmap_warmup);
    assert(adjustments.cache_tokenizer_symbols);
    assert(adjustments.initial_batch_size == 1);

    std::cout << "first_token_consistency_benchmark PASSED!" << std::endl;
    return 0;
}
