#ifndef ISHA_AI_UPDATE_SAFETY_WINDOW_HPP
#define ISHA_AI_UPDATE_SAFETY_WINDOW_HPP

#include <string>

namespace isha {

struct UpdateSafetyFactors {
    double battery_level_percentage;
    double thermal_temp_c;
    double free_storage_gb;
    bool active_inference_running;
    bool watchdog_escalation_active;
    bool safe_mode_active;
    bool fragmentation_critical;
};

class UpdateSafetyWindow {
public:
    // Checks if the active environment complies with all critical safety bounds
    static bool isSafetyWindowOpen(const UpdateSafetyFactors& factors);
};

} // namespace isha

#endif // ISHA_AI_UPDATE_SAFETY_WINDOW_HPP
