#include "runtime_state_versioning.hpp"
#include <cstring>
#include <sstream>

namespace isha {

RuntimeState RuntimeStateVersioning::getDefaults() {
    RuntimeState state;
    state.version = CURRENT_VERSION;
    state.tokenizer_state_hash = "INIT_HASH_V10";
    state.mmap_flags = 1; // Default memory mapped setup
    state.session_version = 1;
    state.telemetry_version = 1;
    state.safe_mode_boot_count = 0;
    state.build_id = "ISHA_AI_RELEASE_V1.0";
    return state;
}

bool RuntimeStateVersioning::verifyVersion(const RuntimeState& state) {
    if (state.version != CURRENT_VERSION) {
        return false;
    }
    if (state.build_id != "ISHA_AI_RELEASE_V1.0") {
        return false;
    }
    return true;
}

bool RuntimeStateVersioning::migrateState(const RuntimeState& old_state, RuntimeState& new_state) {
    // Basic migration implementation: copy fields, update version
    new_state = getDefaults();
    new_state.tokenizer_state_hash = old_state.tokenizer_state_hash;
    new_state.mmap_flags = old_state.mmap_flags;
    new_state.session_version = old_state.session_version;
    new_state.telemetry_version = old_state.telemetry_version;
    new_state.safe_mode_boot_count = old_state.safe_mode_boot_count;
    new_state.version = CURRENT_VERSION; // Upgrade version
    return true;
}

bool RuntimeStateVersioning::serialize(const RuntimeState& state, std::vector<uint8_t>& buffer) {
    buffer.clear();
    
    // Quick binary serialization for stable performance
    auto writeInt = [&](int val) {
        uint8_t bytes[4];
        std::memcpy(bytes, &val, 4);
        for (int i = 0; i < 4; ++i) buffer.push_back(bytes[i]);
    };

    auto writeString = [&](const std::string& str) {
        writeInt(static_cast<int>(str.size()));
        for (char c : str) buffer.push_back(static_cast<uint8_t>(c));
    };

    writeInt(state.version);
    writeString(state.tokenizer_state_hash);
    writeInt(state.mmap_flags);
    writeInt(state.session_version);
    writeInt(state.telemetry_version);
    writeInt(state.safe_mode_boot_count);
    writeString(state.build_id);

    return true;
}

bool RuntimeStateVersioning::deserialize(const std::vector<uint8_t>& buffer, RuntimeState& state) {
    if (buffer.size() < 24) return false; // Minimum size bound check

    size_t offset = 0;

    auto readInt = [&](int& val) -> bool {
        if (offset + 4 > buffer.size()) return false;
        std::memcpy(&val, &buffer[offset], 4);
        offset += 4;
        return true;
    };

    auto readString = [&](std::string& str) -> bool {
        int len = 0;
        if (!readInt(len)) return false;
        if (len < 0 || offset + len > buffer.size()) return false;
        str.clear();
        for (int i = 0; i < len; ++i) {
            str.push_back(static_cast<char>(buffer[offset++]));
        }
        return true;
    };

    if (!readInt(state.version)) return false;
    if (!readString(state.tokenizer_state_hash)) return false;
    if (!readInt(state.mmap_flags)) return false;
    if (!readInt(state.session_version)) return false;
    if (!readInt(state.telemetry_version)) return false;
    if (!readInt(state.safe_mode_boot_count)) return false;
    if (!readString(state.build_id)) return false;

    return true;
}

} // namespace isha
