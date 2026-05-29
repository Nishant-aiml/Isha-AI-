#include "install_time_device_profiler.hpp"
#include <fstream>
#include <sstream>

namespace isha {

SafeDefaults InstallTimeDeviceProfiler::profileDevice(const DeviceRawSpecs& specs) {
    SafeDefaults defaults;

    // Phase 11A Model recommendation & Safe Max Tier logic
    // 4GB -> T1 (Qwen 1.5B)
    // 6GB -> T2 (Gemma-3-2B)
    // 8GB -> T2 (Gemma-3-2B)
    // 12GB+ -> T3 (Mistral 7B)

    if (specs.ram_gb <= 4) {
        defaults.recommended_model_dimension = "1.5B_MIDRANGE"; // 0.5B is router only; T1 is default chatter
        defaults.max_batch_size = 4;
        defaults.max_context_cells = 512;
        defaults.telemetry_interval_ms = 60000;
        defaults.recommended_startup_strategy = "SAFE_MODE_STARTUP";
        defaults.thermal_threshold_ceiling_c = 41.0; // T1 limit
        defaults.allow_hardware_acceleration = false; // CPU fallback for thermal safety on low-end
        
        defaults.ram_tier = "BUDGET_4GB";
        defaults.safe_max_tier = "T1";
        defaults.recommended_model = "qwen2.5-1.5b-instruct-q4_k_m.gguf";
    } else if (specs.ram_gb <= 6) {
        defaults.recommended_model_dimension = "1.5B_MIDRANGE";
        defaults.max_batch_size = 8;
        defaults.max_context_cells = 1024;
        defaults.telemetry_interval_ms = 30000;
        defaults.recommended_startup_strategy = "WARM_STARTUP";
        defaults.thermal_threshold_ceiling_c = 41.0;
        defaults.allow_hardware_acceleration = !specs.is_emmc;
        
        defaults.ram_tier = "MIDRANGE_6GB";
        defaults.safe_max_tier = "T2";
        defaults.recommended_model = "qwen2.5-1.5b-instruct-q4_k_m.gguf";
    } else if (specs.ram_gb <= 8) {
        defaults.recommended_model_dimension = "ADVANCED_CONFIGS";
        defaults.max_batch_size = 16;
        defaults.max_context_cells = 2048;
        defaults.telemetry_interval_ms = 5000;
        defaults.recommended_startup_strategy = "COLD_STARTUP";
        defaults.thermal_threshold_ceiling_c = 44.0; // T2 limit
        defaults.allow_hardware_acceleration = true;
        
        defaults.ram_tier = "HIGH_8GB";
        defaults.safe_max_tier = "T2";
        defaults.recommended_model = "gemma-3-2b-it-q4_0.gguf";
    } else {
        defaults.recommended_model_dimension = "ADVANCED_CONFIGS";
        defaults.max_batch_size = 32;
        defaults.max_context_cells = 2048;
        defaults.telemetry_interval_ms = 5000;
        defaults.recommended_startup_strategy = "COLD_STARTUP";
        defaults.thermal_threshold_ceiling_c = 45.0; // T3 limit
        defaults.allow_hardware_acceleration = true;
        
        defaults.ram_tier = "FLAGSHIP_12GB_PLUS";
        defaults.safe_max_tier = "T3";
        defaults.recommended_model = "mistral-7b-instruct-v0.3-q4_k_m.gguf";
    }

    // Tweak context under active OEM pressure
    if (specs.has_oem_background_kill_app || specs.foreground_service_restricted) {
        defaults.max_context_cells = (defaults.max_context_cells > 512) ? 512 : defaults.max_context_cells;
        defaults.telemetry_interval_ms = 120000;
    }

    return defaults;
}

bool InstallTimeDeviceProfiler::writeManifest(const std::string& path, const DeviceRawSpecs& specs, const SafeDefaults& defaults) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    // Use default values if specs are unpopulated (e.g. 0) to ensure non-empty manifest matching the JSON template
    unsigned int physical_ram = specs.physical_ram_mb > 0 ? specs.physical_ram_mb : (specs.ram_gb * 1024);
    unsigned int free_ram = specs.free_ram_mb > 0 ? specs.free_ram_mb : (physical_ram / 2);
    std::string storage_cls = !specs.storage_class.empty() ? specs.storage_class : (specs.is_emmc ? "eMMC" : "UFS");
    std::string thermal_cls = !specs.thermal_class.empty() ? specs.thermal_class : "MODERATE";

    file << "{\n";
    file << "  \"ram_tier\": \"" << defaults.ram_tier << "\",\n";
    file << "  \"physical_ram_mb\": " << physical_ram << ",\n";
    file << "  \"free_ram_mb\": " << free_ram << ",\n";
    file << "  \"storage_class\": \"" << storage_cls << "\",\n";
    file << "  \"thermal_class\": \"" << thermal_cls << "\",\n";
    file << "  \"battery_health\": " << specs.battery_health << ",\n";
    file << "  \"battery_capacity\": " << specs.battery_capacity << ",\n";
    file << "  \"nnapi_supported\": " << (specs.nnapi_supported ? "true" : "false") << ",\n";
    file << "  \"nnapi_score\": " << specs.nnapi_score << ",\n";
    file << "  \"device_vendor\": \"" << specs.device_vendor << "\",\n";
    file << "  \"device_model\": \"" << specs.device_model << "\",\n";
    file << "  \"android_version\": \"" << specs.android_version << "\",\n";
    file << "  \"safe_max_tier\": \"" << defaults.safe_max_tier << "\",\n";
    file << "  \"recommended_model\": \"" << defaults.recommended_model << "\"\n";
    file << "}\n";

    file.close();
    return true;
}

} // namespace isha
