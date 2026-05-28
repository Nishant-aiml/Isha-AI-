#include "first_token_consistency_policy.hpp"

namespace isha {

ConsistencyAdjustments FirstTokenConsistencyPolicy::evaluateConsistency(const StartupWakeMetrics& metrics) {
    ConsistencyAdjustments adjustments;
    
    // Evaluate if first-decode variance or cold-boot time is excessive (>500ms variance or >2.5s cold-boot)
    bool high_variance = (metrics.first_decode_variance_ms > 500.0) || 
                          (metrics.cold_start_latency_ms > 2500.0) || 
                          (metrics.tokenizer_wake_ms > 400.0);

    if (high_variance) {
        adjustments.enable_aggressive_preload = true;  // Prefetch model segments early to bypass page-fault blocks
        adjustments.enable_mmap_warmup = true;         // Touch mapped memory addresses to trigger cache warmups
        adjustments.cache_tokenizer_symbols = true;    // Persist active vocab nodes in fast access cache
        adjustments.initial_batch_size = 1;            // Limit initial prefill to 1 element to ensure fast first-token responsiveness
        adjustments.force_sequential_startup = true;   // Synchronize startup blocks sequentially to avoid thread competition
    } else {
        adjustments.enable_aggressive_preload = false;
        adjustments.enable_mmap_warmup = false;
        adjustments.cache_tokenizer_symbols = false;
        adjustments.initial_batch_size = 16;
        adjustments.force_sequential_startup = false;
    }

    return adjustments;
}

} // namespace isha
