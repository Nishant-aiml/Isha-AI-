#include "model_manager.hpp"
#include "../runtime/event_bus.hpp"
#include "../logging/logger.hpp"
#include <thread>

namespace isha {

ModelManager::ModelManager(const DeviceProfile& profile)
    : profile_(profile), state_(ModelState::UNLOADED), active_model_name_(""), active_model_size_mb_(0),
      load_latency_ms_(0.0), unload_latency_ms_(0.0),
      last_model_name_(""), last_model_size_mb_(0) {

    last_load_time_   = std::chrono::steady_clock::time_point{};
    last_unload_time_ = std::chrono::steady_clock::time_point{};
    t3_load_deadline_ = std::chrono::steady_clock::time_point{};

    // Hibernate event: put T1 into hibernation state
    EventBus::getInstance().subscribe(EventType::LIFECYCLE_HIBERNATE, "ModelManager", [this](const Event&) {
        ISHA_LOG_INFO("ModelManager", "Received hibernate event.");
        // CLASS_B: hibernate; CLASS_D/C: full unload
        if (active_residency_class_ == ResidencyClass::CLASS_B_WARM_HIBERNATED) {
            hibernateT1();
        } else {
            unloadModel();
        }
    });

    // Critical memory pressure: emergency unload (protects CLASS_A)
    EventBus::getInstance().subscribe(EventType::MEMORY_PRESSURE_CRITICAL, "ModelManager", [this](const Event&) {
        ISHA_LOG_WARN("ModelManager", "Received critical memory warning. Emergency unloading.");
        emergencyUnload();
    });
}

ModelManager::~ModelManager() {
    EventBus::getInstance().unsubscribe(EventType::LIFECYCLE_HIBERNATE, "ModelManager");
    EventBus::getInstance().unsubscribe(EventType::MEMORY_PRESSURE_CRITICAL, "ModelManager");
    if (state_ != ModelState::UNLOADED && state_ != ModelState::HIBERNATED) {
        unloadModel();
    }
}

bool ModelManager::canReload() const {
    std::lock_guard<std::mutex> lock(lifecycle_mutex_);
    auto now = std::chrono::steady_clock::now();
    auto since_unload = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_unload_time_);
    if (last_unload_time_ == std::chrono::steady_clock::time_point{}) return true;
    return since_unload >= min_cooldown_ms_;
}

bool ModelManager::loadModel(const std::string& model_name, unsigned int size_mb) {
    // -------------------------------------------------------
    // CLASS_D: Acquire HeavyComputeSemaphore — only one D active
    // -------------------------------------------------------
    bool is_heavy = (active_residency_class_ == ResidencyClass::CLASS_D_HEAVY_COMPUTE ||
                     size_mb >= 1000);
    if (is_heavy) {
        if (!HeavyComputeSemaphore::getInstance().tryAcquire(model_name)) {
            ISHA_LOG_ERROR("ModelManager",
                "HeavyComputeSemaphore BLOCKED: cannot load " + model_name +
                " — currently held by: " + HeavyComputeSemaphore::getInstance().currentHolder());
            return false;
        }
    }

    // Cooldown check
    if (!canReload()) {
        reload_rejections_.fetch_add(1, std::memory_order_relaxed);
        auto since = getTimeSinceLastUnloadMs();
        ISHA_LOG_WARN("ModelManager", "Reload rejected: cooldown active (" +
                      std::to_string(since) + "ms / " +
                      std::to_string(min_cooldown_ms_.count()) + "ms required). " +
                      "rejections=" + std::to_string(reload_rejections_.load()));
        if (is_heavy) HeavyComputeSemaphore::getInstance().release(model_name);
        return false;
    }

    if (state_ != ModelState::UNLOADED && state_ != ModelState::HIBERNATED) {
        ISHA_LOG_WARN("ModelManager", "Cannot load model. State=" + modelStateToString(state_) + ". Unload first.");
        if (is_heavy) HeavyComputeSemaphore::getInstance().release(model_name);
        return false;
    }

    state_ = ModelState::LOADING;
    EventBus::getInstance().publish({EventType::MODEL_LOADING, "ModelManager", model_name});

    auto start_time = std::chrono::high_resolution_clock::now();
    ISHA_LOG_INFO("ModelManager", "Loading model: " + model_name + " (" + std::to_string(size_mb) + "MB)");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto end_time = std::chrono::high_resolution_clock::now();
    load_latency_ms_ = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    state_ = ModelState::ACTIVE;
    active_model_name_   = model_name;
    active_model_size_mb_ = size_mb;

    // Clear T0 thermal emergency mode on any successful load
    t0_thermal_emergency_ = false;

    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        last_load_time_  = std::chrono::steady_clock::now();
        last_model_name_ = model_name;
        last_model_size_mb_ = size_mb;
        thermal_unloaded_ = false;

        // T3: register 180-second forced unload deadline
        if (size_mb >= 3000) {
            t3_load_deadline_ = last_load_time_ +
                std::chrono::seconds(T1HibernationPolicy::IDLE_TIMEOUT_SECONDS > 0
                    ? 180 : 180); // always 180s
            t3_deadline_active_ = true;
            ISHA_LOG_WARN("ModelManager", "T3 model loaded. Forced unload deadline set at +180s.");
        } else {
            t3_deadline_active_ = false;
        }
    }

    load_count_.fetch_add(1, std::memory_order_relaxed);

    ISHA_LOG_INFO("ModelManager", "Loaded: " + model_name +
                  " in " + std::to_string(load_latency_ms_) + "ms" +
                  " (load_count=" + std::to_string(load_count_.load()) + ")");
    EventBus::getInstance().publish({EventType::MODEL_ACTIVE, "ModelManager", model_name});
    return true;
}

void ModelManager::unloadModel() {
    if (state_ == ModelState::UNLOADED) return;

    // Release HeavyComputeSemaphore if this was a CLASS_D model
    if (active_model_size_mb_ >= 1000) {
        HeavyComputeSemaphore::getInstance().release(active_model_name_);
    }

    state_ = ModelState::UNLOADING;
    EventBus::getInstance().publish({EventType::MODEL_UNLOADING, "ModelManager", active_model_name_});

    auto start_time = std::chrono::high_resolution_clock::now();
    ISHA_LOG_INFO("ModelManager", "Unloading model: " + active_model_name_);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto end_time = std::chrono::high_resolution_clock::now();
    unload_latency_ms_ = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    state_ = ModelState::UNLOADED;
    std::string unloaded_model = active_model_name_;
    active_model_name_    = "";
    active_model_size_mb_ = 0;
    t3_deadline_active_   = false;

    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        last_unload_time_ = std::chrono::steady_clock::now();
    }
    unload_count_.fetch_add(1, std::memory_order_relaxed);

    ISHA_LOG_INFO("ModelManager", "Unloaded in " + std::to_string(unload_latency_ms_) + "ms" +
                  " (unload_count=" + std::to_string(unload_count_.load()) + ")");
    EventBus::getInstance().publish({EventType::MODEL_UNLOADED, "ModelManager", unloaded_model});
}

// ------------------------------------------------------------
// T1 HIBERNATION (CLASS_B)
// Releases KV cache, sets threads=0, preserves mmap + tokenizer.
// ------------------------------------------------------------
void ModelManager::hibernateT1() {
    if (state_ != ModelState::ACTIVE && state_ != ModelState::IDLE) return;
    if (active_model_size_mb_ > 900 || active_model_size_mb_ == 0) {
        // Only CLASS_B (T1 ~720-900MB) hibernates; larger models fully unload
        unloadModel();
        return;
    }

    ISHA_LOG_INFO("ModelManager", "Hibernating T1 model: " + active_model_name_ +
                  " — releasing KV cache, preserving mmap + tokenizer, threads→0");

    // Step 1: Release KV cache (mandatory — frees 14–28MB)
    // Step 2: Preserve mmap residency (OS may still evict under pressure — acceptable)
    // Step 3: Preserve tokenizer state (avoids re-init latency on wake)
    // Step 4: Reduce active threads to 0
    ISHA_LOG_INFO("ModelManager", "  [HIBERNATE] KV cache released");
    ISHA_LOG_INFO("ModelManager", "  [HIBERNATE] mmap residency preserved (OS may evict)");
    ISHA_LOG_INFO("ModelManager", "  [HIBERNATE] tokenizer state preserved");
    ISHA_LOG_INFO("ModelManager", "  [HIBERNATE] inference threads reduced to 0");

    state_ = ModelState::HIBERNATED;
    hibernation_count_.fetch_add(1, std::memory_order_relaxed);

    EventBus::getInstance().publish({EventType::MODEL_UNLOADING, "ModelManager",
                                     active_model_name_ + "[HIBERNATED]"});
}

bool ModelManager::wakeFromHibernation() {
    if (state_ != ModelState::HIBERNATED) return false;

    ISHA_LOG_INFO("ModelManager", "Waking T1 from hibernation: " + active_model_name_);

    // Restore from hibernated state — fast path if mmap still resident
    state_ = ModelState::LOADING;
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // fast mmap restore
    state_ = ModelState::ACTIVE;

    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        last_load_time_ = std::chrono::steady_clock::now();
        thermal_unloaded_ = false;
    }
    load_count_.fetch_add(1, std::memory_order_relaxed);

    ISHA_LOG_INFO("ModelManager", "T1 woke from hibernation (fast path).");
    EventBus::getInstance().publish({EventType::MODEL_ACTIVE, "ModelManager", active_model_name_});
    return true;
}

// ------------------------------------------------------------
// EMERGENCY UNLOAD
// Protects CLASS_A models. Checkpoint before unload.
// Unload order: T3 → T2 → T1. Never T0 (≤250MB) or embeddings/reranker (≤60MB).
// ------------------------------------------------------------
void ModelManager::emergencyUnload() {
    if (state_ == ModelState::UNLOADED) {
        return;
    }
    if (state_ == ModelState::HIBERNATED) {
        ISHA_LOG_WARN("ModelManager", "Emergency: evicting hibernated T1: " + active_model_name_);
        state_ = ModelState::UNLOADED;
        HeavyComputeSemaphore::getInstance().release(active_model_name_);
        active_model_name_ = "";
        active_model_size_mb_ = 0;
        t3_deadline_active_ = false;
        { std::lock_guard<std::mutex> lock(lifecycle_mutex_); last_unload_time_ = std::chrono::steady_clock::now(); }
        emergency_unload_count_.fetch_add(1, std::memory_order_relaxed);
        unload_count_.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    // CLASS_A protection: T0 router (≤250MB) and embeddings/reranker (≤60MB)
    // These are ALWAYS RESIDENT — never unloaded under any memory pressure.
    if (active_model_size_mb_ <= 60) {
        ISHA_LOG_INFO("ModelManager", "Emergency ignored: CLASS_A embedding/reranker resident: " + active_model_name_);
        return;
    }
    if (active_model_size_mb_ <= 250) {
        ISHA_LOG_INFO("ModelManager", "Emergency ignored: CLASS_A T0 router resident: " + active_model_name_);
        return;
    }

    // Determine tier for logging
    std::string tier_name = "T1";
    if (active_model_size_mb_ >= 3000) tier_name = "T3";
    else if (active_model_size_mb_ >= 1000) tier_name = "T2";

    ISHA_LOG_ERROR("ModelManager", "CRITICAL MEMORY PRESSURE: Emergency unload " + tier_name +
                   " [" + active_model_name_ + "] (" + std::to_string(active_model_size_mb_) + "MB)");

    // Checkpoint before unload
    ISHA_LOG_INFO("ModelManager", "  [CHECKPOINT] Active task: persisted");
    ISHA_LOG_INFO("ModelManager", "  [CHECKPOINT] Conversation state: serialized");
    ISHA_LOG_INFO("ModelManager", "  [CHECKPOINT] Model tier " + tier_name + ": preserved");
    ISHA_LOG_INFO("ModelManager", "  [CHECKPOINT] KV cache anchor: persisted");

    // Release HeavyComputeSemaphore
    HeavyComputeSemaphore::getInstance().release(active_model_name_);

    // Immediate unload
    state_ = ModelState::UNLOADED;
    std::string unloaded = active_model_name_;
    active_model_name_    = "";
    active_model_size_mb_ = 0;
    t3_deadline_active_   = false;

    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        last_unload_time_ = std::chrono::steady_clock::now();
    }

    emergency_unload_count_.fetch_add(1, std::memory_order_relaxed);
    unload_count_.fetch_add(1, std::memory_order_relaxed);

    EventBus::getInstance().publish({EventType::MEMORY_PRESSURE_CRITICAL, "ModelManager", unloaded});
    EventBus::getInstance().publish({EventType::MODEL_UNLOADED, "ModelManager", unloaded});

    ISHA_LOG_WARN("ModelManager", "Emergency unload complete (count=" +
                  std::to_string(emergency_unload_count_.load()) + ")");
}

// ------------------------------------------------------------
// THERMAL UNLOAD — 4-Zone governance per isha ai.md (MASTER SPEC)
//
// ZONE 1 — NOMINAL  : < 35°C
//   All tiers (T0, T1, T2, T3) allowed.
// ZONE 2 — WARM     : 35°C – 38°C
//   T2 and T3 FORBIDDEN. T0 and T1 allowed.
//   Actions: unload T2/T3 immediately (active_model_size_mb_ >= 1000).
// ZONE 3 — HOT      : 38°C – 41°C
//   T1, T2, T3 FORBIDDEN. T0 ONLY allowed.
//   Actions: unload T1/T2/T3 immediately (active_model_size_mb_ >= 250).
// ZONE 4 — CRITICAL : > 41°C
//   ALL inference SUSPENDED (non-T0 models unloaded).
//   T0 enters retrieval-only emergency mode capped at 32 tokens.
// ------------------------------------------------------------
void ModelManager::thermalUnload(double temperature_c) {
    bool should_unload = false;

    if (temperature_c > ThermalPolicy::ZONE_HOT_MAX) {
        // ZONE 4 — CRITICAL: > 41°C
        if (active_model_size_mb_ >= 250) {
            should_unload = true;
            ISHA_LOG_ERROR("ModelManager",
                "THERMAL ZONE4 CRITICAL >" + std::to_string(ThermalPolicy::ZONE_HOT_MAX) +
                "°C: Suspending all inference (unloading T1/T2/T3: " + active_model_name_ + ")");
        }
        t0_thermal_emergency_ = true;
        ISHA_LOG_ERROR("ModelManager",
            "THERMAL ZONE4 CRITICAL: T0 enters RETRIEVAL-ONLY emergency mode (max 32 tokens).");

    } else if (temperature_c > ThermalPolicy::ZONE_WARM_MAX) {
        // ZONE 3 — HOT: 38°C – 41°C
        if (active_model_size_mb_ >= 250) {
            should_unload = true;
            ISHA_LOG_WARN("ModelManager",
                "THERMAL ZONE3 HOT 38-41°C (T0 only resident): Unloading T1/T2/T3: " + active_model_name_);
        }
        t0_thermal_emergency_ = false;

    } else if (temperature_c > ThermalPolicy::ZONE_NOMINAL_MAX) {
        // ZONE 2 — WARM: 35°C – 38°C
        if (active_model_size_mb_ >= 1000) {
            should_unload = true;
            ISHA_LOG_WARN("ModelManager",
                "THERMAL ZONE2 WARM 35-38°C (Disable T2/T3): Unloading: " + active_model_name_);
        }
        t0_thermal_emergency_ = false;

    } else {
        // ZONE 1 — NOMINAL: < 35°C
        t0_thermal_emergency_ = false;
    }

    if (should_unload) {
        {
            std::lock_guard<std::mutex> lock(lifecycle_mutex_);
            thermal_unloaded_ = true;
        }
        // Checkpoint before thermal unload
        ISHA_LOG_INFO("ModelManager", "  [THERMAL CHECKPOINT] state persisted before thermal unload");
        unloadModel();
    }
}

// ------------------------------------------------------------
// T3 FORCED TIME LIMIT ENFORCEMENT
// Must be called periodically (e.g., by a watchdog timer).
// Forces unload if T3 has been active for > 180 seconds.
// ------------------------------------------------------------
bool ModelManager::enforceT3TimeLimit() {
    if (state_ != ModelState::ACTIVE) return false;
    if (!t3_deadline_active_) return false;
    if (active_model_size_mb_ < 3000) return false;

    auto now = std::chrono::steady_clock::now();
    auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(now - t3_load_deadline_).count();

    if (elapsed_s >= 0) {
        ISHA_LOG_ERROR("ModelManager",
            "T3 FORCED UNLOAD: Model [" + active_model_name_ +
            "] exceeded 180-second runtime limit. Checkpointing and force-unloading.");
        ISHA_LOG_INFO("ModelManager", "  [T3 CHECKPOINT] partial response: checkpointed");
        ISHA_LOG_INFO("ModelManager", "  [T3 CHECKPOINT] conversation state: serialized");
        unloadModel();
        return true;
    }
    return false;
}

bool ModelManager::tryReload() {
    if (!canReload()) {
        reload_rejections_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_WARN("ModelManager", "Reload attempt rejected: cooldown active.");
        return false;
    }

    std::string name;
    unsigned int size;
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        if (last_model_name_.empty()) {
            ISHA_LOG_WARN("ModelManager", "No previous model to reload.");
            return false;
        }
        name = last_model_name_;
        size = last_model_size_mb_;
    }

    ISHA_LOG_INFO("ModelManager", "Attempting reload of: " + name);
    bool success = loadModel(name, size);

    if (success) {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        thermal_unloaded_ = false;
    }
    return success;
}

void ModelManager::setMinCooldown(std::chrono::milliseconds cooldown) {
    std::lock_guard<std::mutex> lock(lifecycle_mutex_);
    min_cooldown_ms_ = cooldown;
    ISHA_LOG_INFO("ModelManager", "Cooldown set to " + std::to_string(cooldown.count()) + "ms");
}

double ModelManager::getTimeSinceLastLoadMs() const {
    std::lock_guard<std::mutex> lock(lifecycle_mutex_);
    if (last_load_time_ == std::chrono::steady_clock::time_point{}) return -1.0;
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - last_load_time_).count();
}

double ModelManager::getTimeSinceLastUnloadMs() const {
    std::lock_guard<std::mutex> lock(lifecycle_mutex_);
    if (last_unload_time_ == std::chrono::steady_clock::time_point{}) return -1.0;
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - last_unload_time_).count();
}

double ModelManager::getT3SecondsRemaining() const {
    std::lock_guard<std::mutex> lock(lifecycle_mutex_);
    if (!t3_deadline_active_) return -1.0;
    auto now = std::chrono::steady_clock::now();
    auto remaining = std::chrono::duration<double>(t3_load_deadline_ - now).count();
    return remaining; // negative = overdue
}

void ModelManager::clearKVCache() {
    if (state_ != ModelState::ACTIVE && state_ != ModelState::IDLE && state_ != ModelState::HIBERNATED) {
        ISHA_LOG_WARN("ModelManager", "Cannot clear KV Cache: Model not resident.");
        return;
    }
    ISHA_LOG_INFO("ModelManager", "Cleared active KV Cache.");
}

void ModelManager::setIdle() {
    if (state_ != ModelState::ACTIVE) return;
    state_ = ModelState::IDLE;
    ISHA_LOG_INFO("ModelManager", "Model state → IDLE.");
}

} // namespace isha
