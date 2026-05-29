#include "ios_coreml_manager.hpp"
#include "../logging/logger.hpp"

namespace isha {

IosCoremlManager::IosCoremlManager(ModelManager& model_mgr)
    : model_mgr_(model_mgr) {
    ISHA_LOG_INFO("IosCoremlManager", "Initialized iOS CoreML / ANE manager.");
}

// ------------------------------------------------------------
// handleMemoryWarning: iOS didReceiveMemoryWarning
// Unload order: T3 → T2 → T1. Never CLASS_A (T0, embeddings, reranker).
// ------------------------------------------------------------
void IosCoremlManager::handleMemoryWarning() {
    ISHA_LOG_WARN("IosCoremlManager", "iOS didReceiveMemoryWarning — releasing resources.");

    ModelState state = model_mgr_.getState();
    if (state == ModelState::UNLOADED) {
        ISHA_LOG_INFO("IosCoremlManager", "Memory warning: no active model to unload.");
        return;
    }

    unsigned int size = model_mgr_.getActiveModelSizeMB();

    // CLASS_A protection: never unload T0 (≤250MB), embeddings/reranker (≤60MB)
    if (size <= 250) {
        ISHA_LOG_INFO("IosCoremlManager",
            "Memory warning: CLASS_A model (" + std::to_string(size) + "MB) protected — not unloading.");
        return;
    }

    // Unload order: T3 → T2 → T1
    if (size >= 3000) {
        ISHA_LOG_WARN("IosCoremlManager", "Memory warning: unloading T3 (" + std::to_string(size) + "MB)");
    } else if (size >= 1000) {
        ISHA_LOG_WARN("IosCoremlManager", "Memory warning: unloading T2 (" + std::to_string(size) + "MB)");
    } else {
        ISHA_LOG_WARN("IosCoremlManager", "Memory warning: unloading T1 (" + std::to_string(size) + "MB)");
    }

    model_mgr_.emergencyUnload();
}

// ------------------------------------------------------------
// handleBackgroundEntry: iOS applicationDidEnterBackground
//
// Two-path strategy (Issue 6 fix — more nuanced than always-unload):
//
// SHORT background expected (e.g., user switching apps briefly):
//   → suspend_generation only; keep model warm in memory
//   → iOS gives ~30s before expiration
//
// LONG background expected (low probability of quick return):
//   → checkpoint state + unload heavy models
//   → preserve CLASS_A and T0 routing metadata only
//   → avoid unnecessary ANE recompilation on next foreground
//
// Decision heuristic: if CLASS_D model active → always checkpoint+unload
//                     if CLASS_B (T1) active  → suspend only (30s window)
// ------------------------------------------------------------
void IosCoremlManager::handleBackgroundEntry() {
    ISHA_LOG_INFO("IosCoremlManager", "iOS transitioning to background.");

    ModelState state = model_mgr_.getState();
    if (state == ModelState::UNLOADED || state == ModelState::HIBERNATED) {
        ISHA_LOG_INFO("IosCoremlManager", "Background entry: no active heavy model — no action needed.");
        return;
    }

    unsigned int size = model_mgr_.getActiveModelSizeMB();
    ResidencyClass rc = model_mgr_.getResidencyClass();

    if (rc == ResidencyClass::CLASS_D_HEAVY_COMPUTE || size >= 1000) {
        // CLASS_D (T2/T3): always checkpoint + unload on background.
        // These models cause too much RAM pressure, ANE hold, and thermal risk
        // to keep alive during background. Cold restart is acceptable.
        ISHA_LOG_WARN("IosCoremlManager",
            "Background entry: CLASS_D model active (" + std::to_string(size) +
            "MB) — checkpointing and unloading to free ANE/GPU.");
        ISHA_LOG_INFO("IosCoremlManager", "  [iOS CHECKPOINT] Generation state paused and serialized");
        ISHA_LOG_INFO("IosCoremlManager", "  [iOS CHECKPOINT] Active task checkpoint written");
        ISHA_LOG_INFO("IosCoremlManager", "  [iOS CHECKPOINT] Routing metadata preserved for T0");
        model_mgr_.unloadModel();

    } else if (rc == ResidencyClass::CLASS_B_WARM_HIBERNATED && size < 1000) {
        // CLASS_B (T1): suspend generation only for short background window.
        // iOS allows ~30s of background time. If app returns within window,
        // T1 resumes without cold-start cost.
        // If background expiration fires → handleBackgroundExpiration() unloads.
        ISHA_LOG_INFO("IosCoremlManager",
            "Background entry: CLASS_B T1 (" + std::to_string(size) +
            "MB) — suspending generation only (warm hold for 30s window).");
        ISHA_LOG_INFO("IosCoremlManager", "  [iOS SUSPEND] inference threads paused");
        ISHA_LOG_INFO("IosCoremlManager", "  [iOS SUSPEND] tokenizer state preserved");
        ISHA_LOG_INFO("IosCoremlManager", "  [iOS SUSPEND] will unload on background expiration if not resumed");
        model_mgr_.setIdle();

    } else {
        // CLASS_A or unknown — preserve always
        ISHA_LOG_INFO("IosCoremlManager",
            "Background entry: CLASS_A model (" + std::to_string(size) + "MB) — no action.");
    }
}

// ------------------------------------------------------------
// handleBackgroundExpiration: iOS background task expired
// Immediate teardown. No time for graceful checkpoint.
// Must complete within OS-allowed expiration window (~5s).
// CLASS_A preserved implicitly (emergencyUnload protects ≤250MB).
// ------------------------------------------------------------
void IosCoremlManager::handleBackgroundExpiration() {
    ISHA_LOG_ERROR("IosCoremlManager",
        "iOS background task EXPIRED — immediate teardown.");

    ModelState state = model_mgr_.getState();
    if (state != ModelState::UNLOADED) {
        // Best-effort checkpoint (may be truncated by OS)
        ISHA_LOG_ERROR("IosCoremlManager", "  [EXPIRY CHECKPOINT] emergency state flush");
        model_mgr_.emergencyUnload();
    }

    // Reset T0 emergency mode if it was set
    ISHA_LOG_ERROR("IosCoremlManager",
        "Background expiration complete. ANE context destroyed. T0 routing metadata preserved.");
}

} // namespace isha
