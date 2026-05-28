#ifndef ISHA_AI_MODEL_LIFECYCLE_CONTROLLER_HPP
#define ISHA_AI_MODEL_LIFECYCLE_CONTROLLER_HPP

#include <atomic>
#include <cstdint>
#include <string>
#include "../config/device_profile.hpp"
#include "model_registry.hpp"
#include "model_manager.hpp"
#include "tokenizer_manager.hpp"

namespace isha {

struct LoadResult {
    bool success = false;
    std::string error;
    double load_latency_ms = 0.0;
    unsigned int ram_consumed_mb = 0;
    std::string model_id;
};

class ModelLifecycleController {
public:
    ModelLifecycleController(
        ModelRegistry& registry,
        TokenizerManager& tokenizer_mgr,
        ModelManager& model_mgr
    );

    // Full transactional load: registry lookup → compatibility → checksum → tokenizer → model load
    // If ANY step fails: full rollback
    LoadResult loadModel(
        const std::string& model_id,
        const DeviceProfile& profile,
        double temperature_c
    );

    // Clean unload with tokenizer cleanup
    void unloadModel(const std::string& model_id);

    // Pre-flight validation only (no load)
    bool preflightCheck(
        const std::string& model_id,
        const DeviceProfile& profile,
        double temperature_c
    ) const;

    // Prewarm: load tokenizer only (lazy preparation)
    bool prewarmTokenizer(const std::string& model_id);

    // Stats
    uint32_t successfulLoads() const;
    uint32_t failedLoads() const;
    uint32_t rollbackCount() const;

private:
    ModelRegistry& registry_;
    TokenizerManager& tokenizer_mgr_;
    ModelManager& model_mgr_;

    std::atomic<uint32_t> successful_loads_{0};
    std::atomic<uint32_t> failed_loads_{0};
    std::atomic<uint32_t> rollback_count_{0};
};

} // namespace isha

#endif // ISHA_AI_MODEL_LIFECYCLE_CONTROLLER_HPP
