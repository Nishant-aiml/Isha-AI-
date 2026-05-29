#ifndef ISHA_AI_MODEL_MANAGER_HPP
#define ISHA_AI_MODEL_MANAGER_HPP

#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include "../config/device_profile.hpp"
#include "../config/resource_limits.hpp"

namespace isha {

enum class ModelState {
    UNLOADED,
    LOADING,
    ACTIVE,
    IDLE,         // T1 warm — tokenizer alive, KV released, threads=0
    HIBERNATED,   // T1 — mmap preserved if possible; tokenizer alive; KV released
    UNLOADING,
    FAILED
};

inline std::string modelStateToString(ModelState state) {
    switch (state) {
        case ModelState::UNLOADED:   return "UNLOADED";
        case ModelState::LOADING:    return "LOADING";
        case ModelState::ACTIVE:     return "ACTIVE";
        case ModelState::IDLE:       return "IDLE";
        case ModelState::HIBERNATED: return "HIBERNATED";
        case ModelState::UNLOADING:  return "UNLOADING";
        case ModelState::FAILED:     return "FAILED";
        default: return "UNKNOWN";
    }
}

// ============================================================
// HEAVY COMPUTE SEMAPHORE
// Ensures only ONE CLASS_D model (T2 or T3) is active at a time.
// Prevents: T2 + Whisper simultaneously, T3 + TTS simultaneously, etc.
// ============================================================
class HeavyComputeSemaphore {
public:
    static HeavyComputeSemaphore& getInstance() {
        static HeavyComputeSemaphore instance;
        return instance;
    }

    // Try to acquire the heavy-compute slot. Returns false if already held.
    bool tryAcquire(const std::string& model_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (held_) return false;
        held_ = true;
        holder_ = model_name;
        return true;
    }

    // Release the slot. Safe to call even if not held.
    void release(const std::string& model_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (holder_ == model_name) {
            held_ = false;
            holder_.clear();
        }
    }

    bool isHeld() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return held_;
    }

    std::string currentHolder() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return holder_;
    }

private:
    HeavyComputeSemaphore() = default;
    mutable std::mutex mutex_;
    bool held_ = false;
    std::string holder_;
};

class ModelManager {
public:
    ModelManager(const DeviceProfile& profile);
    ~ModelManager();

    // Core lifecycle
    bool loadModel(const std::string& model_name, unsigned int size_mb);
    void unloadModel();
    void clearKVCache();
    void setIdle();

    // T1 hibernation: releases KV cache, sets threads=0, preserves mmap + tokenizer
    // Call after T1HibernationPolicy::IDLE_TIMEOUT_SECONDS of inactivity.
    void hibernateT1();

    // Reload from hibernated state without full cold-start
    bool wakeFromHibernation();

    // Lifecycle governance
    bool canReload() const;

    // Emergency unload — immediate, ignores cooldown.
    // Protects CLASS_A models (T0 ≤ 250MB, embeddings ≤ 60MB, reranker ≤ 60MB).
    void emergencyUnload();

    // Thermal-aware unload — 5-zone policy per ThermalPolicy constants.
    // Zone 5 (>46°C): T0 does NOT continue normal inference — enters retrieval-only mode.
    void thermalUnload(double temperature_c);

    // T0 thermal emergency mode: called when temperature > ZONE4_SUSPEND_ALL (46°C)
    // Switches T0 to retrieval-only, ultra-low-freq, tiny token budget (max 32 tokens).
    bool isT0ThermalEmergencyMode() const { return t0_thermal_emergency_; }

    // T3 forced unload: must be called after 180s of T3 active time.
    // Returns true if T3 was active and was force-unloaded.
    bool enforceT3TimeLimit();

    // Attempt reload after cooldown
    bool tryReload();

    // Configure cooldown (default 2000ms)
    void setMinCooldown(std::chrono::milliseconds cooldown);

    // Residency class of currently loaded model
    void setResidencyClass(ResidencyClass rc) { active_residency_class_ = rc; }
    ResidencyClass getResidencyClass() const { return active_residency_class_; }

    // State queries
    ModelState getState() const { return state_; }
    std::string getActiveModelName() const { return active_model_name_; }
    unsigned int getActiveModelSizeMB() const { return active_model_size_mb_; }
    double getLoadLatencyMs() const { return load_latency_ms_; }
    double getUnloadLatencyMs() const { return unload_latency_ms_; }
    bool isThermalUnloaded() const { return thermal_unloaded_; }
    bool isHibernated() const { return state_ == ModelState::HIBERNATED; }

    // Timing
    double getTimeSinceLastLoadMs() const;
    double getTimeSinceLastUnloadMs() const;

    // T3 deadline: returns seconds remaining before forced unload (< 0 = overdue)
    double getT3SecondsRemaining() const;

    // Stats
    unsigned int getLoadCount() const { return load_count_.load(std::memory_order_relaxed); }
    unsigned int getUnloadCount() const { return unload_count_.load(std::memory_order_relaxed); }
    unsigned int getEmergencyUnloadCount() const { return emergency_unload_count_.load(std::memory_order_relaxed); }
    unsigned int getReloadRejections() const { return reload_rejections_.load(std::memory_order_relaxed); }
    unsigned int getHibernationCount() const { return hibernation_count_.load(std::memory_order_relaxed); }

private:
    DeviceProfile profile_;
    ModelState state_;
    std::string active_model_name_;
    unsigned int active_model_size_mb_;
    ResidencyClass active_residency_class_ = ResidencyClass::CLASS_B_WARM_HIBERNATED;

    double load_latency_ms_;
    double unload_latency_ms_;

    // Lifecycle governance
    std::chrono::steady_clock::time_point last_load_time_;
    std::chrono::steady_clock::time_point last_unload_time_;
    std::chrono::milliseconds min_cooldown_ms_{2000};
    std::atomic<unsigned int> load_count_{0};
    std::atomic<unsigned int> unload_count_{0};
    std::atomic<unsigned int> emergency_unload_count_{0};
    std::atomic<unsigned int> reload_rejections_{0};
    std::atomic<unsigned int> hibernation_count_{0};
    bool thermal_unloaded_ = false;
    std::string last_model_name_;
    unsigned int last_model_size_mb_ = 0;
    mutable std::mutex lifecycle_mutex_;

    // T3 forced unload deadline
    std::chrono::steady_clock::time_point t3_load_deadline_;
    bool t3_deadline_active_ = false;

    // T0 thermal emergency mode flag
    bool t0_thermal_emergency_ = false;
};

} // namespace isha

#endif // ISHA_AI_MODEL_MANAGER_HPP
