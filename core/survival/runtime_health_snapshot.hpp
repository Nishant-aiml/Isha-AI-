#ifndef ISHA_AI_RUNTIME_HEALTH_SNAPSHOT_HPP
#define ISHA_AI_RUNTIME_HEALTH_SNAPSHOT_HPP

#include <string>
#include <vector>

namespace isha {

struct HealthRecord {
    std::string last_crash_reason;
    std::string last_safemode_trigger;
    unsigned int thermal_collapse_events;
    unsigned int fallback_frequency;
    unsigned int startup_failures;
    unsigned int anr_incidents;
    unsigned int watchdog_escalations;
    int last_degradation_ladder_state;
};

class RuntimeHealthSnapshot {
public:
    static HealthRecord getDefaults();
    
    static bool serialize(const HealthRecord& record, std::vector<uint8_t>& buffer);
    static bool deserialize(const std::vector<uint8_t>& buffer, HealthRecord& record);
};

} // namespace isha

#endif // ISHA_AI_RUNTIME_HEALTH_SNAPSHOT_HPP
