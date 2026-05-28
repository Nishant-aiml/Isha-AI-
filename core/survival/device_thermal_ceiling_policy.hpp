#ifndef ISHA_AI_DEVICE_THERMAL_CEILING_POLICY_HPP
#define ISHA_AI_DEVICE_THERMAL_CEILING_POLICY_HPP

#include <string>

namespace isha {

enum class ThermalCeilingClass {
    CONSERVATIVE,
    MODERATE,
    RELAXED
};

struct ThermalCeilingPolicy {
    ThermalCeilingClass ceiling_class;
    unsigned int max_allowed_threads;
    unsigned int max_allowed_batch_size;
    bool enable_acceleration;
    unsigned int scheduler_pacing_ms;
    unsigned int decode_pacing_ms;
};

class DeviceThermalCeilingPolicy {
public:
    static std::string classToString(ThermalCeilingClass ceiling_class);

    // Determines class based on RAM size
    static ThermalCeilingClass determineCeilingClass(unsigned int ram_gb);

    // Drives pacing rules according to active thermal ceiling
    static ThermalCeilingPolicy getPolicy(ThermalCeilingClass ceiling_class);
};

} // namespace isha

#endif // ISHA_AI_DEVICE_THERMAL_CEILING_POLICY_HPP
