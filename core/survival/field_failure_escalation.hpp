#ifndef ISHA_AI_FIELD_FAILURE_ESCALATION_HPP
#define ISHA_AI_FIELD_FAILURE_ESCALATION_HPP

#include <string>

namespace isha {

struct EscalationCounters {
    unsigned int safemode_count;
    unsigned int thermal_collapse_count;
    unsigned int anr_count;
    unsigned int startup_failure_count;
    unsigned int watchdog_escalation_count;
    unsigned int corruption_recovery_count;
    unsigned int background_failure_count;
};

struct EscalationPolicy {
    bool is_field_critical;
    bool disable_acceleration;
    unsigned int limit_context_size;
    unsigned int limit_batch_size;
    unsigned int telemetry_interval_ms;
    unsigned int watchdog_polling_ms;
    bool force_survival_mode;
    bool recommend_reboot;
};

class FieldFailureEscalation {
public:
    static EscalationPolicy evaluateFailureRates(const EscalationCounters& counters);
};

} // namespace isha

#endif // ISHA_AI_FIELD_FAILURE_ESCALATION_HPP
