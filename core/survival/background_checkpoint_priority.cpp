#include "background_checkpoint_priority.hpp"

namespace isha {

std::string BackgroundCheckpointPriority::tierToString(PriorityTier tier) {
    switch (tier) {
        case PriorityTier::CRITICAL: return "CRITICAL";
        case PriorityTier::HIGH: return "HIGH";
        case PriorityTier::MEDIUM: return "MEDIUM";
        case PriorityTier::LOW: return "LOW";
        case PriorityTier::DISCARDABLE: return "DISCARDABLE";
        default: return "UNKNOWN";
    }
}

bool BackgroundCheckpointPriority::shouldWriteData(PriorityTier tier, const ExecutionContextPressure& pressure) {
    // 1. Extreme pressure conditions (background + LMK / OEM kill threat or extreme heat / battery drop)
    bool extreme_pressure = pressure.lmk_pressure || 
                            (pressure.is_backgrounded && pressure.high_oem_restrictions) ||
                            (pressure.thermal_stress && pressure.low_battery);

    if (extreme_pressure) {
        // Under extreme pressure, preserve ONLY CRITICAL state to avoid blocking thread queues and causing ANRs
        return tier == PriorityTier::CRITICAL;
    }

    // 2. Standard background or thermal throttling limits
    if (pressure.is_backgrounded || pressure.thermal_stress || pressure.low_battery) {
        // Discard low-priority telemetry and debug files to save energy and wear
        return tier == PriorityTier::CRITICAL || tier == PriorityTier::HIGH || tier == PriorityTier::MEDIUM;
    }

    // Default healthy operational conditions: write everything
    return true;
}

} // namespace isha
