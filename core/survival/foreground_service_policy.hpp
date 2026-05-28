#ifndef ISHA_AI_FOREGROUND_SERVICE_POLICY_HPP
#define ISHA_AI_FOREGROUND_SERVICE_POLICY_HPP

#include <string>

namespace isha {

enum class AppStandbyBucket {
    ACTIVE,
    WORKING_SET,
    FREQUENT,
    RARE,
    RESTRICTED
};

struct BackgroundConstraints {
    bool is_doze_mode_active;
    bool is_oem_background_kill_aggressive;
    AppStandbyBucket standby_bucket;
};

struct ExecutionParameters {
    bool run_as_foreground_service;
    bool persistence_notification_active;
    unsigned int telemetry_frequency_ms;
    unsigned int context_size_cells;
    bool allow_heavy_inference;
};

class ForegroundServicePolicy {
public:
    static std::string bucketToString(AppStandbyBucket bucket);

    // Derives foreground service behaviors and execution limits based on Android OS context
    static ExecutionParameters evaluateConstraints(const BackgroundConstraints& constraints);
};

} // namespace isha

#endif // ISHA_AI_FOREGROUND_SERVICE_POLICY_HPP
