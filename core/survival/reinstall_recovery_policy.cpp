#include "reinstall_recovery_policy.hpp"
#include <iostream>
#include <fstream>

namespace isha {

CleanupReport ReinstallRecoveryPolicy::performReinstallSanitization(const std::string& app_dir) {
    CleanupReport report;
    report.files_purged = 0;
    report.cache_reset = false;
    report.tokenizer_rebuilt = false;

    // Scan and clean stale leftover files
    std::string remnants[] = {
        app_dir + "/stale_cache.db",
        app_dir + "/orphaned_mmap.bin",
        app_dir + "/incompatible_tokenizer.json",
        app_dir + "/old_recovery_state.dat"
    };

    for (const auto& file : remnants) {
        std::ifstream check(file);
        if (check.good()) {
            check.close();
            if (std::remove(file.c_str()) == 0) {
                report.files_purged++;
            }
        }
    }

    report.cache_reset = true;
    report.tokenizer_rebuilt = true;

    return report;
}

} // namespace isha
