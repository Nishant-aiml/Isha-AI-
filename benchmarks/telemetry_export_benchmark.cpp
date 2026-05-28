#include "survival/telemetry_export_manager.hpp"
#include <iostream>
#include <fstream>
#include <cassert>

int main() {
    std::cout << "[INFO] Running Telemetry Export Validation Benchmark..." << std::endl;

    // Use current directory for temporary diagnostic package
    std::string test_dir = ".";
    
    // Generate diagnostic package
    auto pkg = isha::TelemetryExportManager::generateDiagnosticPackage(test_dir);
    
    std::cout << "[INFO] Package Export Status: " << (pkg.export_success ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "[INFO] Package Path: " << pkg.export_path << std::endl;
    std::cout << "[INFO] Package Size: " << pkg.package_size_bytes << " bytes" << std::endl;
    std::cout << "[INFO] Fingerprint: " << pkg.system_fingerprint << std::endl;

    if (!pkg.export_success) {
        std::cerr << "[ERROR] Diagnostic package generation failed!" << std::endl;
        return 1;
    }

    if (pkg.package_size_bytes == 0) {
        std::cerr << "[ERROR] Exported package is empty!" << std::endl;
        return 1;
    }

    // Verify package integrity
    bool integrity_ok = isha::TelemetryExportManager::verifyPackageIntegrity(pkg.export_path);
    std::cout << "[INFO] Integrity Check: " << (integrity_ok ? "PASSED" : "FAILED") << std::endl;

    if (!integrity_ok) {
        std::cerr << "[ERROR] Package integrity verification failed!" << std::endl;
        return 1;
    }

    // Clean up
    std::remove(pkg.export_path.c_str());
    std::cout << "[INFO] Cleanup completed. Telemetry Export validated successfully." << std::endl;

    return 0;
}
