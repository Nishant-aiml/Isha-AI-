#ifndef ISHA_AI_ACCELERATION_QUARANTINE_HPP
#define ISHA_AI_ACCELERATION_QUARANTINE_HPP

#include <string>
#include <chrono>
#include <map>
#include <mutex>
#include "acceleration_backend.hpp"

namespace isha {

enum class FailureSeverity {
    MINOR_TIMEOUT,
    THERMAL_TRIGGER,
    QUEUE_DEADLOCK,
    SHADER_CRASH,
    INIT_CORRUPTION,
    
    // NNAPI Structured Taxonomy (Phase 8A)
    UNSUPPORTED_OPERATOR,
    GRAPH_COMPILATION_FAILURE,
    TENSOR_SHAPE_MISMATCH,
    BACKEND_TIMEOUT,
    DRIVER_CRASH,
    ALLOCATION_FAILURE,
    THERMAL_TRIGGER_FALLBACK,
    WATCHDOG_TRIGGER_FALLBACK,
    SYNCHRONIZATION_FAILURE,
    STAGING_ALLOCATION_FAILURE,
    BACKEND_RESET_FAILURE,

    // Phase 8B: Android Device Chaos Taxonomy
    LIFECYCLE_INTERRUPT_CRASH,   // Backend crash on app background/foreground
    LMK_EVICTION_CORRUPTION,     // OOM eviction corrupted backend state
    VENDOR_DRIVER_INSTABILITY,   // OEM-specific repeated driver failures
    TOK_PER_WATT_DEGRADATION,    // Efficiency below CPU baseline
    PARTITION_OVERHEAD_EXCESS,   // Partition overhead exceeds adaptive threshold
    SOAK_THERMAL_COLLAPSE,       // Thermal collapse detected during long soak
    STARTUP_LATENCY_EXCESS       // Cold-start exceeds 500ms ceiling
};

struct QuarantineRecord {
    std::string fingerprint;
    int failure_count = 0;
    std::chrono::system_clock::time_point last_failure_time;
    std::chrono::seconds quarantine_duration{0};
    float reputation_score = 1.0f; // 1.0 = perfect trust, 0.0 = quarantined
    bool is_quarantined = false;
};

class AccelerationQuarantine {
public:
    static AccelerationQuarantine& getInstance() {
        static AccelerationQuarantine instance;
        return instance;
    }

    void reportFailure(const std::string& fingerprint, FailureSeverity severity, bool is_debug = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& record = records_[fingerprint];
        record.fingerprint = fingerprint;
        record.failure_count++;
        record.last_failure_time = std::chrono::system_clock::now();

        std::chrono::seconds duration{0};
        float penalty = 0.0f;
        switch (severity) {
            case FailureSeverity::MINOR_TIMEOUT:
            case FailureSeverity::BACKEND_TIMEOUT:
                duration = std::chrono::seconds(60);
                penalty = 0.1f;
                break;
            case FailureSeverity::THERMAL_TRIGGER:
            case FailureSeverity::THERMAL_TRIGGER_FALLBACK:
                duration = std::chrono::seconds(300);
                penalty = 0.2f;
                break;
            case FailureSeverity::QUEUE_DEADLOCK:
            case FailureSeverity::SYNCHRONIZATION_FAILURE:
                duration = std::chrono::seconds(3600);
                penalty = 0.4f;
                break;
            case FailureSeverity::SHADER_CRASH:
            case FailureSeverity::GRAPH_COMPILATION_FAILURE:
            case FailureSeverity::UNSUPPORTED_OPERATOR:
            case FailureSeverity::TENSOR_SHAPE_MISMATCH:
                duration = std::chrono::seconds(43200);
                penalty = 0.6f;
                break;
            case FailureSeverity::INIT_CORRUPTION:
            case FailureSeverity::DRIVER_CRASH:
            case FailureSeverity::ALLOCATION_FAILURE:
            case FailureSeverity::WATCHDOG_TRIGGER_FALLBACK:
            case FailureSeverity::STAGING_ALLOCATION_FAILURE:
            case FailureSeverity::BACKEND_RESET_FAILURE:
            case FailureSeverity::LMK_EVICTION_CORRUPTION:
            case FailureSeverity::VENDOR_DRIVER_INSTABILITY:
                duration = std::chrono::seconds(86400);
                penalty = 0.9f;
                break;
            // Phase 8B specific cases
            case FailureSeverity::LIFECYCLE_INTERRUPT_CRASH:
                duration = std::chrono::seconds(3600);
                penalty = 0.4f;
                break;
            case FailureSeverity::TOK_PER_WATT_DEGRADATION:
                duration = std::chrono::seconds(1800);
                penalty = 0.3f;
                break;
            case FailureSeverity::PARTITION_OVERHEAD_EXCESS:
                duration = std::chrono::seconds(300);
                penalty = 0.2f;
                break;
            case FailureSeverity::SOAK_THERMAL_COLLAPSE:
                duration = std::chrono::seconds(7200);
                penalty = 0.5f;
                break;
            case FailureSeverity::STARTUP_LATENCY_EXCESS:
                duration = std::chrono::seconds(600);
                penalty = 0.25f;
                break;
        }

        record.reputation_score = std::max(0.0f, record.reputation_score - penalty);
        
        // Cooldown and quarantine check
        if (record.failure_count >= 3 && record.failure_count < 5) {
            record.quarantine_duration = std::max(record.quarantine_duration, std::chrono::seconds(60));
        } else if (record.failure_count >= 5 || record.reputation_score <= 0.2f) {
            record.quarantine_duration = std::max(record.quarantine_duration, std::chrono::seconds(86400));
            if (!is_debug) {
                record.is_quarantined = true;
            }
        }
    }

    void reportSuccess(const std::string& fingerprint) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = records_.find(fingerprint);
        if (it != records_.end()) {
            // Decay penalty / recover trust slowly
            it->second.reputation_score = std::min(1.0f, it->second.reputation_score + 0.05f);
            if (it->second.reputation_score > 0.5f) {
                it->second.is_quarantined = false;
            }
            if (it->second.failure_count > 0) {
                it->second.failure_count--;
            }
        }
    }

    bool isQuarantined(const std::string& fingerprint) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = records_.find(fingerprint);
        if (it == records_.end()) return false;
        
        if (it->second.is_quarantined) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - it->second.last_failure_time);
            if (elapsed >= it->second.quarantine_duration) {
                // Decay quarantine status on expiration
                it->second.is_quarantined = false;
                it->second.reputation_score = 0.5f;
                return false;
            }
            return true;
        }
        return false;
    }

    float getReputation(const std::string& fingerprint) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = records_.find(fingerprint);
        if (it == records_.end()) return 1.0f;
        return it->second.reputation_score;
    }

    void resetReputation(const std::string& fingerprint) {
        std::lock_guard<std::mutex> lock(mutex_);
        records_.erase(fingerprint);
    }

private:
    AccelerationQuarantine() = default;
    std::map<std::string, QuarantineRecord> records_;
    std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_ACCELERATION_QUARANTINE_HPP
