#include "write_budget_manager.hpp"

namespace isha {

WriteBudgetManager& WriteBudgetManager::getInstance() {
    static WriteBudgetManager instance;
    return instance;
}

void WriteBudgetManager::setHourlyLimit(size_t limit_bytes) {
    std::lock_guard<std::mutex> lock(budget_mutex_);
    hourly_limit_bytes_ = limit_bytes;
}

bool WriteBudgetManager::requestWritePermission(size_t size_bytes) {
    std::lock_guard<std::mutex> lock(budget_mutex_);
    // Block write if it pushes us over the safety budget
    if (accumulated_writes_bytes_ + size_bytes > hourly_limit_bytes_) {
        return false;
    }
    return true;
}

void WriteBudgetManager::recordActualWrite(size_t size_bytes) {
    std::lock_guard<std::mutex> lock(budget_mutex_);
    accumulated_writes_bytes_ += size_bytes;
}

void WriteBudgetManager::adaptBudget(bool is_emmc, bool thermal_stress, bool low_battery, bool background) {
    std::lock_guard<std::mutex> lock(budget_mutex_);
    
    // Scale budget aggressively based on wear risk
    size_t base_limit = 10 * 1024 * 1024; // 10MB/hour
    
    if (is_emmc) {
        base_limit /= 5; // eMMC slow disk wear safety (2MB/hour)
    }
    if (thermal_stress) {
        base_limit /= 2; // Reduce disk IO heat
    }
    if (low_battery) {
        base_limit /= 2; // Save power by avoiding NAND write operations
    }
    if (background) {
        base_limit /= 10; // Suspend telemetry/checkpoints to minimum in background
    }

    // Ensure we have at least a floor budget of 100KB per hour
    if (base_limit < 100 * 1024) {
        base_limit = 100 * 1024;
    }
    
    hourly_limit_bytes_ = base_limit;
}

void WriteBudgetManager::resetAccumulator() {
    std::lock_guard<std::mutex> lock(budget_mutex_);
    accumulated_writes_bytes_ = 0;
}

} // namespace isha
