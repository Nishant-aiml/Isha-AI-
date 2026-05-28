#include <iostream>
#include <cassert>
#include "survival/deployment_health_summary.hpp"

using namespace isha;

int main() {
    std::cout << "Starting deployment_health_summary_benchmark..." << std::endl;

    HealthStats stats;
    stats.crash_count = 0;
    stats.safemode_count = 0;
    stats.thermal_collapse_count = 0;
    stats.startup_instability_count = 0;
    stats.fragmentation_ratio = 0.1;
    stats.battery_drain_rate = 1.5;
    stats.storage_pressure_events = 0;
    stats.watchdog_escalations = 0;
    stats.recovery_attempts = 1;
    stats.recovery_successes = 1;

    std::string report = DeploymentHealthSummary::generateHealthReport(stats);
    assert(!report.empty());
    assert(report.find("HEALTHY") != std::string::npos);

    stats.crash_count = 6;
    report = DeploymentHealthSummary::generateHealthReport(stats);
    assert(report.find("CRITICAL") != std::string::npos);

    std::cout << "deployment_health_summary_benchmark PASSED!" << std::endl;
    return 0;
}
