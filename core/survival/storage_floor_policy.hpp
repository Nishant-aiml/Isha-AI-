#ifndef ISHA_AI_STORAGE_FLOOR_POLICY_HPP
#define ISHA_AI_STORAGE_FLOOR_POLICY_HPP

#include <string>

namespace isha {

enum class StorageState {
    NORMAL,
    LOW_STORAGE_WARNING,
    SURVIVAL_STORAGE_MODE,
    SAFE_MODE_STORAGE_RECOVERY
};

class StorageFloorPolicy {
public:
    static constexpr double MIN_FREE_STORAGE_GB = 1.0;

    static std::string stateToString(StorageState state);

    // Evaluates the current free disk space and determines active storage tier
    static StorageState determineStorageState(double free_storage_gb);

    // Core validation boundaries
    static bool allowModelDownload(double free_storage_gb, double model_size_gb);
    static bool allowModelUpdate(double free_storage_gb);
    static bool allowExtraction(double free_storage_gb, double extraction_size_gb);

    // Auto cleanup triggers
    struct CleanupActions {
        bool purge_stale_temp;
        bool purge_telemetry_history;
        bool purge_prompt_caches;
    };

    static CleanupActions getRequiredCleanup(StorageState state);
};

} // namespace isha

#endif // ISHA_AI_STORAGE_FLOOR_POLICY_HPP
