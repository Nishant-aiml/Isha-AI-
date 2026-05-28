#ifndef ISHA_AI_RECOVERY_NOTIFICATION_POLICY_HPP
#define ISHA_AI_RECOVERY_NOTIFICATION_POLICY_HPP

#include <string>
#include <mutex>
#include <map>
#include <chrono>

namespace isha {

enum class NotificationType {
    THERMAL_WARNING,
    SAFE_MODE_NOTICE,
    LOW_STORAGE_ALERT,
    ACCELERATION_DISABLED,
    RECOVERY_TRIGGERED
};

struct NotificationAction {
    bool should_display;
    bool display_silent;
    bool display_grouped;
    std::string custom_display_copy;
};

class RecoveryNotificationPolicy {
public:
    static RecoveryNotificationPolicy& getInstance();

    // Clears notification history logs
    void resetNotificationHistory();

    // Determines if notification should display based on duplicate checks and cooldown parameters
    NotificationAction evaluateNotification(NotificationType type, const std::string& fallback_desc);

private:
    RecoveryNotificationPolicy() = default;
    ~RecoveryNotificationPolicy() = default;

    std::map<NotificationType, std::chrono::steady_clock::time_point> last_notification_timestamps_;
    std::map<NotificationType, unsigned int> notification_counts_;
    std::mutex notification_mutex_;
};

} // namespace isha

#endif // ISHA_AI_RECOVERY_NOTIFICATION_POLICY_HPP
