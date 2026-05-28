#ifndef ISHA_AI_THERMAL_RECOVERY_WINDOW_HPP
#define ISHA_AI_THERMAL_RECOVERY_WINDOW_HPP

#include <string>
#include <chrono>

namespace isha {

enum class ThermalRecoveryState {
    HOT,
    COOLING,
    STABILIZING,
    RECOVERED
};

struct ThermalRecoveryMetrics {
    double cooldown_duration_sec;
    unsigned int repeated_thermal_spikes;
    double oscillation_frequency;
    double recovery_quality;
    double throughput_decay_ratio;
};

class ThermalRecoveryWindow {
public:
    ThermalRecoveryWindow();
    ~ThermalRecoveryWindow() = default;

    static std::string stateToString(ThermalRecoveryState state);

    // Records a temperature metric and transitions state machine
    ThermalRecoveryState updateTemperature(double temp_c);

    // Configures safety pacing intervals to prevent thermal bounce loops
    unsigned int getPacingDelayMs() const;

    // Checks if full hardware acceleration is safe to boost
    bool isAccelerationRestorationSafe() const;

    // Retrieve stats
    ThermalRecoveryState getState() const { return current_state_; }
    ThermalRecoveryMetrics getMetrics() const;

private:
    ThermalRecoveryState current_state_;
    double last_temperature_;
    std::chrono::steady_clock::time_point last_state_change_;
    unsigned int spike_count_;
    unsigned int state_oscillations_;
    double initial_throughput_;
    double current_throughput_;
};

} // namespace isha

#endif // ISHA_AI_THERMAL_RECOVERY_WINDOW_HPP
