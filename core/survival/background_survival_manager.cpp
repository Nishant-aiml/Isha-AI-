#include "background_survival_manager.hpp"

namespace isha {

BackgroundSurvivalManager::SurvivalParameters BackgroundSurvivalManager::onEnterBackground() {
    SurvivalParameters params;
    params.acceleration_unloaded = true; // Always unload heavy GPU acceleration when backgrounded
    params.context_size_cells = 512;     // Shrink context memory space to absolute minimum
    params.telemetry_frequency_ms = 60000; // Drop telemetry polling rate to 1 minute
    params.scheduler_active = false;      // Pause scheduler background executions
    params.stale_buffers_cleaned = true;  // Clean up cache arenas
    params.polling_frequency_ms = 10000;  // Slow background tick to 10 seconds
    return params;
}

BackgroundSurvivalManager::SurvivalParameters BackgroundSurvivalManager::onEnterForeground() {
    SurvivalParameters params;
    params.acceleration_unloaded = false; // Restore acceleration context
    params.context_size_cells = 2048;    // Scale back up to standard 2048 context size
    params.telemetry_frequency_ms = 5000; // Restore telemetry to 5 seconds
    params.scheduler_active = true;
    params.stale_buffers_cleaned = false;
    params.polling_frequency_ms = 100;    // High-responsiveness tick
    return params;
}

} // namespace isha
