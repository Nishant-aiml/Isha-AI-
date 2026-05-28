#ifndef ISHA_AI_ACCELERATION_BACKEND_HPP
#define ISHA_AI_ACCELERATION_BACKEND_HPP

#include <string>
#include "../config/device_profile.hpp"

namespace isha {

enum class AccelerationBackend {
    CPU_ONLY,
    NNAPI,
    VULKAN,
    HYBRID,
    DISABLED,
    FALLBACK_RECOVERY
};

struct AccelerationCapability {
    bool nnapi_available = false;
    bool vulkan_available = false;
    int vulkan_api_version = 0;
    std::string nnapi_driver_name;
    std::string gpu_name;
    uint64_t gpu_memory_bytes = 0;
    int max_safe_gpu_layers = 0;
    float probe_speedup_ratio = 0.0f;
    bool probe_passed = false;
};

struct AccelerationConfig {
    AccelerationBackend requested_backend = AccelerationBackend::CPU_ONLY;
    int n_gpu_layers = 0;
    bool allow_fallback = true;
    int probe_timeout_ms = 5000;
    float min_speedup_ratio = 1.1f;
};

class IAccelerationBackend {
public:
    virtual ~IAccelerationBackend() = default;
    virtual AccelerationBackend type() const = 0;
    virtual AccelerationCapability probe(const DeviceProfile& profile) = 0;
    virtual bool initialize(const AccelerationConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual int recommendedGpuLayers(int model_embd_size) const = 0;
    virtual bool isHealthy() const = 0;
};

} // namespace isha

#endif // ISHA_AI_ACCELERATION_BACKEND_HPP
