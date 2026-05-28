#include <iostream>
#include <cassert>
#include "survival/device_thermal_ceiling_policy.hpp"

using namespace isha;

int main() {
    std::cout << "Starting device_thermal_ceiling_benchmark..." << std::endl;

    assert(DeviceThermalCeilingPolicy::determineCeilingClass(4) == ThermalCeilingClass::CONSERVATIVE);
    assert(DeviceThermalCeilingPolicy::determineCeilingClass(6) == ThermalCeilingClass::MODERATE);
    assert(DeviceThermalCeilingPolicy::determineCeilingClass(8) == ThermalCeilingClass::RELAXED);

    auto policy = DeviceThermalCeilingPolicy::getPolicy(ThermalCeilingClass::CONSERVATIVE);
    assert(policy.max_allowed_threads == 2);
    assert(!policy.enable_acceleration);
    assert(policy.scheduler_pacing_ms == 500);

    std::cout << "device_thermal_ceiling_benchmark PASSED!" << std::endl;
    return 0;
}
