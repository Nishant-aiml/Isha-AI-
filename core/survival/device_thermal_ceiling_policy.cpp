#include "device_thermal_ceiling_policy.hpp"

namespace isha {

std::string DeviceThermalCeilingPolicy::classToString(ThermalCeilingClass ceiling_class) {
    switch (ceiling_class) {
        case ThermalCeilingClass::CONSERVATIVE: return "CONSERVATIVE";
        case ThermalCeilingClass::MODERATE: return "MODERATE";
        case ThermalCeilingClass::RELAXED: return "RELAXED";
        default: return "UNKNOWN";
    }
}

ThermalCeilingClass DeviceThermalCeilingPolicy::determineCeilingClass(unsigned int ram_gb) {
    if (ram_gb <= 4) {
        return ThermalCeilingClass::CONSERVATIVE; // Strict budget eMMC/4GB limits
    } else if (ram_gb <= 6) {
        return ThermalCeilingClass::MODERATE;     // Standard midrange 6GB constraints
    } else {
        return ThermalCeilingClass::RELAXED;      // Flagship 8GB+ thermal dissipation is higher
    }
}

ThermalCeilingPolicy DeviceThermalCeilingPolicy::getPolicy(ThermalCeilingClass ceiling_class) {
    ThermalCeilingPolicy policy;
    policy.ceiling_class = ceiling_class;

    switch (ceiling_class) {
        case ThermalCeilingClass::CONSERVATIVE:
            policy.max_allowed_threads = 2;       // Cap thread heat generation on budget cores
            policy.max_allowed_batch_size = 4;
            policy.enable_acceleration = false;   // CPU only fallback to minimize battery heating
            policy.scheduler_pacing_ms = 500;     // Insert gaps between runs
            policy.decode_pacing_ms = 20;         // Cap generation rate to cool CPU
            break;
            
        case ThermalCeilingClass::MODERATE:
            policy.max_allowed_threads = 4;
            policy.max_allowed_batch_size = 8;
            policy.enable_acceleration = true;
            policy.scheduler_pacing_ms = 100;
            policy.decode_pacing_ms = 5;
            break;
            
        case ThermalCeilingClass::RELAXED:
            policy.max_allowed_threads = 8;
            policy.max_allowed_batch_size = 32;
            policy.enable_acceleration = true;
            policy.scheduler_pacing_ms = 0;
            policy.decode_pacing_ms = 0;
            break;
    }

    return policy;
}

} // namespace isha
