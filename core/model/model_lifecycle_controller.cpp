#include "model_lifecycle_controller.hpp"
#include "../logging/logger.hpp"
#include <chrono>

namespace isha {

ModelLifecycleController::ModelLifecycleController(
    ModelRegistry& registry,
    TokenizerManager& tokenizer_mgr,
    ModelManager& model_mgr)
    : registry_(registry),
      tokenizer_mgr_(tokenizer_mgr),
      model_mgr_(model_mgr) {}

LoadResult ModelLifecycleController::loadModel(
    const std::string& model_id,
    const DeviceProfile& profile,
    double temperature_c) {

    LoadResult result;
    result.model_id = model_id;

    auto start = std::chrono::high_resolution_clock::now();

    // Step 1: Registry lookup
    ISHA_LOG_INFO("ModelLifecycle", "Step 1/5: Looking up model '" + model_id + "' in registry.");
    const ModelEntry* entry = registry_.findModel(model_id);
    if (!entry) {
        result.error = "Model '" + model_id + "' not found in registry";
        failed_loads_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_ERROR("ModelLifecycle", result.error);
        return result;
    }

    // Step 2: Compatibility check
    ISHA_LOG_INFO("ModelLifecycle", "Step 2/5: Checking compatibility for model '" + model_id + "'.");
    CompatibilityResult compat = registry_.checkCompatibility(model_id, profile, temperature_c);
    if (!compat.compatible) {
        result.error = "Model '" + model_id + "' incompatible: " + compat.reason;
        failed_loads_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_ERROR("ModelLifecycle", result.error);
        return result;
    }

    // Step 3: Validation (checksum)
    ISHA_LOG_INFO("ModelLifecycle", "Step 3/5: Validating model '" + model_id + "'.");
    if (!registry_.validateModel(model_id)) {
        result.error = "Model '" + model_id + "' validation failed";
        failed_loads_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_ERROR("ModelLifecycle", result.error);
        return result;
    }

    // Step 4: Tokenizer load
    ISHA_LOG_INFO("ModelLifecycle", "Step 4/5: Loading tokenizer for model '" + model_id + "'.");
    if (!tokenizer_mgr_.loadTokenizer(model_id, entry->filepath)) {
        result.error = "Tokenizer load failed for model '" + model_id + "'";
        failed_loads_.fetch_add(1, std::memory_order_relaxed);
        rollback_count_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_ERROR("ModelLifecycle", result.error);
        // No rollback needed — steps 1-3 are read-only
        return result;
    }

    // Step 5: Model load
    ISHA_LOG_INFO("ModelLifecycle", "Step 5/5: Loading model '" + model_id + "' (" 
                  + std::to_string(entry->estimated_ram_mb) + "MB).");
    if (!model_mgr_.loadModel(entry->model_id, entry->estimated_ram_mb)) {
        // Rollback step 4: unload tokenizer
        ISHA_LOG_WARN("ModelLifecycle", "Model load failed. Rolling back tokenizer for '" 
                      + model_id + "'.");
        tokenizer_mgr_.unloadTokenizer(model_id);

        result.error = "Model load failed for '" + model_id + "'";
        failed_loads_.fetch_add(1, std::memory_order_relaxed);
        rollback_count_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_ERROR("ModelLifecycle", result.error);
        return result;
    }

    // Success
    auto end = std::chrono::high_resolution_clock::now();
    result.success = true;
    result.load_latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
    result.ram_consumed_mb = entry->estimated_ram_mb;

    successful_loads_.fetch_add(1, std::memory_order_relaxed);
    ISHA_LOG_INFO("ModelLifecycle", "Model '" + model_id + "' loaded successfully in " 
                  + std::to_string(result.load_latency_ms) + "ms (" 
                  + std::to_string(result.ram_consumed_mb) + "MB).");

    return result;
}

void ModelLifecycleController::unloadModel(const std::string& model_id) {
    ISHA_LOG_INFO("ModelLifecycle", "Unloading model '" + model_id + "'.");
    model_mgr_.unloadModel();
    tokenizer_mgr_.unloadTokenizer(model_id);
    ISHA_LOG_INFO("ModelLifecycle", "Model '" + model_id + "' unloaded with tokenizer cleanup.");
}

bool ModelLifecycleController::preflightCheck(
    const std::string& model_id,
    const DeviceProfile& profile,
    double temperature_c) const {

    ISHA_LOG_INFO("ModelLifecycle", "Preflight check for model '" + model_id + "'.");

    // Step 1: Registry lookup
    const ModelEntry* entry = registry_.findModel(model_id);
    if (!entry) {
        ISHA_LOG_WARN("ModelLifecycle", "Preflight failed: model '" + model_id + "' not found.");
        return false;
    }

    // Step 2: Compatibility check
    CompatibilityResult compat = registry_.checkCompatibility(model_id, profile, temperature_c);
    if (!compat.compatible) {
        ISHA_LOG_WARN("ModelLifecycle", "Preflight failed: model '" + model_id 
                      + "' incompatible — " + compat.reason);
        return false;
    }

    // Step 3: Validation — note: validateModel is non-const, so we use a const-compatible path
    // For preflight, we check if the model entry reports validity
    if (!entry->is_valid && !entry->checksum_verified) {
        ISHA_LOG_WARN("ModelLifecycle", "Preflight failed: model '" + model_id 
                      + "' not yet validated.");
        return false;
    }

    ISHA_LOG_INFO("ModelLifecycle", "Preflight check passed for model '" + model_id + "'.");
    return true;
}

bool ModelLifecycleController::prewarmTokenizer(const std::string& model_id) {
    ISHA_LOG_INFO("ModelLifecycle", "Prewarming tokenizer for model '" + model_id + "'.");

    const ModelEntry* entry = registry_.findModel(model_id);
    if (!entry) {
        ISHA_LOG_WARN("ModelLifecycle", "Prewarm failed: model '" + model_id + "' not found.");
        return false;
    }

    if (tokenizer_mgr_.hasTokenizer(model_id)) {
        ISHA_LOG_INFO("ModelLifecycle", "Tokenizer already cached for model '" + model_id + "'.");
        return true;
    }

    bool loaded = tokenizer_mgr_.loadTokenizer(model_id, entry->filepath);
    if (loaded) {
        ISHA_LOG_INFO("ModelLifecycle", "Tokenizer prewarmed for model '" + model_id + "'.");
    } else {
        ISHA_LOG_ERROR("ModelLifecycle", "Tokenizer prewarm failed for model '" + model_id + "'.");
    }
    return loaded;
}

uint32_t ModelLifecycleController::successfulLoads() const {
    return successful_loads_.load(std::memory_order_relaxed);
}

uint32_t ModelLifecycleController::failedLoads() const {
    return failed_loads_.load(std::memory_order_relaxed);
}

uint32_t ModelLifecycleController::rollbackCount() const {
    return rollback_count_.load(std::memory_order_relaxed);
}

} // namespace isha
