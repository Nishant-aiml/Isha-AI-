#include "recovery_notification_policy.hpp"

namespace isha {

RecoveryNotificationPolicy& RecoveryNotificationPolicy::getInstance() {
    static RecoveryNotificationPolicy instance;
    return instance;
}

void RecoveryNotificationPolicy::resetNotificationHistory() {
    std::lock_guard<std::mutex> lock(notification_mutex_);
    last_notification_timestamps_.clear();
    notification_counts_.clear();
}

NotificationAction RecoveryNotificationPolicy::evaluateNotification(NotificationType type, const std::string& fallback_desc) {
    std::lock_guard<std::mutex> lock(notification_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    NotificationAction action;
    action.should_display = true;
    action.display_silent = false;
    action.display_grouped = false;
    action.custom_display_copy = fallback_desc;

    // Check last display timestamp to prevent spam loops (cooldown boundary 10 seconds)
    auto it_time = last_notification_timestamps_.find(type);
    if (it_time != last_notification_timestamps_.end()) {
        double duration = std::chrono::duration<double>(now - it_time->second).count();
        if (duration < 10.0) {
            action.should_display = false; // Block repeated warning displays within cooldown limits
            return action;
        }
    }

    // Increment count
    unsigned int count = ++notification_counts_[type];
    last_notification_timestamps_[type] = now;

    // 1. Grouped notification responses on repeated occurrences
    if (count >= 5) {
        action.display_grouped = true;
        action.custom_display_copy = "Notice: Several system adjustments have occurred to keep your app running smoothly.";
    } else if (count >= 2) {
        action.display_silent = true; // Make subsequent notifications non-intrusive / silent
    }

    return action;
}

} // namespace isha
