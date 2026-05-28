#include "model_recommendation_policy.hpp"

namespace isha {

std::string ModelRecommendationPolicy::ramClassToString(DeviceRamClass ram_class) {
    switch (ram_class) {
        case DeviceRamClass::BUDGET_4GB: return "BUDGET_4GB";
        case DeviceRamClass::MIDRANGE_6GB: return "MIDRANGE_6GB";
        case DeviceRamClass::FLAGSHIP_8GB_PLUS: return "FLAGSHIP_8GB_PLUS";
        default: return "UNKNOWN";
    }
}

DeviceRamClass ModelRecommendationPolicy::determineRamClass(unsigned int ram_gb) {
    if (ram_gb <= 4) {
        return DeviceRamClass::BUDGET_4GB;
    } else if (ram_gb <= 6) {
        return DeviceRamClass::MIDRANGE_6GB;
    } else {
        return DeviceRamClass::FLAGSHIP_8GB_PLUS;
    }
}

RecommendationDetails ModelRecommendationPolicy::getRecommendation(DeviceRamClass ram_class) {
    RecommendationDetails details;

    switch (ram_class) {
        case DeviceRamClass::BUDGET_4GB:
            details.recommended_model_dimension = "0.5B_LIGHTWEIGHT";
            details.recommended_context_limit = 256;         // Ultra-safe context to fit low RAM budgets
            details.optimal_batch_size = 4;
            details.enable_acceleration = false;             // Force CPU fallback for safety
            details.thermal_threshold_c = 38.0;              // Low thermal hysteresis floor
            details.scheduler_pacing_ms = 500;
            break;
            
        case DeviceRamClass::MIDRANGE_6GB:
            details.recommended_model_dimension = "1.5B_MIDRANGE";
            details.recommended_context_limit = 1024;
            details.optimal_batch_size = 8;
            details.enable_acceleration = true;
            details.thermal_threshold_c = 41.0;
            details.scheduler_pacing_ms = 100;
            break;
            
        case DeviceRamClass::FLAGSHIP_8GB_PLUS:
            details.recommended_model_dimension = "ADVANCED_CONFIGS";
            details.recommended_context_limit = 2048;
            details.optimal_batch_size = 16;
            details.enable_acceleration = true;
            details.thermal_threshold_c = 45.0;
            details.scheduler_pacing_ms = 0;
            break;
    }

    return details;
}

} // namespace isha
