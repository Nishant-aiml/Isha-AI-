#include "install_time_device_profiler.hpp"

namespace isha {

SafeDefaults InstallTimeDeviceProfiler::profileDevice(const DeviceRawSpecs& specs) {
    SafeDefaults defaults;

    // 1. Check RAM constraints to map dimension classes
    if (specs.ram_gb <= 4) {
        defaults.recommended_model_dimension = "0.5B_BUDGET";
        defaults.max_batch_size = 4;
        defaults.max_context_cells = 256;
        defaults.telemetry_interval_ms = 60000; // Slow logging to preserve flash
        defaults.recommended_startup_strategy = "SAFE_MODE_STARTUP";
        defaults.thermal_threshold_ceiling_c = 38.0;
        defaults.allow_hardware_acceleration = false; // Never use GPU on budget RAM to prevent crashes
    } else if (specs.ram_gb <= 6) {
        defaults.recommended_model_dimension = "1.5B_MIDRANGE";
        defaults.max_batch_size = 8;
        defaults.max_context_cells = 1024;
        defaults.telemetry_interval_ms = 30000;
        defaults.recommended_startup_strategy = "WARM_STARTUP";
        defaults.thermal_threshold_ceiling_c = 41.0;
        defaults.allow_hardware_acceleration = !specs.is_emmc; // Only accelerate if disk is UFS
    } else {
        defaults.recommended_model_dimension = "ADVANCED_CONFIGS";
        defaults.max_batch_size = 16;
        defaults.max_context_cells = 2048;
        defaults.telemetry_interval_ms = 5000;
        defaults.recommended_startup_strategy = "COLD_STARTUP";
        defaults.thermal_threshold_ceiling_c = 45.0;
        defaults.allow_hardware_acceleration = true;
    }

    // 2. Tweak defaults based on restriction severity (e.g. background constraints)
    if (specs.has_oem_background_kill_app || specs.foreground_service_restricted) {
        defaults.max_context_cells = (defaults.max_context_cells > 512) ? 512 : defaults.max_context_cells;
        defaults.telemetry_interval_ms = 120000; // Drastically scale down logging to bypass memory hooks
    }

    return defaults;
}

} // namespace isha
