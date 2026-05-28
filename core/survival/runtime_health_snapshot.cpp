#include "runtime_health_snapshot.hpp"
#include <cstring>

namespace isha {

HealthRecord RuntimeHealthSnapshot::getDefaults() {
    HealthRecord record;
    record.last_crash_reason = "NONE";
    record.last_safemode_trigger = "NONE";
    record.thermal_collapse_events = 0;
    record.fallback_frequency = 0;
    record.startup_failures = 0;
    record.anr_incidents = 0;
    record.watchdog_escalations = 0;
    record.last_degradation_ladder_state = 0; // DegradationStep::NORMAL
    return record;
}

bool RuntimeHealthSnapshot::serialize(const HealthRecord& record, std::vector<uint8_t>& buffer) {
    buffer.clear();
    
    auto writeInt = [&](unsigned int val) {
        uint8_t bytes[4];
        std::memcpy(bytes, &val, 4);
        for (int i = 0; i < 4; ++i) buffer.push_back(bytes[i]);
    };

    auto writeString = [&](const std::string& str) {
        writeInt(static_cast<unsigned int>(str.size()));
        for (char c : str) buffer.push_back(static_cast<uint8_t>(c));
    };

    writeString(record.last_crash_reason);
    writeString(record.last_safemode_trigger);
    writeInt(record.thermal_collapse_events);
    writeInt(record.fallback_frequency);
    writeInt(record.startup_failures);
    writeInt(record.anr_incidents);
    writeInt(record.watchdog_escalations);
    writeInt(static_cast<unsigned int>(record.last_degradation_ladder_state));

    return true;
}

bool RuntimeHealthSnapshot::deserialize(const std::vector<uint8_t>& buffer, HealthRecord& record) {
    if (buffer.size() < 24) return false;

    size_t offset = 0;

    auto readInt = [&](unsigned int& val) -> bool {
        if (offset + 4 > buffer.size()) return false;
        std::memcpy(&val, &buffer[offset], 4);
        offset += 4;
        return true;
    };

    auto readString = [&](std::string& str) -> bool {
        unsigned int len = 0;
        if (!readInt(len)) return false;
        if (offset + len > buffer.size()) return false;
        str.clear();
        for (unsigned int i = 0; i < len; ++i) {
            str.push_back(static_cast<char>(buffer[offset++]));
        }
        return true;
    };

    unsigned int deg_state = 0;

    if (!readString(record.last_crash_reason)) return false;
    if (!readString(record.last_safemode_trigger)) return false;
    if (!readInt(record.thermal_collapse_events)) return false;
    if (!readInt(record.fallback_frequency)) return false;
    if (!readInt(record.startup_failures)) return false;
    if (!readInt(record.anr_incidents)) return false;
    if (!readInt(record.watchdog_escalations)) return false;
    if (!readInt(deg_state)) return false;

    record.last_degradation_ladder_state = static_cast<int>(deg_state);
    return true;
}

} // namespace isha
