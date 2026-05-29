#ifndef ISHA_AI_INFERENCE_THREAD_POLICY_HPP
#define ISHA_AI_INFERENCE_THREAD_POLICY_HPP

#include <algorithm>
#include "../config/device_profile.hpp"

namespace isha {

struct InferenceThreadPolicy {
    static constexpr int MIN_THREADS = 1;
    static constexpr int MAX_THREADS = 8;

    // Compute optimal thread count based on device and conditions
    static int computeThreads(const DeviceProfile& profile, double temperature_c,
                              int model_embd_size,
                              bool battery_saver = false, float scheduler_pressure = 0.0f) {
        int threads = baseThreadsForTier(profile.tier);

        // Reduce threads for large models (>1024 embd = 1.5B+)
        if (model_embd_size > 1024) {
            threads = reducedThreadsForLargeModel(profile.tier);
        }

        threads -= thermalReduction(temperature_c);

        if (battery_saver) {
            threads /= 2;
        }

        if (scheduler_pressure > 0.7f) {
            threads -= 1;
        }

        return std::clamp(threads, MIN_THREADS, MAX_THREADS);
    }

    static int computeThreads(const DeviceProfile& profile, double temperature_c,
                              bool battery_saver = false, float scheduler_pressure = 0.0f) {
        return computeThreads(profile, temperature_c, 0, battery_saver, scheduler_pressure);
    }

    // FIXATION 2: Hardened thread counts for 1.5B
    static int reducedThreadsForLargeModel(DeviceTier tier) {
        switch (tier) {
            case DeviceTier::LOW:      return 1;  // Should never load 1.5B
            case DeviceTier::MID:      return 2;  // STRICT 2-3 max
            case DeviceTier::HIGH:     return 4;
            case DeviceTier::FLAGSHIP: return 4;  // Conservative, not 6
            default:                   return 2;
        }
    }

    // Tier-based base threads: LOW=2, MID=2, HIGH=5, FLAGSHIP=6
    static int baseThreadsForTier(DeviceTier tier) {
        switch (tier) {
            case DeviceTier::LOW:      return 2;
            case DeviceTier::MID:      return 2;
            case DeviceTier::HIGH:     return 5;
            case DeviceTier::FLAGSHIP: return 6;
            default:                   return 2;
        }
    }

    // Thermal reduction: reduce by 1 thread per 3°C above 38°C
    static int thermalReduction(double temperature_c) {
        if (temperature_c <= 38.0) {
            return 0;
        }
        return static_cast<int>((temperature_c - 38.0) / 3.0);
    }
};

} // namespace isha

#endif // ISHA_AI_INFERENCE_THREAD_POLICY_HPP
