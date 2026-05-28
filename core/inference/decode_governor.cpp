#include "decode_governor.hpp"
#include "../logging/logger.hpp"
#include <thread>

namespace isha {

DecodeGovernor::DecodeGovernor()
    : session_start_(std::chrono::steady_clock::now()),
      last_token_time_(std::chrono::steady_clock::now()),
      last_temp_read_time_(std::chrono::steady_clock::now()) {}

void DecodeGovernor::paceToken(double temperature_c) {
    std::lock_guard<std::mutex> lock(mutex_);

    uint32_t current_count = tokens_generated_.fetch_add(1, std::memory_order_relaxed);

    // Update predictive thermal slope
    auto now = std::chrono::steady_clock::now();
    double time_diff = std::chrono::duration<double>(now - last_temp_read_time_).count();
    if (time_diff > 0.05) { // Minimum 50ms interval for stable gradient
        if (last_temperature_ > 0.0) {
            double temp_diff = temperature_c - last_temperature_;
            last_temp_slope_ = temp_diff / time_diff; // °C per second
        }
        last_temperature_ = temperature_c;
        last_temp_read_time_ = now;
    }

    // Burst mode: first N tokens unthrottled for fast first-response
    if (current_count < burst_tokens_) {
        last_token_time_ = std::chrono::steady_clock::now();
        return;
    }

    // Determine target thermal tier with predictive factor.
    // If slope is strongly rising (>0.3°C/sec), proactively bump the effective temperature.
    double effective_temp = temperature_c;
    if (last_temp_slope_ > 0.3) {
        double prediction_boost = last_temp_slope_ * 4.0; // Predict temperature 4 seconds in the future
        effective_temp += prediction_boost;
        ISHA_LOG_DEBUG("DecodeGovernor", "Predictive boost: Actual=" + std::to_string(temperature_c) + 
                       "°C, Slope=" + std::to_string(last_temp_slope_) + 
                       "°C/s, Effective=" + std::to_string(effective_temp) + "°C");
    }

    // Determine TPS limit based on predictive/effective thermal tier
    float tps_limit = 0.0f;
    if (effective_temp > 41.0) {
        tps_limit = tps_critical_;
    } else if (effective_temp > 38.0) {
        tps_limit = tps_hot_;
    } else if (effective_temp > 35.0) {
        tps_limit = tps_warm_;
    } else {
        tps_limit = tps_cool_;
    }

    // If tps_limit is 0 (unlimited), no pacing needed
    if (tps_limit <= 0.0f) {
        last_token_time_ = std::chrono::steady_clock::now();
        return;
    }

    // Compute target delay between tokens
    auto target_delay = std::chrono::duration<double>(1.0 / static_cast<double>(tps_limit));
    now = std::chrono::steady_clock::now();
    auto elapsed = now - last_token_time_;

    if (elapsed < target_delay) {
        auto sleep_duration = target_delay - elapsed;
        auto sleep_ms = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(sleep_duration);

        std::this_thread::sleep_for(sleep_duration);

        pause_count_.fetch_add(1, std::memory_order_relaxed);
        float current_pause = total_pause_ms_.load(std::memory_order_relaxed);
        total_pause_ms_.store(current_pause + sleep_ms.count(), std::memory_order_relaxed);

        ISHA_LOG_DEBUG("DecodeGovernor", "Paced token decode: slept " 
                       + std::to_string(sleep_ms.count()) + "ms at temperature " 
                       + std::to_string(temperature_c) + "°C (effective: " 
                       + std::to_string(effective_temp) + "°C)");
    }

    last_token_time_ = std::chrono::steady_clock::now();
}

void DecodeGovernor::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    tokens_generated_.store(0, std::memory_order_relaxed);
    pause_count_.store(0, std::memory_order_relaxed);
    total_pause_ms_.store(0.0f, std::memory_order_relaxed);
    session_start_ = std::chrono::steady_clock::now();
    last_token_time_ = std::chrono::steady_clock::now();
    last_temp_read_time_ = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    last_temperature_ = 0.0;
    last_temp_slope_ = 0.0;

    ISHA_LOG_DEBUG("DecodeGovernor", "Pacing state reset for new generation session.");
}

float DecodeGovernor::currentTokensPerSecond() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto elapsed = std::chrono::steady_clock::now() - session_start_;
    double elapsed_seconds = std::chrono::duration<double>(elapsed).count();
    if (elapsed_seconds <= 0.0) {
        return 0.0f;
    }
    return static_cast<float>(tokens_generated_.load(std::memory_order_relaxed)) 
           / static_cast<float>(elapsed_seconds);
}

uint32_t DecodeGovernor::totalTokensGenerated() const {
    return tokens_generated_.load(std::memory_order_relaxed);
}

uint32_t DecodeGovernor::totalPauseCount() const {
    return pause_count_.load(std::memory_order_relaxed);
}

float DecodeGovernor::totalPauseTimeMs() const {
    return total_pause_ms_.load(std::memory_order_relaxed);
}

void DecodeGovernor::setBurstTokens(uint32_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    burst_tokens_ = count;
    ISHA_LOG_INFO("DecodeGovernor", "Burst tokens set to " + std::to_string(count));
}

void DecodeGovernor::setMaxTokensPerSecond(float tps_cool, float tps_warm, 
                                            float tps_hot, float tps_critical) {
    std::lock_guard<std::mutex> lock(mutex_);
    tps_cool_ = tps_cool;
    tps_warm_ = tps_warm;
    tps_hot_ = tps_hot;
    tps_critical_ = tps_critical;
    ISHA_LOG_INFO("DecodeGovernor", "TPS limits updated: cool=" + std::to_string(tps_cool) 
                  + " warm=" + std::to_string(tps_warm) + " hot=" + std::to_string(tps_hot) 
                  + " critical=" + std::to_string(tps_critical));
}

} // namespace isha
