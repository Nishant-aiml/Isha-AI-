#ifndef ISHA_AI_MODEL_RECOMMENDATION_POLICY_HPP
#define ISHA_AI_MODEL_RECOMMENDATION_POLICY_HPP

#include <string>

namespace isha {

enum class DeviceRamClass {
    BUDGET_4GB,
    MIDRANGE_6GB,
    FLAGSHIP_8GB_PLUS
};

struct RecommendationDetails {
    std::string  recommended_model_dimension;
    unsigned int recommended_context_limit;
    unsigned int optimal_batch_size;
    bool         enable_acceleration;
    double       thermal_threshold_c;
    unsigned int scheduler_pacing_ms;
    unsigned int max_output_tokens = 384; // Hard output cap per tier
};

class ModelRecommendationPolicy {
public:
    static std::string ramClassToString(DeviceRamClass ram_class);

    // Determines RAM category of active hardware
    static DeviceRamClass determineRamClass(unsigned int ram_gb);

    // Formulates absolute safe recommendations and operational parameters
    static RecommendationDetails getRecommendation(DeviceRamClass ram_class);

    // -------------------------------------------------------
    // RUNTIME LOAD GATES — called before every model load attempt
    // All conditions must be true. Failure = graceful fallback.
    // -------------------------------------------------------

    // T1 gate: free_ram>=1200 && thermal<44 && battery>15 && !system_memory_pressure
    // Failure fallback: T0 + retrieval-only + honest user message
    static bool canLoadT1(unsigned int free_ram_mb, double thermal_c,
                          unsigned int battery_pct, bool system_memory_pressure);

    // T2 gate: physical_ram>=6GB && free_ram>=2200 && thermal<42 && battery>20
    // T2 does NOT support 4GB devices.
    static bool canLoadT2(unsigned int physical_ram_gb, unsigned int free_ram_mb,
                          double thermal_c, unsigned int battery_pct);

    // T3 gate: physical_ram>=8GB && free_ram>=6000 && thermal<35 && battery>40 && charging
    // T3 NEVER auto-loads. Caller must verify explicit user request first.
    // 8GB: experimental (charging required). 12GB+: certified.
    static bool canLoadT3(unsigned int physical_ram_gb, unsigned int free_ram_mb,
                          double thermal_c, unsigned int battery_pct, bool charging);
};

} // namespace isha

#endif // ISHA_AI_MODEL_RECOMMENDATION_POLICY_HPP
