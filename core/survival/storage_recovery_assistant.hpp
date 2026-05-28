#ifndef ISHA_AI_STORAGE_RECOVERY_ASSISTANT_HPP
#define ISHA_AI_STORAGE_RECOVERY_ASSISTANT_HPP

#include <string>

namespace isha {

struct StorageDiagnostics {
    double current_model_size_gb;
    double free_storage_gb;
    double stale_cache_size_gb;
    double recoverable_storage_gb;
};

struct SafeCleanupOptions {
    bool clean_stale_telemetry;
    bool clean_temp_extraction;
    bool clean_obsolete_models;
    bool clean_corrupted_caches;
    bool clean_stale_checkpoints;
};

class StorageRecoveryAssistant {
public:
    // Generate clean diagnostics
    static StorageDiagnostics analyzeStorage(double free_gb, double model_gb, double cache_gb);

    // Executes a transactional, user-approved recovery purge
    static double performSafeCleanup(const SafeCleanupOptions& options);
};

} // namespace isha

#endif // ISHA_AI_STORAGE_RECOVERY_ASSISTANT_HPP
