#include "update_safety_window.hpp"

namespace isha {

bool UpdateSafetyWindow::isSafetyWindowOpen(const UpdateSafetyFactors& factors) {
    if (factors.battery_level_percentage < 15.0) return false;      // Forbid update under 15% battery to prevent sudden shutdowns
    if (factors.thermal_temp_c > 40.0) return false;                 // Forbid update under thermal load
    if (factors.free_storage_gb < 1.0) return false;                 // Forbid update if free space < 1GB
    if (factors.active_inference_running) return false;              // Forbid live update during runtime execution
    if (factors.watchdog_escalation_active) return false;            // Forbid update if system is unstable
    if (factors.safe_mode_active) return false;
    if (factors.fragmentation_critical) return false;

    return true;
}

} // namespace isha
