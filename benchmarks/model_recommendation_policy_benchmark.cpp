#include <iostream>
#include <cassert>
#include "survival/model_recommendation_policy.hpp"

using namespace isha;

int main() {
    std::cout << "Starting model_recommendation_policy_benchmark..." << std::endl;

    assert(ModelRecommendationPolicy::determineRamClass(4) == DeviceRamClass::BUDGET_4GB);
    assert(ModelRecommendationPolicy::determineRamClass(6) == DeviceRamClass::MIDRANGE_6GB);
    assert(ModelRecommendationPolicy::determineRamClass(8) == DeviceRamClass::FLAGSHIP_8GB_PLUS);

    auto recommendation = ModelRecommendationPolicy::getRecommendation(DeviceRamClass::BUDGET_4GB);
    assert(recommendation.recommended_model_dimension == "1.5B_MIDRANGE");
    assert(recommendation.recommended_context_limit == 512);
    assert(!recommendation.enable_acceleration);

    std::cout << "model_recommendation_policy_benchmark PASSED!" << std::endl;
    return 0;
}
