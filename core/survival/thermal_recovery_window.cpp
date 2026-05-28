#include "thermal_recovery_window.hpp"

namespace isha {

ThermalRecoveryWindow::ThermalRecoveryWindow()
    : current_state_(ThermalRecoveryState::RECOVERED),
      last_temperature_(30.0),
      last_state_change_(std::chrono::steady_clock::now()),
      spike_count_(0),
      state_oscillations_(0),
      initial_throughput_(15.0),
      current_throughput_(15.0) {}

std::string ThermalRecoveryWindow::stateToString(ThermalRecoveryState state) {
    switch (state) {
        case ThermalRecoveryState::HOT: return "HOT";
        case ThermalRecoveryState::COOLING: return "COOLING";
        case ThermalRecoveryState::STABILIZING: return "STABILIZING";
        case ThermalRecoveryState::RECOVERED: return "RECOVERED";
        default: return "UNKNOWN";
    }
}

ThermalRecoveryState ThermalRecoveryWindow::updateTemperature(double temp_c) {
    auto now = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(now - last_state_change_).count();

    ThermalRecoveryState next_state = current_state_;

    // 1. Evaluate State transitions
    if (temp_c > 41.0) {
        if (current_state_ != ThermalRecoveryState::HOT) {
            spike_count_++;
            if (current_state_ == ThermalRecoveryState::STABILIZING || 
                current_state_ == ThermalRecoveryState::COOLING) {
                state_oscillations_++; // Oscillating hot again too early!
            }
            next_state = ThermalRecoveryState::HOT;
            last_state_change_ = now;
        }
    } else if (current_state_ == ThermalRecoveryState::HOT && temp_c <= 41.0 && temp_c > 38.0) {
        next_state = ThermalRecoveryState::COOLING;
        last_state_change_ = now;
    } else if (current_state_ == ThermalRecoveryState::COOLING && temp_c <= 38.0) {
        next_state = ThermalRecoveryState::STABILIZING;
        last_state_change_ = now;
    } else if (current_state_ == ThermalRecoveryState::STABILIZING && temp_c <= 35.0 && duration > 5.0) {
        next_state = ThermalRecoveryState::RECOVERED;
        last_state_change_ = now;
    }

    current_state_ = next_state;
    last_temperature_ = temp_c;

    // Adapt simulated throughput decay
    if (current_state_ == ThermalRecoveryState::HOT) {
        current_throughput_ = initial_throughput_ * 0.3; // 70% decay
    } else if (current_state_ == ThermalRecoveryState::COOLING) {
        current_throughput_ = initial_throughput_ * 0.5;
    } else if (current_state_ == ThermalRecoveryState::STABILIZING) {
        current_throughput_ = initial_throughput_ * 0.8;
    } else {
        current_throughput_ = initial_throughput_;
    }

    return current_state_;
}

unsigned int ThermalRecoveryWindow::getPacingDelayMs() const {
    switch (current_state_) {
        case ThermalRecoveryState::HOT: return 2000;         // High throttling wait
        case ThermalRecoveryState::COOLING: return 1000;     // Moderate pacing wait
        case ThermalRecoveryState::STABILIZING: return 200;  // Minimal pacing wait
        case ThermalRecoveryState::RECOVERED: return 0;
        default: return 0;
    }
}

bool ThermalRecoveryWindow::isAccelerationRestorationSafe() const {
    // Only allow restoring hardware accelerators when we are fully stabilized and have no immediate oscillation risk
    return current_state_ == ThermalRecoveryState::RECOVERED && state_oscillations_ < 3;
}

ThermalRecoveryMetrics ThermalRecoveryWindow::getMetrics() const {
    ThermalRecoveryMetrics metrics;
    auto now = std::chrono::steady_clock::now();
    metrics.cooldown_duration_sec = std::chrono::duration<double>(now - last_state_change_).count();
    metrics.repeated_thermal_spikes = spike_count_;
    metrics.oscillation_frequency = static_cast<double>(state_oscillations_);
    metrics.throughput_decay_ratio = current_throughput_ / initial_throughput_;
    
    // Recovery quality decreases with repeated loops
    metrics.recovery_quality = (state_oscillations_ == 0) ? 1.0 : (1.0 / (1.0 + state_oscillations_));
    return metrics;
}

} // namespace isha
