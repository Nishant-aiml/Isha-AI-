#include <iostream>
#include <cassert>
#include "survival/recovery_notification_policy.hpp"

using namespace isha;

int main() {
    std::cout << "Starting recovery_notification_policy_benchmark..." << std::endl;

    RecoveryNotificationPolicy& policy = RecoveryNotificationPolicy::getInstance();
    policy.resetNotificationHistory();

    // First display request
    auto action = policy.evaluateNotification(NotificationType::THERMAL_WARNING, "Device is running warm.");
    assert(action.should_display);
    assert(!action.display_silent);

    // Second display request (within 10s cooldown limit)
    action = policy.evaluateNotification(NotificationType::THERMAL_WARNING, "Device is running warm.");
    assert(!action.should_display); // Suppress repeat notification spam

    std::cout << "recovery_notification_policy_benchmark PASSED!" << std::endl;
    return 0;
}
