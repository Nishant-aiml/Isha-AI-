#ifndef ISHA_AI_BATTERY_LIFETIME_OBSERVER_HPP
#define ISHA_AI_BATTERY_LIFETIME_OBSERVER_HPP

namespace isha {

struct BatteryObservation {
    double idle_drain_24h_percent;
    double overnight_standby_drain_percent;
    double screen_off_drain_per_hour;
    double charging_thermal_rise_c;
    int background_wakes_per_hour;
    bool battery_saver_active;
};

struct BatteryMitigation {
    bool reduce_background_work;
    bool reduce_telemetry;
    bool reduce_checkpoint_frequency;
    bool reduce_wake_frequency;
    bool throttle_acceleration;
};

class BatteryLifetimeObserver {
public:
    static BatteryMitigation evaluateBatteryImpact(const BatteryObservation& obs);
};

} // namespace isha

#endif // ISHA_AI_BATTERY_LIFETIME_OBSERVER_HPP
