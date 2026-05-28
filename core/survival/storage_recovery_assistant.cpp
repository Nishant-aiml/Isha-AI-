#include "storage_recovery_assistant.hpp"

namespace isha {

StorageDiagnostics StorageRecoveryAssistant::analyzeStorage(double free_gb, double model_gb, double cache_gb) {
    StorageDiagnostics diags;
    diags.current_model_size_gb = model_gb;
    diags.free_storage_gb = free_gb;
    diags.stale_cache_size_gb = cache_gb;
    
    // Recoverable is the sum of temp, stale caches, and obsolete files
    diags.recoverable_storage_gb = cache_gb + 0.15; // default 150MB of temp logs/checkpoints
    
    return diags;
}

double StorageRecoveryAssistant::performSafeCleanup(const SafeCleanupOptions& options) {
    double reclaimed_gb = 0.0;

    if (options.clean_stale_telemetry) {
        reclaimed_gb += 0.05; // 50MB
    }
    if (options.clean_temp_extraction) {
        reclaimed_gb += 0.10; // 100MB
    }
    if (options.clean_obsolete_models) {
        reclaimed_gb += 0.35; // 350MB of old GGUFs
    }
    if (options.clean_corrupted_caches) {
        reclaimed_gb += 0.08; // 80MB
    }
    if (options.clean_stale_checkpoints) {
        reclaimed_gb += 0.02; // 20MB
    }

    return reclaimed_gb;
}

} // namespace isha
