#ifndef ISHA_AI_BACKGROUND_SURVIVAL_MANAGER_HPP
#define ISHA_AI_BACKGROUND_SURVIVAL_MANAGER_HPP

#include <string>

namespace isha {

class BackgroundSurvivalManager {
public:
    struct SurvivalParameters {
        bool acceleration_unloaded;
        unsigned int context_size_cells;
        unsigned int telemetry_frequency_ms;
        bool scheduler_active;
        bool stale_buffers_cleaned;
        unsigned int polling_frequency_ms;
    };

    static SurvivalParameters onEnterBackground();
    static SurvivalParameters onEnterForeground();
};

} // namespace isha

#endif // ISHA_AI_BACKGROUND_SURVIVAL_MANAGER_HPP
