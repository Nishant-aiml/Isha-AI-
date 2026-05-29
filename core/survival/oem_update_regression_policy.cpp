#include "oem_update_regression_policy.hpp"

namespace isha {

bool OemUpdateRegressionPolicy::detectRegression(const PlatformState& baseline, const PlatformState& current) {
    // Regression occurs if firmware fingerprint changes and constraints tighten
    if (baseline.build_fingerprint != current.build_fingerprint) {
        if (current.background_restricted && !baseline.background_restricted) {
            return true;
        }
        if (current.max_available_cores < baseline.max_available_cores) {
            return true;
        }
        if (current.thermal_limit_c < (baseline.thermal_limit_c - 2.0)) {
            return true;
        }
    }
    return false;
}

RegressionResponse OemUpdateRegressionPolicy::handleRegression(bool is_regressed) {
    RegressionResponse resp = { false, false, false, false };
    if (is_regressed) {
        resp.trigger_reprobe = true;
        resp.reset_trust_score = true;
        resp.lower_batching = true;
        resp.survival_first_mode = true;
    }
    return resp;
}


} // namespace isha
