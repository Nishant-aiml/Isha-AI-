#ifndef ISHA_AI_OEM_UPDATE_REGRESSION_POLICY_HPP
#define ISHA_AI_OEM_UPDATE_REGRESSION_POLICY_HPP

#include <string>

namespace isha {

struct PlatformState {
    std::string build_fingerprint;
    bool background_restricted;
    double thermal_limit_c;
    int max_available_cores;
};

struct RegressionResponse {
    bool trigger_reprobe;
    bool reset_trust_score;
    bool lower_batching;
    bool survival_first_mode;
};

class OemUpdateRegressionPolicy {
public:
    // Returns true if a regression has been detected relative to the baseline state
    static bool detectRegression(const PlatformState& baseline, const PlatformState& current);

    // Determines action responses on detection
    static RegressionResponse handleRegression(bool is_regressed);
};


} // namespace isha

#endif // ISHA_AI_OEM_UPDATE_REGRESSION_POLICY_HPP
