#ifndef ISHA_AI_RECOVERY_UX_MANAGER_HPP
#define ISHA_AI_RECOVERY_UX_MANAGER_HPP

#include <string>

namespace isha {

enum class UserNotificationEvent {
    CORRUPTED_MODEL_RECOVERED,
    SAFE_MODE_ACTIVATED,
    LOW_STORAGE_RECLAIMED,
    DEGRADED_PERFORMANCE_ACTIVE,
    THERMAL_PROTECTION_ENGAGED,
    UPDATE_ROLLED_BACK,
    ACCELERATION_DISABLED
};

class RecoveryUXManager {
public:
    // Formulates clear, reassuring, user-friendly copy explaining recovery events
    static std::string getDisplayHeading(UserNotificationEvent event);
    static std::string getDisplayDescription(UserNotificationEvent event);
};

} // namespace isha

#endif // ISHA_AI_RECOVERY_UX_MANAGER_HPP
