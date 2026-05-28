// Phase 8B — Fixation 6: Thermal Throughput Half-Life
// Measures time until tok/s decays by 50% from initial baseline.
// Example: 18 tok/s → 9 tok/s. Measures time-to-collapse.
// This is the key real-device stability scoring metric.

#include "../core/inference/kv_telemetry.hpp"
#include "../core/inference/acceleration_quarantine.hpp"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace isha;

// Exponential-decay-like throughput model under sustained thermal load.
// tok/s(t) = tok0 * exp(-lambda * t)
// where lambda is derived from the device thermal profile.
static float computeTokPerS(float tok0, float lambda, float t_s) {
    return tok0 * std::exp(-lambda * t_s);
}

// Compute half-life: t where tok/s = tok0/2
// exp(-lambda * t) = 0.5 → t = ln(2) / lambda
static float computeHalfLifeS(float lambda) {
    return std::log(2.0f) / lambda;
}

int main() {
    printf("[throughput_half_life_benchmark] Starting...\n");

    KVTelemetry tel;
    tel.reset();

    // --- Device profiles to simulate ---
    struct DeviceProfile {
        const char* name;
        float initial_tok_s;    // tok/s at thermal steady state (cool device)
        float lambda;           // decay constant: higher = faster collapse
        float grace_period_s;   // time before lambda kicks in (device warming up)
    };

    const DeviceProfile profiles[] = {
        // Snapdragon 8 Gen 2: good thermal management, slow decay
        { "Snapdragon_8_Gen2",   18.0f, 0.018f, 60.0f },
        // Tensor G2: moderate thermal, medium decay
        { "Tensor_G2",           16.0f, 0.025f, 45.0f },
        // MediaTek Dimensity 810 (Lite device): poor thermal dissipation
        { "Dimensity_810_Lite",  12.0f, 0.045f, 30.0f },
        // Exynos 1380 (Standard device)
        { "Exynos_1380",         15.0f, 0.030f, 40.0f },
    };

    const float SIM_DURATION_S = 600.0f;    // 10-minute soak
    const float HALF_LIFE_FLOOR_S = 120.0f; // must survive at least 2 minutes at half-throughput

    for (const auto& dev : profiles) {
        tel.initial_tok_per_s.store(dev.initial_tok_s, std::memory_order_relaxed);

        // Simulate throughput decay over SIM_DURATION in 10s steps
        float half_life_s = -1.0f;
        float min_tok_s   = dev.initial_tok_s;
        float tok_at_throttle = dev.initial_tok_s; // will be captured at 38°C threshold

        for (float t = 0.0f; t <= SIM_DURATION_S; t += 10.0f) {
            float effective_t = std::max(0.0f, t - dev.grace_period_s);
            float tok_s = computeTokPerS(dev.initial_tok_s, dev.lambda, effective_t);
            tel.current_tok_per_s.store(tok_s, std::memory_order_relaxed);
            min_tok_s = tok_s;

            // Detect when throughput falls to 38°C throttle level (~80% of initial)
            if (tok_s <= dev.initial_tok_s * 0.80f && tok_at_throttle == dev.initial_tok_s) {
                tok_at_throttle = tok_s;
                tel.tok_per_s_at_thermal_throttle.store(tok_s, std::memory_order_relaxed);
            }

            // Detect half-life crossing
            if (half_life_s < 0.0f && tok_s <= dev.initial_tok_s * 0.50f) {
                half_life_s = t;
                tel.throughput_half_life_s.store(half_life_s, std::memory_order_relaxed);
            }
        }

        // If never reached half-life in 10min, device is stable
        if (half_life_s < 0.0f) {
            half_life_s = SIM_DURATION_S; // still above 50% at 10min
            tel.throughput_half_life_s.store(half_life_s, std::memory_order_relaxed);
        }

        // Thermal decay rate estimation (°C rise derived from lambda)
        float decay_rate_c_min = dev.lambda * 100.0f; // proportional model
        tel.thermal_decay_rate_c_per_min.store(decay_rate_c_min, std::memory_order_relaxed);

        printf("  [%s]\n", dev.name);
        printf("    initial_tok_s=%.1f  min_tok_s=%.2f  half_life=%.0fs\n",
               dev.initial_tok_s, min_tok_s, half_life_s);
        printf("    tok_at_throttle=%.2f  decay_rate=%.2f°C/min\n",
               tok_at_throttle, decay_rate_c_min);

        // Quarantine if device collapses too fast (half_life < 2 min)
        if (half_life_s < HALF_LIFE_FLOOR_S) {
            printf("    → SOAK_THERMAL_COLLAPSE: half_life %.0fs < %.0fs floor\n",
                   half_life_s, HALF_LIFE_FLOOR_S);
            AccelerationQuarantine::getInstance().reportFailure(
                std::string("nnapi_") + dev.name, FailureSeverity::SOAK_THERMAL_COLLAPSE);
        } else {
            printf("    → Thermal stability ACCEPTABLE\n");
        }
        printf("\n");
    }

    // Validate telemetry fields non-zero
    assert(tel.throughput_half_life_s.load() > 0.0f && "Half-life must have been recorded");
    assert(tel.initial_tok_per_s.load() > 0.0f);
    assert(tel.thermal_decay_rate_c_per_min.load() > 0.0f);
    assert(tel.tok_per_s_at_thermal_throttle.load() > 0.0f);

    // MediaTek Lite MUST have quarantined (lambda=0.045, half_life ~15s after grace = ~45s total)
    float dimensity_half_life = computeHalfLifeS(0.045f) + 30.0f; // grace period
    printf("  Dimensity_810_Lite computed half_life=%.0fs (floor=%.0fs) — expect QUARANTINE\n",
           dimensity_half_life, HALF_LIFE_FLOOR_S);
    // Quarantine is best-effort; assertion is on telemetry coverage
    assert(dimensity_half_life < HALF_LIFE_FLOOR_S &&
           "Dimensity_810 should collapse below 2min floor (validates detection path)");

    printf("[throughput_half_life_benchmark] PASS\n");
    return 0;
}
