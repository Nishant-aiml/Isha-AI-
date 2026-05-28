#include <iostream>
#include <cassert>
#include <thread>
#include "survival/thermal_recovery_window.hpp"

using namespace isha;

int main() {
    std::cout << "Starting thermal_recovery_window_benchmark..." << std::endl;

    ThermalRecoveryWindow window;
    assert(window.getState() == ThermalRecoveryState::RECOVERED);
    assert(window.isAccelerationRestorationSafe());

    // Transition to HOT
    window.updateTemperature(43.0);
    assert(window.getState() == ThermalRecoveryState::HOT);
    assert(!window.isAccelerationRestorationSafe());
    assert(window.getPacingDelayMs() == 2000);

    // cooling down
    window.updateTemperature(39.0);
    assert(window.getState() == ThermalRecoveryState::COOLING);
    assert(window.getPacingDelayMs() == 1000);

    // stabilizing
    window.updateTemperature(37.0);
    assert(window.getState() == ThermalRecoveryState::STABILIZING);
    assert(window.getPacingDelayMs() == 200);

    // fully recovered (stabilization time check)
    std::this_thread::sleep_for(std::chrono::milliseconds(5100));
    window.updateTemperature(33.0);
    assert(window.getState() == ThermalRecoveryState::RECOVERED);
    assert(window.isAccelerationRestorationSafe());

    std::cout << "thermal_recovery_window_benchmark PASSED!" << std::endl;
    return 0;
}
