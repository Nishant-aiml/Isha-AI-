#include "deployment_health_summary.hpp"
#include <sstream>
#include <iomanip>

namespace isha {

std::string DeploymentHealthSummary::generateHealthReport(const HealthStats& stats) {
    std::stringstream ss;
    ss << "=== ISHA AI DEPLOYMENT HEALTH SUMMARY ===\n";
    ss << "System Status: ";
    
    // Evaluate high-level status
    bool critical = (stats.crash_count >= 5 || stats.safemode_count >= 5 || stats.thermal_collapse_count >= 5);
    bool degraded = (stats.crash_count >= 2 || stats.safemode_count >= 2 || stats.thermal_collapse_count >= 2 || stats.storage_pressure_events >= 3);

    if (critical) {
        ss << "CRITICAL (Self-preservation loops active)\n";
    } else if (degraded) {
        ss << "DEGRADED (Throttling active)\n";
    } else {
        ss << "HEALTHY (Optimal execution parameters)\n";
    }

    ss << "-----------------------------------------\n";
    ss << "Crashes: " << stats.crash_count << "\n";
    ss << "Safe Mode Activations: " << stats.safemode_count << "\n";
    ss << "Thermal Collapses: " << stats.thermal_collapse_count << "\n";
    ss << "Startup Instability Count: " << stats.startup_instability_count << "\n";
    
    ss << std::fixed << std::setprecision(2);
    ss << "Memory Fragmentation: " << (stats.fragmentation_ratio * 100.0) << "%\n";
    ss << "Battery Drain Rate: " << stats.battery_drain_rate << " %/hr\n";
    
    ss << "Storage Pressure Events: " << stats.storage_pressure_events << "\n";
    ss << "Watchdog Escalations: " << stats.watchdog_escalations << "\n";
    
    double success_rate = 100.0;
    if (stats.recovery_attempts > 0) {
        success_rate = (static_cast<double>(stats.recovery_successes) / stats.recovery_attempts) * 100.0;
    }
    ss << "Recovery Success Rate: " << success_rate << "% (" << stats.recovery_successes << "/" << stats.recovery_attempts << ")\n";
    ss << "=========================================";

    return ss.str();
}

} // namespace isha
