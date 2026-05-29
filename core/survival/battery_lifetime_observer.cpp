#include "battery_lifetime_observer.hpp"

namespace isha {

BatteryMitigation BatteryLifetimeObserver::evaluateBatteryImpact(const BatteryObservation& obs) {
    BatteryMitigation mit;
    mit.reduce_background_work = false;
    mit.reduce_telemetry = false;
    mit.reduce_checkpoint_frequency = false;
    mit.reduce_wake_frequency = false;
    mit.throttle_acceleration = false;

    // Strict limits to maintain positive battery-life perceptions
    if (obs.battery_saver_active) {
        mit.reduce_background_work = true;
        mit.reduce_telemetry = true;
        mit.reduce_checkpoint_frequency = true;
        mit.reduce_wake_frequency = true;
        mit.throttle_acceleration = true;
        return mit;
    }

    if (obs.overnight_standby_drain_percent > 3.0 || obs.screen_off_drain_per_hour > 1.5) {
        mit.reduce_background_work = true;
        mit.reduce_wake_frequency = true;
    }

    if (obs.charging_thermal_rise_c > 8.0) {
        mit.throttle_acceleration = true;
    }

    if (obs.background_wakes_per_hour > 10) {
        mit.reduce_wake_frequency = true;
        mit.reduce_telemetry = true;
    }

    return mit;
}

} // namespace isha
