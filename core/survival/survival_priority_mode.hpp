#ifndef ISHA_AI_SURVIVAL_PRIORITY_MODE_HPP
#define ISHA_AI_SURVIVAL_PRIORITY_MODE_HPP

#include <string>

namespace isha {

struct DeviceMetrics {
    double thermal_temp_c;
    double fragmentation_ratio;
    bool anr_risk_threat;
    bool scheduler_stalled;
    bool storage_stalled;
    double battery_drain_rate;
    bool oem_restrictions_tight;
};

struct ResourcePolicy {
    bool survival_mode_active;
    unsigned int optimal_batch_size;
    unsigned int max_context_cells;
    unsigned int telemetry_interval_ms;
    unsigned int scheduler_cool_down_ms;
    bool use_acceleration;
};

class SurvivalPriorityMode {
public:
    // Decides resource limits based on critical hardware indicators
    static ResourcePolicy evaluateSystemMetrics(const DeviceMetrics& metrics);
};

} // namespace isha

#endif // ISHA_AI_SURVIVAL_PRIORITY_MODE_HPP
