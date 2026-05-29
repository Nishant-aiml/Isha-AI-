#include <iostream>
#include <cassert>
#include "survival/install_time_device_profiler.hpp"

using namespace isha;

int main() {
    std::cout << "Starting install_time_device_profiler_benchmark..." << std::endl;

    DeviceRawSpecs specs;
    specs.ram_gb = 4;
    specs.is_emmc = true;
    specs.measured_write_latency_ms = 45.0;
    specs.chipset_family = "MediaTek";
    specs.android_api_level = 28;
    specs.has_oem_background_kill_app = true;
    specs.battery_saver_active = false;
    specs.foreground_service_restricted = false;

    auto defaults = InstallTimeDeviceProfiler::profileDevice(specs);
    assert(defaults.recommended_model_dimension == "1.5B_MIDRANGE");
    assert(defaults.max_batch_size == 4);
    assert(!defaults.allow_hardware_acceleration);

    specs.ram_gb = 8;
    specs.is_emmc = false;
    specs.has_oem_background_kill_app = false;
    defaults = InstallTimeDeviceProfiler::profileDevice(specs);
    assert(defaults.recommended_model_dimension == "ADVANCED_CONFIGS");
    assert(defaults.max_batch_size == 16);
    assert(defaults.allow_hardware_acceleration);

    std::cout << "install_time_device_profiler_benchmark PASSED!" << std::endl;
    return 0;
}
