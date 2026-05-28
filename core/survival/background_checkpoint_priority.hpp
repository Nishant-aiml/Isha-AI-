#ifndef ISHA_AI_BACKGROUND_CHECKPOINT_PRIORITY_HPP
#define ISHA_AI_BACKGROUND_CHECKPOINT_PRIORITY_HPP

#include <string>

namespace isha {

enum class PriorityTier {
    CRITICAL,
    HIGH,
    MEDIUM,
    LOW,
    DISCARDABLE
};

struct ExecutionContextPressure {
    bool low_battery;
    bool is_backgrounded;
    bool thermal_stress;
    bool high_oem_restrictions;
    bool lmk_pressure;
};

class BackgroundCheckpointPriority {
public:
    static std::string tierToString(PriorityTier tier);

    // Determines if a data tier should be prioritized and written based on contextual resource stress
    static bool shouldWriteData(PriorityTier tier, const ExecutionContextPressure& pressure);
};

} // namespace isha

#endif // ISHA_AI_BACKGROUND_CHECKPOINT_PRIORITY_HPP
