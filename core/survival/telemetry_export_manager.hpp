#ifndef ISHA_AI_TELEMETRY_EXPORT_MANAGER_HPP
#define ISHA_AI_TELEMETRY_EXPORT_MANAGER_HPP

#include <string>
#include <vector>

namespace isha {

struct ExportPackage {
    std::string export_path;
    size_t package_size_bytes;
    bool export_success;
    std::string system_fingerprint;
};

class TelemetryExportManager {
public:
    // Creates a packed diagnostic package containing logs, crash history, thermals, and recovery logs
    // Strictly user-triggered, offline-only, never transmitted over the network
    static ExportPackage generateDiagnosticPackage(const std::string& target_directory);

    // Verifies the structural integrity of the export file
    static bool verifyPackageIntegrity(const std::string& package_path);
};

} // namespace isha

#endif // ISHA_AI_TELEMETRY_EXPORT_MANAGER_HPP
