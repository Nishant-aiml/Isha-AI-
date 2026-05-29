#include "model_recommendation_policy.hpp"
#include "../config/resource_limits.hpp"

namespace isha {

std::string ModelRecommendationPolicy::ramClassToString(DeviceRamClass ram_class) {
    switch (ram_class) {
        case DeviceRamClass::BUDGET_4GB:        return "BUDGET_4GB";
        case DeviceRamClass::MIDRANGE_6GB:      return "MIDRANGE_6GB";
        case DeviceRamClass::FLAGSHIP_8GB_PLUS: return "FLAGSHIP_8GB_PLUS";
        default: return "UNKNOWN";
    }
}

DeviceRamClass ModelRecommendationPolicy::determineRamClass(unsigned int ram_gb) {
    if (ram_gb <= 4)  return DeviceRamClass::BUDGET_4GB;
    if (ram_gb <= 6)  return DeviceRamClass::MIDRANGE_6GB;
    return DeviceRamClass::FLAGSHIP_8GB_PLUS;
}

// ============================================================
// RUNTIME LOAD GATE CHECKS
// These are the authoritative go/no-go decisions at load time.
// Called by the orchestration layer before attempting any load.
// ============================================================

bool ModelRecommendationPolicy::canLoadT1(
    unsigned int free_ram_mb,
    double thermal_c,
    unsigned int battery_pct,
    bool system_memory_pressure)
{
    bool ok = ResourceLimits::canLoadT1(free_ram_mb, thermal_c, battery_pct, system_memory_pressure);
    if (!ok) {
        // Fallback: T0 + retrieval-only mode with honest user message
        // "Your device is currently under memory or thermal pressure.
        //  Running lightweight offline mode."
    }
    return ok;
}

bool ModelRecommendationPolicy::canLoadT2(
    unsigned int physical_ram_gb,
    unsigned int free_ram_mb,
    double thermal_c,
    unsigned int battery_pct)
{
    // T2 does NOT target 4GB devices.
    // Minimum 6GB physical RAM. Recommended 8GB.
    return ResourceLimits::canLoadT2(physical_ram_gb, free_ram_mb, thermal_c, battery_pct);
}

bool ModelRecommendationPolicy::canLoadT3(
    unsigned int physical_ram_gb,
    unsigned int free_ram_mb,
    double thermal_c,
    unsigned int battery_pct,
    bool charging)
{
    // T3 NEVER auto-loads. Caller must verify explicit user request
    // (Deep Reasoning / Maximum Quality / Ultra Mode) before calling this.
    // 8GB devices: experimental — requires charging.
    // 12GB+ devices: certified — charging preferred but not required.
    return ResourceLimits::canLoadT3(physical_ram_gb, free_ram_mb, thermal_c, battery_pct, charging);
}

// ============================================================
// RECOMMENDATION — Returns model dimension + config per device class.
// ============================================================
RecommendationDetails ModelRecommendationPolicy::getRecommendation(DeviceRamClass ram_class) {
    RecommendationDetails details;

    // Strict Role Boundaries:
    // T0 (0.5B, CLASS_A): Router & Classifier ONLY. Always resident. Never converses.
    // T1 (1.5B, CLASS_B): Conversational workhorse. Warm hibernated. Conditional load.
    // T2 (2B Gemma-3, CLASS_D): Complex reasoning. 6GB minimum. 42°C gate.
    // T3 (7B Mistral, CLASS_D): Flagship tasks. 8GB minimum (12GB recommended). Explicit-only.

    switch (ram_class) {

        case DeviceRamClass::BUDGET_4GB:
            // T1 is the conversation model, BUT conditionally supported on 4GB.
            // If canLoadT1() fails → fallback to T0 + retrieval-only.
            // User message: "Your device is under memory/thermal pressure. Running lightweight mode."
            details.recommended_model_dimension = "1.5B_MIDRANGE"; // conditional on T1 gate
            details.recommended_context_limit   = 512;   // safe for 4GB devices
            details.optimal_batch_size          = 4;
            details.enable_acceleration         = false; // CPU-only for thermal safety
            details.thermal_threshold_c         = ThermalPolicy::ZONE3_T1_ONLY; // 44°C gate
            details.scheduler_pacing_ms         = 500;
            details.max_output_tokens           = TokenBudgetPolicy::T1_MAX_OUTPUT; // 384
            break;

        case DeviceRamClass::MIDRANGE_6GB:
            // T1 default. T2 available if canLoadT2() passes.
            details.recommended_model_dimension = "1.5B_MIDRANGE";
            details.recommended_context_limit   = 1024;
            details.optimal_batch_size          = 8;
            details.enable_acceleration         = true;
            details.thermal_threshold_c         = ThermalPolicy::ZONE3_T1_ONLY; // 44°C T1 gate
            details.scheduler_pacing_ms         = 100;
            details.max_output_tokens           = TokenBudgetPolicy::T1_MAX_OUTPUT; // 384
            break;

        case DeviceRamClass::FLAGSHIP_8GB_PLUS:
            // T2 default. T3 only on explicit user request + canLoadT3().
            // 8GB: T3 experimental (charging required).
            // 12GB+: T3 certified.
            details.recommended_model_dimension = "ADVANCED_GATED"; // T2 default; T3 explicit
            details.recommended_context_limit   = 2048;
            details.optimal_batch_size          = 16;
            details.enable_acceleration         = true;
            details.thermal_threshold_c         = ThermalPolicy::ZONE2_T2_DISABLE; // 42°C T2 gate
            details.scheduler_pacing_ms         = 0;
            details.max_output_tokens           = TokenBudgetPolicy::T2_MAX_OUTPUT; // 768 for T2 default
            break;
    }

    return details;
}

} // namespace isha
