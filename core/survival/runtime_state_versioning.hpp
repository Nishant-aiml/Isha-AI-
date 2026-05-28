#ifndef ISHA_AI_RUNTIME_STATE_VERSIONING_HPP
#define ISHA_AI_RUNTIME_STATE_VERSIONING_HPP

#include <string>
#include <vector>

namespace isha {

struct RuntimeState {
    int version;
    std::string tokenizer_state_hash;
    int mmap_flags;
    int session_version;
    int telemetry_version;
    int safe_mode_boot_count;
    std::string build_id;
};

class RuntimeStateVersioning {
public:
    static const int CURRENT_VERSION = 10;

    static RuntimeState getDefaults();
    static bool verifyVersion(const RuntimeState& state);
    static bool migrateState(const RuntimeState& old_state, RuntimeState& new_state);
    static bool serialize(const RuntimeState& state, std::vector<uint8_t>& buffer);
    static bool deserialize(const std::vector<uint8_t>& buffer, RuntimeState& state);
};

} // namespace isha

#endif // ISHA_AI_RUNTIME_STATE_VERSIONING_HPP
