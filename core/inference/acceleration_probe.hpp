#ifndef ISHA_AI_ACCELERATION_PROBE_HPP
#define ISHA_AI_ACCELERATION_PROBE_HPP

#include "acceleration_backend.hpp"
#include "../config/device_profile.hpp"

namespace isha {

struct NormalizedEfficiency {
    float tokens_per_watt = 0.0f;
    float battery_drain_estimate_ma = 0.0f;
    float thermal_rise_c_per_sec = 0.0f;
    float sustained_efficiency = 0.0f;
};

class AccelerationProbe {
public:
    static AccelerationCapability runProbe(
        const DeviceProfile& profile, 
        AccelerationBackend backend, 
        int model_size_params_m,
        int prompt_tokens,
        int context_tokens,
        int batch_size,
        int thread_count);

    static NormalizedEfficiency calculateNormalizedEfficiency(
        float tok_sec,
        float battery_current_ma,
        float start_temp_c,
        float end_temp_c,
        float duration_sec,
        int model_size_params_m,
        int prompt_tokens,
        int context_tokens,
        int batch_size,
        int thread_count);
};

} // namespace isha

#endif // ISHA_AI_ACCELERATION_PROBE_HPP
