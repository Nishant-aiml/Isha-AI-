#ifndef ISHA_AI_SESSION_DURABILITY_VALIDATION_HPP
#define ISHA_AI_SESSION_DURABILITY_VALIDATION_HPP

#include <string>
#include <vector>

namespace isha {

struct SessionState {
    int last_token_pos;
    std::vector<int> history_ids;
    bool in_transaction;
};

class SessionDurabilityValidation {
public:
    // Performs an atomic write of the session state
    static bool saveSessionStateAtomic(const std::string& filepath, const SessionState& state);

    // Restores and validates session state integrity
    static bool loadAndVerifyState(const std::string& filepath, SessionState& out_state);
};

} // namespace isha

#endif // ISHA_AI_SESSION_DURABILITY_VALIDATION_HPP
