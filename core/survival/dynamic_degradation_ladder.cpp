#include "dynamic_degradation_ladder.hpp"

namespace isha {

std::string DynamicDegradationLadder::stepToString(DegradationStep step) {
    switch (step) {
        case DegradationStep::NORMAL: return "NORMAL";
        case DegradationStep::LIGHT_THROTTLE: return "LIGHT_THROTTLE";
        case DegradationStep::SURVIVAL_MODE: return "SURVIVAL_MODE";
        case DegradationStep::SAFE_MODE: return "SAFE_MODE";
        default: return "UNKNOWN";
    }
}

DegradationStep DynamicDegradationLadder::determineDegradationStep(const EnvironmentalStress& stress) {
    // 1. Extreme triggers => SAFE_MODE
    if (stress.temp_c > 45.0 || stress.battery_percentage < 10.0 || 
        stress.free_storage_gb < 0.3 || stress.anr_risk_threat) {
        return DegradationStep::SAFE_MODE;
    }

    // 2. High triggers => SURVIVAL_MODE
    if (stress.temp_c > 41.0 || stress.battery_percentage < 20.0 || 
        stress.free_storage_gb < 0.8 || stress.watchdog_escalation || stress.page_fault_storm) {
        return DegradationStep::SURVIVAL_MODE;
    }

    // 3. Moderate triggers => LIGHT_THROTTLE
    if (stress.temp_c > 38.0 || stress.battery_percentage < 40.0 || 
        stress.free_storage_gb < 1.5 || stress.fragmentation_ratio > 0.4) {
        return DegradationStep::LIGHT_THROTTLE;
    }

    // Healthy parameters => NORMAL
    return DegradationStep::NORMAL;
}

ExecutionLimits DynamicDegradationLadder::getLimits(DegradationStep step) {
    ExecutionLimits limits;
    limits.step = step;

    switch (step) {
        case DegradationStep::NORMAL:
            limits.batch_size = 16;
            limits.max_context_cells = 2048;
            limits.use_acceleration = true;
            limits.telemetry_frequency_ms = 5000;
            limits.polling_frequency_ms = 100;
            limits.scheduler_aggressive = true;
            break;
            
        case DegradationStep::LIGHT_THROTTLE:
            limits.batch_size = 8;
            limits.max_context_cells = 1024;
            limits.use_acceleration = true;
            limits.telemetry_frequency_ms = 15000;
            limits.polling_frequency_ms = 250;
            limits.scheduler_aggressive = true;
            break;
            
        case DegradationStep::SURVIVAL_MODE:
            limits.batch_size = 4;
            limits.max_context_cells = 512;
            limits.use_acceleration = false; // Disable power-hungry accelerator units
            limits.telemetry_frequency_ms = 60000;
            limits.polling_frequency_ms = 1000;
            limits.scheduler_aggressive = false;
            break;
            
        case DegradationStep::SAFE_MODE:
            limits.batch_size = 1;
            limits.max_context_cells = 256;
            limits.use_acceleration = false;
            limits.telemetry_frequency_ms = 120000;
            limits.polling_frequency_ms = 5000;
            limits.scheduler_aggressive = false;
            break;
    }

    return limits;
}

} // namespace isha
