#ifndef ISHA_AI_APK_UPDATE_MIGRATION_VALIDATION_HPP
#define ISHA_AI_APK_UPDATE_MIGRATION_VALIDATION_HPP

#include <string>

namespace isha {

struct MigrationStatus {
    bool migration_success;
    bool rollback_triggered;
    bool stale_mmap_cleaned;
    bool tokenizer_realigned;
    std::string error_message;
};

class ApkUpdateMigrationValidation {
public:
    // Performs validation of the update path and migrates existing assets
    static MigrationStatus validateAndMigrate(const std::string& old_version, 
                                              const std::string& new_version, 
                                              const std::string& model_dir);

    // Forces cleanup of any partial or orphaned update extractions
    static void cleanupOrphanedArtifacts(const std::string& temp_dir);
};

} // namespace isha

#endif // ISHA_AI_APK_UPDATE_MIGRATION_VALIDATION_HPP
