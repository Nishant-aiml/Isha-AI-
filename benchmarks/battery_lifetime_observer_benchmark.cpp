#include "survival/battery_lifetime_observer.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Starting battery_lifetime_observer_benchmark..." << std::endl;

    // Normal state - no mitigation
    isha::BatteryObservation normal_obs = { 1.0, 1.2, 0.5, 2.0, 3, false };
    auto normal_mit = isha::BatteryLifetimeObserver::evaluateBatteryImpact(normal_obs);
    assert(!normal_mit.reduce_background_work);
    assert(!normal_mit.throttle_acceleration);

    // Battery saver active - full mitigation
    isha::BatteryObservation saver_obs = { 1.0, 1.2, 0.5, 2.0, 3, true };
    auto saver_mit = isha::BatteryLifetimeObserver::evaluateBatteryImpact(saver_obs);
    assert(saver_mit.reduce_background_work);
    assert(saver_mit.throttle_acceleration);
    assert(saver_mit.reduce_checkpoint_frequency);

    // Overnight drain excess
    isha::BatteryObservation drain_obs = { 5.0, 4.2, 0.5, 2.0, 3, false };
    auto drain_mit = isha::BatteryLifetimeObserver::evaluateBatteryImpact(drain_obs);
    assert(drain_mit.reduce_background_work);

    std::cout << "battery_lifetime_observer_benchmark PASSED!" << std::endl;
    return 0;
}
