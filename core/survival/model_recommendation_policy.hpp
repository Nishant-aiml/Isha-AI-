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
    std::string recommended_model_dimension;
    unsigned int recommended_context_limit;
    unsigned int optimal_batch_size;
    bool enable_acceleration;
    double thermal_threshold_c;
    unsigned int scheduler_pacing_ms;
};

class ModelRecommendationPolicy {
public:
    static std::string ramClassToString(DeviceRamClass ram_class);

    // Determines RAM category of active hardware
    static DeviceRamClass determineRamClass(unsigned int ram_gb);

    // Formulates absolute safe recommendations and operational parameters
    static RecommendationDetails getRecommendation(DeviceRamClass ram_class);
};

} // namespace isha

#endif // ISHA_AI_MODEL_RECOMMENDATION_POLICY_HPP
