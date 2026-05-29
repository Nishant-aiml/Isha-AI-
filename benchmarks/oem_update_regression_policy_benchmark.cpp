#include "survival/oem_update_regression_policy.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Starting oem_update_regression_policy_benchmark..." << std::endl;

    isha::PlatformState baseline = { "android_12", false, 40.0, 8 };
    
    // No change in fingerprint
    isha::PlatformState current_same = { "android_12", false, 40.0, 8 };
    bool reg_same = isha::OemUpdateRegressionPolicy::detectRegression(baseline, current_same);
    assert(!reg_same);

    // Fingerprint changed but no restriction
    isha::PlatformState current_ok = { "android_13", false, 40.0, 8 };
    bool reg_ok = isha::OemUpdateRegressionPolicy::detectRegression(baseline, current_ok);
    assert(!reg_ok);

    // Fingerprint changed and regressed constraints
    isha::PlatformState current_bad = { "android_13", true, 35.0, 4 };
    bool reg_bad = isha::OemUpdateRegressionPolicy::detectRegression(baseline, current_bad);
    assert(reg_bad);

    auto action = isha::OemUpdateRegressionPolicy::handleRegression(reg_bad);
    assert(action.trigger_reprobe == true);
    assert(action.lower_batching == true);
    assert(action.survival_first_mode == true);

    std::cout << "oem_update_regression_policy_benchmark PASSED!" << std::endl;
    return 0;
}

