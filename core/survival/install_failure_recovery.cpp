#include "install_failure_recovery.hpp"
#include <iostream>
#include <fstream>

namespace isha {

ExtractionResult InstallFailureRecovery::stageAndUnpack(const std::string& source_archive, 
                                                       const std::string& destination_file, 
                                                       size_t available_storage_bytes) {
    ExtractionResult res;
    res.success = false;
    res.rolled_back = false;
    res.storage_freed = false;
    res.active_bytes = 0;

    // Simulate archive size requirements
    size_t required_bytes = 400 * 1024 * 1024; // 400MB
    if (available_storage_bytes < required_bytes) {
        // Force cleanup and staging rollback
        res.rolled_back = true;
        res.storage_freed = true;
        std::remove(destination_file.c_str());
        return res;
    }

    // Simulate staging
    std::ofstream out(destination_file, std::ios::binary);
    if (!out) return res;

    // Write a dummy staging marker
    out.write("STAGE_OK", 8);
    out.close();

    res.success = true;
    res.active_bytes = 8;
    return res;
}

bool InstallFailureRecovery::verifyAndPurgeStaleInstalls(const std::string& app_dir) {
    std::string partial_file = app_dir + "/temp_unpack.tmp";
    std::ifstream check(partial_file);
    if (check.good()) {
        check.close();
        std::remove(partial_file.c_str());
        return true;
    }
    return false;
}

} // namespace isha
