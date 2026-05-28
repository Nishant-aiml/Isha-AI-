#include "recovery_ux_manager.hpp"

namespace isha {

std::string RecoveryUXManager::getDisplayHeading(UserNotificationEvent event) {
    switch (event) {
        case UserNotificationEvent::CORRUPTED_MODEL_RECOVERED:
            return "Model Restored Successfully";
        case UserNotificationEvent::SAFE_MODE_ACTIVATED:
            return "Safe Mode Enabled";
        case UserNotificationEvent::LOW_STORAGE_RECLAIMED:
            return "Storage Cleaned Up";
        case UserNotificationEvent::DEGRADED_PERFORMANCE_ACTIVE:
            return "Optimized Performance Active";
        case UserNotificationEvent::THERMAL_PROTECTION_ENGAGED:
            return "Device Cooling Active";
        case UserNotificationEvent::UPDATE_ROLLED_BACK:
            return "Update Safeguard Active";
        case UserNotificationEvent::ACCELERATION_DISABLED:
            return "Battery Preservation Active";
        default:
            return "Notification Alert";
    }
}

std::string RecoveryUXManager::getDisplayDescription(UserNotificationEvent event) {
    switch (event) {
        case UserNotificationEvent::CORRUPTED_MODEL_RECOVERED:
            return "We detected an issue with the AI helper files and successfully recovered them. The app is fully ready for use.";
        case UserNotificationEvent::SAFE_MODE_ACTIVATED:
            return "The application is running in Safe Mode to guarantee stability. Your chat logs are fully secure.";
        case UserNotificationEvent::LOW_STORAGE_RECLAIMED:
            return "We cleaned up unused temporary cache files to prevent your phone's storage from filling up.";
        case UserNotificationEvent::DEGRADED_PERFORMANCE_ACTIVE:
            return "The app is running in low-resource mode to ensure it remains stable and responsive on your phone.";
        case UserNotificationEvent::THERMAL_PROTECTION_ENGAGED:
            return "Your phone is running warm. We are temporarily pacing the AI to prevent battery drain and let it cool down.";
        case UserNotificationEvent::UPDATE_ROLLED_BACK:
            return "A recent download was interrupted. We safely rolled back to your previous working version.";
        case UserNotificationEvent::ACCELERATION_DISABLED:
            return "Your battery is low. We have disabled advanced graphics processing to help extend your battery life.";
        default:
            return "An internal status adjustment has occurred to keep the app working cleanly.";
    }
}

} // namespace isha
