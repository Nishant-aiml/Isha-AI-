#ifndef ISHA_AI_DYNAMIC_DEGRADATION_LADDER_HPP
#define ISHA_AI_DYNAMIC_DEGRADATION_LADDER_HPP

#include <string>

namespace isha {

enum class DegradationStep {
    NORMAL,
    LIGHT_THROTTLE,
    SURVIVAL_MODE,
    SAFE_MODE
};

struct EnvironmentalStress {
    double temp_c;
    double fragmentation_ratio;
    double battery_percentage;
    double free_storage_gb;
    bool anr_risk_threat;
    bool watchdog_escalation;
    bool page_fault_storm;
};

struct ExecutionLimits {
    DegradationStep step;
    unsigned int batch_size;
    unsigned int max_context_cells;
    bool use_acceleration;
    unsigned int telemetry_frequency_ms;
    unsigned int polling_frequency_ms;
    bool scheduler_aggressive;
};

class DynamicDegradationLadder {
public:
    static std::string stepToString(DegradationStep step);

    // Determines active step on the degradation ladder based on cumulative environmental pressure
    static DegradationStep determineDegradationStep(const EnvironmentalStress& stress);

    // Applies corresponding limits
    static ExecutionLimits getLimits(DegradationStep step);
};

} // namespace isha

#endif // ISHA_AI_DYNAMIC_DEGRADATION_LADDER_HPP
