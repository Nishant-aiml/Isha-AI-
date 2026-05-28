#ifndef ISHA_AI_FIRST_TOKEN_CONSISTENCY_POLICY_HPP
#define ISHA_AI_FIRST_TOKEN_CONSISTENCY_POLICY_HPP

#include <string>

namespace isha {

struct StartupWakeMetrics {
    double cold_start_latency_ms;
    double warm_start_latency_ms;
    double degraded_latency_ms;
    double tokenizer_wake_ms;
    double mmap_wake_ms;
    double first_decode_variance_ms;
};

struct ConsistencyAdjustments {
    bool enable_aggressive_preload;
    bool enable_mmap_warmup;
    bool cache_tokenizer_symbols;
    unsigned int initial_batch_size;
    bool force_sequential_startup;
};

class FirstTokenConsistencyPolicy {
public:
    // Analyzes start latency profiles and determines adjustments to minimize response time variance
    static ConsistencyAdjustments evaluateConsistency(const StartupWakeMetrics& metrics);
};

} // namespace isha

#endif // ISHA_AI_FIRST_TOKEN_CONSISTENCY_POLICY_HPP
