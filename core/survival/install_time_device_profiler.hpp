#ifndef ISHA_AI_INSTALL_TIME_DEVICE_PROFILER_HPP
#define ISHA_AI_INSTALL_TIME_DEVICE_PROFILER_HPP

#include <string>

namespace isha {

struct DeviceRawSpecs {
    unsigned int ram_gb;
    bool is_emmc;
    double measured_write_latency_ms;
    std::string chipset_family;
    unsigned int android_api_level;
    bool has_oem_background_kill_app;
    bool battery_saver_active;
    bool foreground_service_restricted;
};

struct SafeDefaults {
    std::string recommended_model_dimension;
    unsigned int max_batch_size;
    unsigned int max_context_cells;
    unsigned int telemetry_interval_ms;
    std::string recommended_startup_strategy;
    double thermal_threshold_ceiling_c;
    bool allow_hardware_acceleration;
};

class InstallTimeDeviceProfiler {
public:
    // Profiles hardware spec values during installation/first launch and computes optimal operational boundaries
    static SafeDefaults profileDevice(const DeviceRawSpecs& specs);
};

} // namespace isha

#endif // ISHA_AI_INSTALL_TIME_DEVICE_PROFILER_HPP
