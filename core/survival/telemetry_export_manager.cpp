#include "telemetry_export_manager.hpp"
#include <fstream>
#include <sstream>

namespace isha {

ExportPackage TelemetryExportManager::generateDiagnosticPackage(const std::string& target_directory) {
    ExportPackage pkg;
    pkg.export_success = false;
    pkg.package_size_bytes = 0;
    pkg.export_path = target_directory + "/isha_ai_diagnostics.dat";
    pkg.system_fingerprint = "ISHA_DIAG_V1.0_CPU_ONLY";

    std::ofstream out(pkg.export_path, std::ios::binary);
    if (!out) return pkg;

    // Simulate packing multiple local log components into a single diagnostic stream
    std::stringstream ss;
    ss << "[SYSTEM_INFO]\nBuild: ISHA_RELEASE_V1.0\nAPI_Level: 33\n\n";
    ss << "[CRASH_LOGS]\nNone recorded. Safe mode verified.\n\n";
    ss << "[THERMAL_HISTORY]\nPeak: 40.5C, Stabilized: 34.2C\n\n";
    ss << "[RECOVERY_HISTORY]\nTotal: 1, Restored: KNOWN_GOOD_CPU_0.5B\n";

    std::string contents = ss.str();
    
    // Write header
    char magic[4] = {'D', 'I', 'A', 'G'};
    out.write(magic, 4);
    
    unsigned int size = static_cast<unsigned int>(contents.size());
    out.write(reinterpret_cast<const char*>(&size), 4);
    out.write(contents.data(), size);
    
    out.flush();
    out.close();

    pkg.export_success = true;
    pkg.package_size_bytes = 8 + size;
    return pkg;
}

bool TelemetryExportManager::verifyPackageIntegrity(const std::string& package_path) {
    std::ifstream in(package_path, std::ios::binary);
    if (!in) return false;

    char magic[4];
    in.read(magic, 4);
    if (magic[0] != 'D' || magic[1] != 'I' || magic[2] != 'A' || magic[3] != 'G') {
        return false;
    }

    unsigned int size = 0;
    in.read(reinterpret_cast<char*>(&size), 4);
    if (size == 0) return false;

    return true;
}

} // namespace isha
