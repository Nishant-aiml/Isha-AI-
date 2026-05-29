#ifndef ISHA_AI_REINSTALL_RECOVERY_POLICY_HPP
#define ISHA_AI_REINSTALL_RECOVERY_POLICY_HPP

#include <string>

namespace isha {

struct CleanupReport {
    int files_purged;
    bool cache_reset;
    bool tokenizer_rebuilt;
};

class ReinstallRecoveryPolicy {
public:
    // Performs comprehensive cleanup of leftover states from previous installations
    static CleanupReport performReinstallSanitization(const std::string& app_dir);
};

} // namespace isha

#endif // ISHA_AI_REINSTALL_RECOVERY_POLICY_HPP
