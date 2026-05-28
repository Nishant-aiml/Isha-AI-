#ifndef ISHA_AI_DEPLOYMENT_HEALTH_SUMMARY_HPP
#define ISHA_AI_DEPLOYMENT_HEALTH_SUMMARY_HPP

#include <string>

namespace isha {

struct HealthStats {
    unsigned int crash_count;
    unsigned int safemode_count;
    unsigned int thermal_collapse_count;
    unsigned int startup_instability_count;
    double fragmentation_ratio;
    double battery_drain_rate;
    unsigned int storage_pressure_events;
    unsigned int watchdog_escalations;
    unsigned int recovery_attempts;
    unsigned int recovery_successes;
};

class DeploymentHealthSummary {
public:
    // Produces a human-readable health report for quick offline logging diagnostics
    static std::string generateHealthReport(const HealthStats& stats);
};

} // namespace isha

#endif // ISHA_AI_DEPLOYMENT_HEALTH_SUMMARY_HPP
