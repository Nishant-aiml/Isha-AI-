#ifndef ISHA_AI_BATTERY_FLOOR_POLICY_HPP
#define ISHA_AI_BATTERY_FLOOR_POLICY_HPP

#include <string>

namespace isha {

enum class BatteryState {
    NORMAL,
    LIGHT_THROTTLE,
    SURVIVAL_MODE,
    SAFE_MODE_RECOMMENDED
};

struct BatteryPolicy {
    BatteryState state;
    unsigned int optimal_thread_count;
    unsigned int telemetry_frequency_ms;
    unsigned int batch_size;
    unsigned int max_context_cells;
    bool allow_acceleration;
    unsigned int watchdog_polling_ms;
    bool enable_background_prefetch;
};

class BatteryFloorPolicy {
public:
    static std::string stateToString(BatteryState state);

    // Determines battery state based on current level percentage
    static BatteryState determineState(double battery_level_percentage);

    // Configures adaptive parameters according to the active battery state
    static BatteryPolicy getPolicy(BatteryState state);
};

} // namespace isha

#endif // ISHA_AI_BATTERY_FLOOR_POLICY_HPP
