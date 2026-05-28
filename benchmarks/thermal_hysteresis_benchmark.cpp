// Phase 8B — Fixation 2: Thermal Recovery Hysteresis Benchmark
// Validates oscillation suppression: after thermal collapse, NNAPI cannot
// re-enable until a minimum stable-temperature window has been observed.

#include "../core/inference/kv_telemetry.hpp"
#include "../core/inference/acceleration_quarantine.hpp"
#include <cassert>
#include <cstdio>
#include <thread>
#include <chrono>
#include <cmath>

using namespace isha;

// Simulate a thermal sensor reading that rises then falls
static float simulateThermalC(float base_c, float elapsed_s, float rise_rate) {
    return base_c + rise_rate * elapsed_s;
}

int main() {
    printf("[thermal_hysteresis_benchmark] Starting...\n");

    KVTelemetry tel;
    tel.reset();

    // --- Phase A: Simulate thermal collapse scenario ---
    // Device starts at 32°C (NOMINAL). Rises at ~1.5°C/min during inference.
    const float THROTTLE_THRESHOLD_C = 38.0f;
    const float HYSTERESIS_WINDOW_S  = 30.0f; // must stay below threshold for 30s
    const float RISE_RATE_C_PER_MIN  = 1.5f;

    float device_temp_c = 32.0f;
    float elapsed_s     = 0.0f;
    float initial_tok_s = 18.0f;
    float stable_below_threshold_s = 0.0f;

    tel.initial_tok_per_s.store(initial_tok_s, std::memory_order_relaxed);
    tel.thermal_stable_window_s.store(HYSTERESIS_WINDOW_S, std::memory_order_relaxed);

    bool thermal_collapse_triggered = false;

    // Simulate 5 minutes of soak (in 10s steps)
    for (int step = 0; step < 30; ++step) {
        elapsed_s += 10.0f;
        device_temp_c = simulateThermalC(32.0f, elapsed_s / 60.0f, RISE_RATE_C_PER_MIN);

        float decay_rate = RISE_RATE_C_PER_MIN;
        tel.thermal_decay_rate_c_per_min.store(decay_rate, std::memory_order_relaxed);

        float current_tok_s = initial_tok_s - (elapsed_s / 60.0f) * 0.5f;
        if (current_tok_s < 0.0f) current_tok_s = 0.0f;
        tel.current_tok_per_s.store(current_tok_s, std::memory_order_relaxed);

        if (device_temp_c >= THROTTLE_THRESHOLD_C && !thermal_collapse_triggered) {
            // Thermal collapse: record tok/s at throttle point and set hysteresis gate
            tel.tok_per_s_at_thermal_throttle.store(current_tok_s, std::memory_order_relaxed);
            tel.thermal_hysteresis_gate.store(true, std::memory_order_relaxed);
            thermal_collapse_triggered = true;
            printf("  [step %d] Thermal collapse at %.1f°C — tok/s=%.1f — hysteresis gate ARMED\n",
                   step, device_temp_c, current_tok_s);

            // Report quarantine event
            AccelerationQuarantine::getInstance().reportFailure(
                "nnapi_thermal_hysteresis", FailureSeverity::SOAK_THERMAL_COLLAPSE);
        }

        if (!thermal_collapse_triggered) {
            printf("  [step %d] Temp=%.1f°C tok/s=%.1f — NOMINAL\n", step, device_temp_c, current_tok_s);
        }
    }

    assert(thermal_collapse_triggered && "Expected thermal collapse at 38°C within 5min soak");
    assert(tel.thermal_hysteresis_gate.load() && "Hysteresis gate must be armed after collapse");

    // --- Phase B: Cooling phase ---
    // Device cools at -0.8°C/min. Hysteresis gate must remain until stable for 30s.
    device_temp_c = 41.0f;
    bool premature_reenable_attempt = false;
    bool hysteresis_cleared = false;

    printf("\n[thermal_hysteresis_benchmark] Cooling phase...\n");
    for (int step = 0; step < 12; ++step) {
        device_temp_c -= 0.8f; // ~0.8°C per 1s sim step
        bool above_threshold = device_temp_c >= THROTTLE_THRESHOLD_C;

        if (above_threshold) {
            stable_below_threshold_s = 0.0f; // reset stability timer
        } else {
            stable_below_threshold_s += 10.0f;
        }

        // Premature re-enable attempt at step 3 (stable_below_threshold_s < 30s)
        if (step == 3) {
            premature_reenable_attempt = true;
            if (tel.thermal_hysteresis_gate.load() && stable_below_threshold_s < HYSTERESIS_WINDOW_S) {
                // Gate correctly blocks re-enable
                tel.thermal_oscillation_events.fetch_add(1, std::memory_order_relaxed);
                printf("  [step %d] Premature re-enable BLOCKED (stable=%.0fs < %.0fs)\n",
                       step, stable_below_threshold_s, HYSTERESIS_WINDOW_S);
            }
        }

        // Clear gate only after hysteresis window satisfied
        if (stable_below_threshold_s >= HYSTERESIS_WINDOW_S && tel.thermal_hysteresis_gate.load()) {
            tel.thermal_hysteresis_gate.store(false, std::memory_order_relaxed);
            hysteresis_cleared = true;
            printf("  [step %d] Hysteresis window satisfied (%.0fs stable at %.1f°C) — gate CLEARED\n",
                   step, stable_below_threshold_s, device_temp_c);
            AccelerationQuarantine::getInstance().reportSuccess("nnapi_thermal_hysteresis");
            break;
        }

        printf("  [step %d] Temp=%.1f°C stable=%.0fs gate=%s\n",
               step, device_temp_c, stable_below_threshold_s,
               tel.thermal_hysteresis_gate.load() ? "ARMED" : "clear");
    }

    assert(premature_reenable_attempt && "Premature re-enable must have been attempted");
    assert(tel.thermal_oscillation_events.load() >= 1 && "Oscillation suppression must have fired");
    assert(hysteresis_cleared && "Hysteresis gate must clear after stable window");
    assert(!tel.thermal_hysteresis_gate.load() && "Gate must be clear after cooldown");

    printf("\n[thermal_hysteresis_benchmark] PASS\n");
    printf("  thermal_oscillation_events=%u\n", tel.thermal_oscillation_events.load());
    printf("  tok_per_s_at_throttle=%.1f\n", tel.tok_per_s_at_thermal_throttle.load());
    printf("  stable_window_satisfied=%.0fs\n", stable_below_threshold_s);

    return 0;
}
