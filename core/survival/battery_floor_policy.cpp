#include "battery_floor_policy.hpp"

namespace isha {

std::string BatteryFloorPolicy::stateToString(BatteryState state) {
    switch (state) {
        case BatteryState::NORMAL: return "NORMAL";
        case BatteryState::LIGHT_THROTTLE: return "LIGHT_THROTTLE";
        case BatteryState::SURVIVAL_MODE: return "SURVIVAL_MODE";
        case BatteryState::SAFE_MODE_RECOMMENDED: return "SAFE_MODE_RECOMMENDED";
        default: return "UNKNOWN";
    }
}

BatteryState BatteryFloorPolicy::determineState(double battery_level_percentage) {
    if (battery_level_percentage > 40.0) {
        return BatteryState::NORMAL;
    } else if (battery_level_percentage > 20.0) {
        return BatteryState::LIGHT_THROTTLE;
    } else if (battery_level_percentage > 10.0) {
        return BatteryState::SURVIVAL_MODE;
    } else {
        return BatteryState::SAFE_MODE_RECOMMENDED;
    }
}

BatteryPolicy BatteryFloorPolicy::getPolicy(BatteryState state) {
    BatteryPolicy policy;
    policy.state = state;

    switch (state) {
        case BatteryState::NORMAL:
            policy.optimal_thread_count = 4;
            policy.telemetry_frequency_ms = 5000;
            policy.batch_size = 16;
            policy.max_context_cells = 2048;
            policy.allow_acceleration = true;
            policy.watchdog_polling_ms = 100;
            policy.enable_background_prefetch = true;
            break;
            
        case BatteryState::LIGHT_THROTTLE:
            policy.optimal_thread_count = 2; // Throttle threads
            policy.telemetry_frequency_ms = 15000;
            policy.batch_size = 8;
            policy.max_context_cells = 1024;
            policy.allow_acceleration = true;
            policy.watchdog_polling_ms = 200;
            policy.enable_background_prefetch = true;
            break;
            
        case BatteryState::SURVIVAL_MODE:
            policy.optimal_thread_count = 1; // Drop threads to minimum to save current
            policy.telemetry_frequency_ms = 60000;
            policy.batch_size = 4;
            policy.max_context_cells = 512;
            policy.allow_acceleration = false; // Disable power-hungry GPU
            policy.watchdog_polling_ms = 1000;
            policy.enable_background_prefetch = false;
            break;
            
        case BatteryState::SAFE_MODE_RECOMMENDED:
            policy.optimal_thread_count = 1;
            policy.telemetry_frequency_ms = 120000;
            policy.batch_size = 1;
            policy.max_context_cells = 256;
            policy.allow_acceleration = false;
            policy.watchdog_polling_ms = 5000;
            policy.enable_background_prefetch = false;
            break;
    }
    
    return policy;
}

} // namespace isha
