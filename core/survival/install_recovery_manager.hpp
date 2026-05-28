#ifndef ISHA_AI_INSTALL_RECOVERY_MANAGER_HPP
#define ISHA_AI_INSTALL_RECOVERY_MANAGER_HPP

#include <string>
#include <vector>

namespace isha {

class InstallRecoveryManager {
public:
    enum class SystemStatus {
        STABLE,
        CORRUPTED_MODEL,
        CORRUPTED_LIBS,
        ABI_MISMATCH,
        LOW_STORAGE_STARVATION,
        STALE_UPGRADE_DIRTY
    };

    struct RecoveryReport {
        bool success;
        std::string recovery_details;
        std::string restored_model_version;
    };

    static std::string statusToString(SystemStatus status);

    // Main diagnostic checks
    static SystemStatus diagnosticSelfCheck();

    // Trigger full safe mode rollback recovery
    static RecoveryReport performEmergencyRollback(SystemStatus status);
};

} // namespace isha

#endif // ISHA_AI_INSTALL_RECOVERY_MANAGER_HPP
