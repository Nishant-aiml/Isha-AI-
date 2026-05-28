#ifndef ISHA_AI_MODEL_MANAGER_HPP
#define ISHA_AI_MODEL_MANAGER_HPP

#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include "../config/device_profile.hpp"

namespace isha {

enum class ModelState {
    UNLOADED,
    LOADING,
    ACTIVE,
    IDLE,
    UNLOADING,
    FAILED
};

inline std::string modelStateToString(ModelState state) {
    switch (state) {
        case ModelState::UNLOADED: return "UNLOADED";
        case ModelState::LOADING: return "LOADING";
        case ModelState::ACTIVE: return "ACTIVE";
        case ModelState::IDLE: return "IDLE";
        case ModelState::UNLOADING: return "UNLOADING";
        case ModelState::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

class ModelManager {
public:
    ModelManager(const DeviceProfile& profile);
    ~ModelManager();

    // Core lifecycle
    bool loadModel(const std::string& model_name, unsigned int size_mb);
    void unloadModel();
    void clearKVCache();
    void setIdle();

    // Lifecycle governance — reload storm prevention
    bool canReload() const;

    // Emergency unload — immediate, ignores cooldown
    void emergencyUnload();

    // Thermal-aware unload — respects doc thresholds:
    //   HOT (>38C): unload T2/T3 (>1000MB)
    //   CRITICAL (>41C): unload everything except T0 (<250MB)
    void thermalUnload(double temperature_c);

    // Attempt reload after cooldown
    bool tryReload();

    // Configure cooldown (default 2000ms)
    void setMinCooldown(std::chrono::milliseconds cooldown);

    // State queries
    ModelState getState() const { return state_; }
    std::string getActiveModelName() const { return active_model_name_; }
    unsigned int getActiveModelSizeMB() const { return active_model_size_mb_; }
    double getLoadLatencyMs() const { return load_latency_ms_; }
    double getUnloadLatencyMs() const { return unload_latency_ms_; }
    bool isThermalUnloaded() const { return thermal_unloaded_; }

    // Timing
    double getTimeSinceLastLoadMs() const;
    double getTimeSinceLastUnloadMs() const;

    // Stats
    unsigned int getLoadCount() const { return load_count_.load(std::memory_order_relaxed); }
    unsigned int getUnloadCount() const { return unload_count_.load(std::memory_order_relaxed); }
    unsigned int getEmergencyUnloadCount() const { return emergency_unload_count_.load(std::memory_order_relaxed); }
    unsigned int getReloadRejections() const { return reload_rejections_.load(std::memory_order_relaxed); }

private:
    DeviceProfile profile_;
    ModelState state_;
    std::string active_model_name_;
    unsigned int active_model_size_mb_;

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
    bool thermal_unloaded_ = false;
    std::string last_model_name_;
    unsigned int last_model_size_mb_ = 0;
    mutable std::mutex lifecycle_mutex_;
};

} // namespace isha

#endif // ISHA_AI_MODEL_MANAGER_HPP
