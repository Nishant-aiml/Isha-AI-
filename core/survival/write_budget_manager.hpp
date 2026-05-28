#ifndef ISHA_AI_WRITE_BUDGET_MANAGER_HPP
#define ISHA_AI_WRITE_BUDGET_MANAGER_HPP

#include <string>
#include <mutex>

namespace isha {

class WriteBudgetManager {
public:
    WriteBudgetManager() : accumulated_writes_bytes_(0), hourly_limit_bytes_(10 * 1024 * 1024) {} // default 10MB/hour for telemetry/metadata
    ~WriteBudgetManager() = default;

    static WriteBudgetManager& getInstance();

    // Configure daily write bounds
    void setHourlyLimit(size_t limit_bytes);

    // Track an expected write event
    bool requestWritePermission(size_t size_bytes);
    void recordActualWrite(size_t size_bytes);

    // Dynamic throttle mapping based on triggers
    void adaptBudget(bool is_emmc, bool thermal_stress, bool low_battery, bool background);

    // Queries
    size_t getAccumulatedWrites() const { return accumulated_writes_bytes_; }
    size_t getHourlyLimit() const { return hourly_limit_bytes_; }
    void resetAccumulator();

private:
    size_t accumulated_writes_bytes_;
    size_t hourly_limit_bytes_;
    mutable std::mutex budget_mutex_;
};

} // namespace isha

#endif // ISHA_AI_WRITE_BUDGET_MANAGER_HPP
