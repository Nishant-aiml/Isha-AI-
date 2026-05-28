#include "storage_floor_policy.hpp"

namespace isha {

std::string StorageFloorPolicy::stateToString(StorageState state) {
    switch (state) {
        case StorageState::NORMAL: return "NORMAL";
        case StorageState::LOW_STORAGE_WARNING: return "LOW_STORAGE_WARNING";
        case StorageState::SURVIVAL_STORAGE_MODE: return "SURVIVAL_STORAGE_MODE";
        case StorageState::SAFE_MODE_STORAGE_RECOVERY: return "SAFE_MODE_STORAGE_RECOVERY";
        default: return "UNKNOWN";
    }
}

StorageState StorageFloorPolicy::determineStorageState(double free_storage_gb) {
    if (free_storage_gb >= 2.0) {
        return StorageState::NORMAL;
    } else if (free_storage_gb >= MIN_FREE_STORAGE_GB) {
        return StorageState::LOW_STORAGE_WARNING;
    } else if (free_storage_gb >= 0.5) {
        return StorageState::SURVIVAL_STORAGE_MODE;
    } else {
        return StorageState::SAFE_MODE_STORAGE_RECOVERY;
    }
}

bool StorageFloorPolicy::allowModelDownload(double free_storage_gb, double model_size_gb) {
    // downloads require at least MIN_FREE_STORAGE_GB remaining AFTER download completes
    return (free_storage_gb - model_size_gb) >= MIN_FREE_STORAGE_GB;
}

bool StorageFloorPolicy::allowModelUpdate(double free_storage_gb) {
    return free_storage_gb >= MIN_FREE_STORAGE_GB;
}

bool StorageFloorPolicy::allowExtraction(double free_storage_gb, double extraction_size_gb) {
    return (free_storage_gb - extraction_size_gb) >= 0.5; // Survival margin limit
}

StorageFloorPolicy::CleanupActions StorageFloorPolicy::getRequiredCleanup(StorageState state) {
    CleanupActions actions = {false, false, false};
    switch (state) {
        case StorageState::NORMAL:
            break;
        case StorageState::LOW_STORAGE_WARNING:
            actions.purge_stale_temp = true;
            break;
        case StorageState::SURVIVAL_STORAGE_MODE:
            actions.purge_stale_temp = true;
            actions.purge_telemetry_history = true;
            break;
        case StorageState::SAFE_MODE_STORAGE_RECOVERY:
            actions.purge_stale_temp = true;
            actions.purge_telemetry_history = true;
            actions.purge_prompt_caches = true;
            break;
    }
    return actions;
}

} // namespace isha
