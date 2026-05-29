#ifndef ISHA_AI_FIRST_WEEK_RETENTION_VALIDATION_HPP
#define ISHA_AI_FIRST_WEEK_RETENTION_VALIDATION_HPP

namespace isha {

struct RetentionRiskMetrics {
    int startup_abandonment_count;
    int thermal_exit_count;
    int battery_complaint_count;
    int safe_mode_entry_count;
    int recovery_dismissal_count;
    int ui_lag_event_count;
};

enum class RiskLevel {
    LOW,
    MODERATE,
    HIGH,
    CRITICAL
};

struct RetentionAutoResponse {
    bool reduce_batching;
    bool reduce_telemetry;
    bool reduce_checkpoint_freq;
    bool reduce_acceleration;
    bool increase_cooldown;
};

class FirstWeekRetentionValidation {
public:
    // Calculates uninstall risk score
    static double calculateUninstallProbability(const RetentionRiskMetrics& metrics);

    // Maps probability to RiskLevel
    static RiskLevel getRiskLevel(double probability);

    // Determines mitigation responses based on RiskLevel
    static RetentionAutoResponse evaluateMitigations(RiskLevel level);
};


} // namespace isha

#endif // ISHA_AI_FIRST_WEEK_RETENTION_VALIDATION_HPP
