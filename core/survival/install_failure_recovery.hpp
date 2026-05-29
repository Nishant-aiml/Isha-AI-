#ifndef ISHA_AI_INSTALL_FAILURE_RECOVERY_HPP
#define ISHA_AI_INSTALL_FAILURE_RECOVERY_HPP

#include <string>

namespace isha {

struct ExtractionResult {
    bool success;
    bool rolled_back;
    bool storage_freed;
    size_t active_bytes;
};

class InstallFailureRecovery {
public:
    // Safely staging extractions transactionally
    static ExtractionResult stageAndUnpack(const std::string& source_archive, 
                                           const std::string& destination_file, 
                                           size_t available_storage_bytes);

    // Forces cleanups of invalid staging artifacts
    static bool verifyAndPurgeStaleInstalls(const std::string& app_dir);
};

} // namespace isha

#endif // ISHA_AI_INSTALL_FAILURE_RECOVERY_HPP
