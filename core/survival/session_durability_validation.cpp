#include "session_durability_validation.hpp"
#include <fstream>
#include <iostream>

namespace isha {

bool SessionDurabilityValidation::saveSessionStateAtomic(const std::string& filepath, const SessionState& state) {
    std::string temp_path = filepath + ".tmp";
    std::ofstream out(temp_path, std::ios::binary);
    if (!out) return false;

    // Write transactional marker
    char marker = state.in_transaction ? 1 : 0;
    out.write(&marker, 1);

    int pos = state.last_token_pos;
    out.write(reinterpret_cast<const char*>(&pos), 4);

    unsigned int size = static_cast<unsigned int>(state.history_ids.size());
    out.write(reinterpret_cast<const char*>(&size), 4);
    if (size > 0) {
        out.write(reinterpret_cast<const char*>(state.history_ids.data()), size * sizeof(int));
    }

    out.flush();
    out.close();

    // Atomic rename simulation / replacement
    std::remove(filepath.c_str());
    return std::rename(temp_path.c_str(), filepath.c_str()) == 0;
}

bool SessionDurabilityValidation::loadAndVerifyState(const std::string& filepath, SessionState& out_state) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in) return false;

    char marker = 0;
    in.read(&marker, 1);
    out_state.in_transaction = (marker == 1);

    int pos = 0;
    in.read(reinterpret_cast<char*>(&pos), 4);
    out_state.last_token_pos = pos;

    unsigned int size = 0;
    in.read(reinterpret_cast<char*>(&size), 4);
    out_state.history_ids.resize(size);
    if (size > 0) {
        in.read(reinterpret_cast<char*>(out_state.history_ids.data()), size * sizeof(int));
    }

    return true;
}

} // namespace isha
