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

    // Phase 11A expanded manifest fields
    unsigned int physical_ram_mb = 0;
    unsigned int free_ram_mb = 0;
    std::string storage_class = "";
    std::string thermal_class = "";
    int battery_health = 100;
    int battery_capacity = 4500;
    bool nnapi_supported = true;
    int nnapi_score = 100;
    std::string device_vendor = "Unknown";
    std::string device_model = "Unknown";
    std::string android_version = "Unknown";
};

struct SafeDefaults {
    std::string recommended_model_dimension;
    unsigned int max_batch_size;
    unsigned int max_context_cells;
    unsigned int telemetry_interval_ms;
    std::string recommended_startup_strategy;
    double thermal_threshold_ceiling_c;
    bool allow_hardware_acceleration;

    // Phase 11A manifest outputs
    std::string ram_tier;
    std::string safe_max_tier;
    std::string recommended_model;
};

class InstallTimeDeviceProfiler {
public:
    // Profiles hardware spec values during installation/first launch and computes optimal operational boundaries
    static SafeDefaults profileDevice(const DeviceRawSpecs& specs);

    // Writes the capability manifest JSON to the specified path
    static bool writeManifest(const std::string& path, const DeviceRawSpecs& specs, const SafeDefaults& defaults);
};

} // namespace isha

#endif // ISHA_AI_INSTALL_TIME_DEVICE_PROFILER_HPP
