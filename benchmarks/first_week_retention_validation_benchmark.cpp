#include "survival/first_week_retention_validation.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Starting first_week_retention_validation_benchmark..." << std::endl;

    // Normal clean state
    isha::RetentionRiskMetrics normal = { 0, 0, 0, 0, 0, 0 };
    double normal_risk = isha::FirstWeekRetentionValidation::calculateUninstallProbability(normal);
    assert(normal_risk == 0.0);
    auto normal_level = isha::FirstWeekRetentionValidation::getRiskLevel(normal_risk);
    assert(normal_level == isha::RiskLevel::LOW);

    // High risk state
    isha::RetentionRiskMetrics high = { 2, 1, 3, 1, 0, 5 };
    double high_risk = isha::FirstWeekRetentionValidation::calculateUninstallProbability(high);
    assert(high_risk > 0.5);
    auto high_level = isha::FirstWeekRetentionValidation::getRiskLevel(high_risk);
    assert(high_level == isha::RiskLevel::HIGH || high_level == isha::RiskLevel::CRITICAL);

    auto mit = isha::FirstWeekRetentionValidation::evaluateMitigations(high_level);
    assert(mit.reduce_batching == true);
    assert(mit.reduce_acceleration == true);

    std::cout << "first_week_retention_validation_benchmark PASSED!" << std::endl;
    return 0;
}

