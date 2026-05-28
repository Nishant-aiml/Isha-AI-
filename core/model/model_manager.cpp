#include "model_manager.hpp"
#include "../runtime/event_bus.hpp"
#include "../logging/logger.hpp"
#include <thread>

namespace isha {

ModelManager::ModelManager(const DeviceProfile& profile)
    : profile_(profile), state_(ModelState::UNLOADED), active_model_name_(""), active_model_size_mb_(0),
      load_latency_ms_(0.0), unload_latency_ms_(0.0),
      last_model_name_(""), last_model_size_mb_(0) {

    // Initialize timing points to epoch so canReload() returns true initially
    last_load_time_ = std::chrono::steady_clock::time_point{};
    last_unload_time_ = std::chrono::steady_clock::time_point{};

    // Subscribe to lifecycle and memory warnings for automated cleanups
    EventBus::getInstance().subscribe(EventType::LIFECYCLE_HIBERNATE, "ModelManager", [this](const Event&) {
        ISHA_LOG_INFO("ModelManager", "Received hibernate event. Auto-unloading active model.");
        unloadModel();
    });

    EventBus::getInstance().subscribe(EventType::MEMORY_PRESSURE_CRITICAL, "ModelManager", [this](const Event&) {
        ISHA_LOG_WARN("ModelManager", "Received critical memory warning. Emergency unloading.");
        emergencyUnload();
    });
}

ModelManager::~ModelManager() {
    EventBus::getInstance().unsubscribe(EventType::LIFECYCLE_HIBERNATE, "ModelManager");
    EventBus::getInstance().unsubscribe(EventType::MEMORY_PRESSURE_CRITICAL, "ModelManager");
    if (state_ != ModelState::UNLOADED) {
        unloadModel();
    }
}

bool ModelManager::canReload() const {
    std::lock_guard<std::mutex> lock(lifecycle_mutex_);
    auto now = std::chrono::steady_clock::now();
    auto since_unload = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_unload_time_);

    // If never unloaded (epoch), allow reload
    if (last_unload_time_ == std::chrono::steady_clock::time_point{}) return true;

    return since_unload >= min_cooldown_ms_;
}

bool ModelManager::loadModel(const std::string& model_name, unsigned int size_mb) {
    // Cooldown check — prevent reload storms
    if (!canReload()) {
        reload_rejections_.fetch_add(1, std::memory_order_relaxed);
        auto since = getTimeSinceLastUnloadMs();
        ISHA_LOG_WARN("ModelManager", "Reload rejected: cooldown active (" +
                      std::to_string(since) + "ms / " +
                      std::to_string(min_cooldown_ms_.count()) + "ms required). " +
                      "rejections=" + std::to_string(reload_rejections_.load()));
        return false;
    }

    if (state_ != ModelState::UNLOADED) {
        ISHA_LOG_WARN("ModelManager", "Cannot load model. Current state is: " + modelStateToString(state_) + ". Unload first.");
        return false;
    }

    state_ = ModelState::LOADING;
    EventBus::getInstance().publish({EventType::MODEL_LOADING, "ModelManager", model_name});

    auto start_time = std::chrono::high_resolution_clock::now();
    ISHA_LOG_INFO("ModelManager", "Loading model: " + model_name + " (" + std::to_string(size_mb) + "MB)");

    // Simulate loading work (will be replaced with real llama.cpp load)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto end_time = std::chrono::high_resolution_clock::now();
    load_latency_ms_ = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    state_ = ModelState::ACTIVE;
    active_model_name_ = model_name;
    active_model_size_mb_ = size_mb;

    // Record for lifecycle governance
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        last_load_time_ = std::chrono::steady_clock::now();
        last_model_name_ = model_name;
        last_model_size_mb_ = size_mb;
        thermal_unloaded_ = false;
    }
    load_count_.fetch_add(1, std::memory_order_relaxed);

    ISHA_LOG_INFO("ModelManager", "Successfully loaded model: " + model_name +
                  " in " + std::to_string(load_latency_ms_) + "ms" +
                  " (load_count=" + std::to_string(load_count_.load()) + ")");
    EventBus::getInstance().publish({EventType::MODEL_ACTIVE, "ModelManager", model_name});
    return true;
}

void ModelManager::unloadModel() {
    if (state_ == ModelState::UNLOADED) return;

    state_ = ModelState::UNLOADING;
    EventBus::getInstance().publish({EventType::MODEL_UNLOADING, "ModelManager", active_model_name_});

    auto start_time = std::chrono::high_resolution_clock::now();
    ISHA_LOG_INFO("ModelManager", "Unloading model: " + active_model_name_);

    // Simulate unloading work
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto end_time = std::chrono::high_resolution_clock::now();
    unload_latency_ms_ = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    state_ = ModelState::UNLOADED;
    std::string unloaded_model = active_model_name_;
    active_model_name_ = "";
    active_model_size_mb_ = 0;

    // Record for lifecycle governance
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        last_unload_time_ = std::chrono::steady_clock::now();
    }
    unload_count_.fetch_add(1, std::memory_order_relaxed);

    ISHA_LOG_INFO("ModelManager", "Successfully unloaded model in " + std::to_string(unload_latency_ms_) + "ms" +
                  " (unload_count=" + std::to_string(unload_count_.load()) + ")");
    EventBus::getInstance().publish({EventType::MODEL_UNLOADED, "ModelManager", unloaded_model});
}

void ModelManager::emergencyUnload() {
    if (state_ == ModelState::UNLOADED) return;

    ISHA_LOG_WARN("ModelManager", "EMERGENCY UNLOAD: " + active_model_name_ +
                  " (" + std::to_string(active_model_size_mb_) + "MB)");

    // Bypass normal unload simulation — immediate
    state_ = ModelState::UNLOADED;
    std::string unloaded_model = active_model_name_;
    active_model_name_ = "";
    active_model_size_mb_ = 0;

    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        last_unload_time_ = std::chrono::steady_clock::now();
    }

    emergency_unload_count_.fetch_add(1, std::memory_order_relaxed);
    unload_count_.fetch_add(1, std::memory_order_relaxed);

    EventBus::getInstance().publish({EventType::MEMORY_PRESSURE_CRITICAL, "ModelManager", unloaded_model});
    EventBus::getInstance().publish({EventType::MODEL_UNLOADED, "ModelManager", unloaded_model});

    ISHA_LOG_WARN("ModelManager", "Emergency unload complete (emergency_count=" +
                  std::to_string(emergency_unload_count_.load()) + ")");
}

void ModelManager::thermalUnload(double temperature_c) {
    if (state_ == ModelState::UNLOADED) return;

    // Doc thresholds: HOT >38C, CRITICAL >41C
    bool should_unload = false;

    if (temperature_c > 41.0) {
        // CRITICAL: unload everything except T0-sized models (<250MB)
        if (active_model_size_mb_ >= 250) {
            should_unload = true;
            ISHA_LOG_WARN("ModelManager", "THERMAL CRITICAL (" + std::to_string(temperature_c) +
                          "C): Unloading " + active_model_name_ + " (" +
                          std::to_string(active_model_size_mb_) + "MB)");
        }
    } else if (temperature_c > 38.0) {
        // HOT: unload T2/T3 models (>1000MB)
        if (active_model_size_mb_ > 1000) {
            should_unload = true;
            ISHA_LOG_WARN("ModelManager", "THERMAL HOT (" + std::to_string(temperature_c) +
                          "C): Unloading large model " + active_model_name_ + " (" +
                          std::to_string(active_model_size_mb_) + "MB)");
        }
    }

    if (should_unload) {
        {
            std::lock_guard<std::mutex> lock(lifecycle_mutex_);
            thermal_unloaded_ = true;
        }
        unloadModel();
    }
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
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(now - last_load_time_).count();
}

double ModelManager::getTimeSinceLastUnloadMs() const {
    std::lock_guard<std::mutex> lock(lifecycle_mutex_);
    if (last_unload_time_ == std::chrono::steady_clock::time_point{}) return -1.0;
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(now - last_unload_time_).count();
}

void ModelManager::clearKVCache() {
    if (state_ != ModelState::ACTIVE && state_ != ModelState::IDLE) {
        ISHA_LOG_WARN("ModelManager", "Cannot clear KV Cache: Model is not resident.");
        return;
    }
    ISHA_LOG_INFO("ModelManager", "Cleared active KV Cache.");
}

void ModelManager::setIdle() {
    if (state_ != ModelState::ACTIVE) return;
    state_ = ModelState::IDLE;
    ISHA_LOG_INFO("ModelManager", "Model state transitioned to IDLE.");
}

} // namespace isha
