#ifndef ISHA_AI_DECODE_GOVERNOR_HPP
#define ISHA_AI_DECODE_GOVERNOR_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>

namespace isha {

class DecodeGovernor {
public:
    DecodeGovernor();

    // Call before each token decode. Sleeps if pacing required.
    void paceToken(double temperature_c);

    // Reset pacing state (new generation session)
    void reset();

    // Stats
    float currentTokensPerSecond() const;
    uint32_t totalTokensGenerated() const;
    uint32_t totalPauseCount() const;
    float totalPauseTimeMs() const;
    double lastTemperatureSlope() const { return last_temp_slope_; }

    // Configuration
    void setBurstTokens(uint32_t count);
    void setMaxTokensPerSecond(float tps_cool, float tps_warm, float tps_hot, float tps_critical);

private:
    // Burst mode: first N tokens unthrottled for fast first-response
    uint32_t burst_tokens_ = 10;
    std::atomic<uint32_t> tokens_generated_{0};
    std::atomic<uint32_t> pause_count_{0};
    std::atomic<float> total_pause_ms_{0.0f};

    // Thermal TPS limits
    float tps_cool_ = 0.0f;      // <35°C: unlimited (0 = no limit)
    float tps_warm_ = 15.0f;     // 35-38°C
    float tps_hot_ = 10.0f;      // 38-41°C
    float tps_critical_ = 5.0f;  // >41°C

    // Predictive thermal tracking
    double last_temperature_ = 0.0;
    std::chrono::steady_clock::time_point last_temp_read_time_;
    double last_temp_slope_ = 0.0; // °C per second change

    std::chrono::steady_clock::time_point session_start_;
    std::chrono::steady_clock::time_point last_token_time_;
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_DECODE_GOVERNOR_HPP
