#include "first_week_retention_validation.hpp"

namespace isha {

double FirstWeekRetentionValidation::calculateUninstallProbability(const RetentionRiskMetrics& metrics) {
    double risk = 0.0;

    // Weight indicators representing negative UX impressions
    risk += metrics.startup_abandonment_count * 0.25;
    risk += metrics.thermal_exit_count * 0.20;
    risk += metrics.battery_complaint_count * 0.15;
    risk += metrics.safe_mode_entry_count * 0.30;
    risk += metrics.recovery_dismissal_count * 0.10;
    risk += metrics.ui_lag_event_count * 0.05;

    if (risk > 1.0) risk = 1.0;
    if (risk < 0.0) risk = 0.0;

    return risk;
}

RiskLevel FirstWeekRetentionValidation::getRiskLevel(double probability) {
    if (probability < 0.25) return RiskLevel::LOW;
    if (probability < 0.50) return RiskLevel::MODERATE;
    if (probability < 0.75) return RiskLevel::HIGH;
    return RiskLevel::CRITICAL;
}

RetentionAutoResponse FirstWeekRetentionValidation::evaluateMitigations(RiskLevel level) {
    RetentionAutoResponse resp = { false, false, false, false, false };
    if (level == RiskLevel::HIGH || level == RiskLevel::CRITICAL) {
        resp.reduce_batching = true;
        resp.reduce_telemetry = true;
        resp.reduce_checkpoint_freq = true;
        resp.reduce_acceleration = true;
        resp.increase_cooldown = true;
    }
    return resp;
}


} // namespace isha
