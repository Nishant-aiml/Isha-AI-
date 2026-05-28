#include "acceleration_probe.hpp"
#include <thread>
#include <chrono>
#include <cmath>

namespace isha {

AccelerationCapability AccelerationProbe::runProbe(
    const DeviceProfile& profile, 
    AccelerationBackend backend, 
    int model_size_params_m,
    int prompt_tokens,
    int context_tokens,
    int batch_size,
    int thread_count) {

    AccelerationCapability cap;
    if (backend == AccelerationBackend::CPU_ONLY) {
        cap.probe_passed = true;
        cap.probe_speedup_ratio = 1.0f;
        return cap;
    }

    // 1. Warmup Phase: Compile shaders, resident memory
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 2. Stabilization Phase: Latency check
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    // 3. Measured Phase: Latency check
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Simulated benchmark outputs
    float simulated_tok_sec = 0.0f;
    float simulated_current_ma = 1500.0f; // raw current
    
    if (backend == AccelerationBackend::VULKAN) {
        simulated_tok_sec = profile.has_vulkan ? 24.0f : 0.0f;
        cap.vulkan_available = profile.has_vulkan;
        cap.gpu_name = profile.gpu_name;
    } else if (backend == AccelerationBackend::NNAPI) {
        simulated_tok_sec = profile.has_nnapi ? 20.0f : 0.0f;
        cap.nnapi_available = profile.has_nnapi;
    }

    float cpu_tok_sec = 12.0f;
    cap.probe_speedup_ratio = simulated_tok_sec / cpu_tok_sec;

    NormalizedEfficiency eff = calculateNormalizedEfficiency(
        simulated_tok_sec,
        simulated_current_ma,
        35.0f, // Start Temp
        35.5f, // End Temp
        0.1f,  // Duration
        model_size_params_m,
        prompt_tokens,
        context_tokens,
        batch_size,
        thread_count
    );

    // Gating check
    if (cap.probe_speedup_ratio >= 1.1f && eff.tokens_per_watt > 0.01f) {
        cap.probe_passed = true;
    } else {
        cap.probe_passed = false;
    }

    return cap;
}

NormalizedEfficiency AccelerationProbe::calculateNormalizedEfficiency(
    float tok_sec,
    float battery_current_ma,
    float start_temp_c,
    float end_temp_c,
    float duration_sec,
    int model_size_params_m,
    int prompt_tokens,
    int context_tokens,
    int batch_size,
    int thread_count) {

    NormalizedEfficiency eff;
    eff.battery_drain_estimate_ma = battery_current_ma;
    eff.thermal_rise_c_per_sec = (end_temp_c - start_temp_c) / std::max(duration_sec, 0.001f);

    // Power (Watts) = Current (Amps) * Voltage (Assume constant 3.7V for mobile)
    float power_watts = (battery_current_ma / 1000.0f) * 3.7f;
    float raw_tokens_per_watt = (power_watts > 0.0f) ? (tok_sec / power_watts) : 0.0f;

    // Normalization factors
    // Larger model, larger context, prompt size, batch size, thread count increases efficiency credit
    float model_factor = static_cast<float>(model_size_params_m) / 1000.0f;
    float context_factor = static_cast<float>(context_tokens) / 512.0f;
    float prompt_factor = static_cast<float>(prompt_tokens) / 64.0f;
    float batch_factor = static_cast<float>(batch_size);
    float thread_factor = 1.0f / std::max(1.0f, static_cast<float>(thread_count));

    eff.tokens_per_watt = raw_tokens_per_watt * model_factor * context_factor * prompt_factor * batch_factor * thread_factor;
    eff.sustained_efficiency = eff.tokens_per_watt / std::max(1.0f, eff.thermal_rise_c_per_sec);

    return eff;
}

} // namespace isha
