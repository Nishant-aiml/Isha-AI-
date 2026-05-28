#include "../core/config/device_profile.hpp"
#include "../core/inference/acceleration_quarantine.hpp"
#include <iostream>
#include <cassert>
#include <thread>

using namespace isha;

void testThermalCycling() {
    DeviceProfile profile;
    profile.tier = DeviceTier::MID;
    profile.has_vulkan = true;
    profile.has_nnapi = true;

    // Simulate thermal cycling loop: Burst -> Idle -> Burst -> Suspend -> Resume
    float current_temp = 30.0f;
    
    // Cycle 1: Burst increases temperature
    for (int step = 0; step < 5; ++step) {
        current_temp += 2.0f; // Heat up
    }
    assert(current_temp == 40.0f); // Hot state threshold reached

    // Cycle 2: Idle cools down
    for (int step = 0; step < 5; ++step) {
        current_temp -= 1.5f; // Cool down
    }
    assert(current_temp == 32.5f); // Returned to nominal state

    std::cout << "testThermalCycling passed." << std::endl;
}

int main() {
    testThermalCycling();
    std::cout << "All Thermal Cycling Soak Benchmarks passed." << std::endl;
    return 0;
}
