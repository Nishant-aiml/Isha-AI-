#include "survival/reinstall_recovery_policy.hpp"
#include <iostream>
#include <fstream>
#include <cassert>

int main() {
    std::cout << "Starting reinstall_recovery_policy_benchmark..." << std::endl;

    std::string test_dir = ".";
    std::string stale_db = test_dir + "/stale_cache.db";

    // Create a stale remnant file
    std::ofstream out(stale_db);
    out << "OLD_DATA";
    out.close();

    // Perform recovery sanitization
    auto report = isha::ReinstallRecoveryPolicy::performReinstallSanitization(test_dir);
    assert(report.files_purged > 0);
    assert(report.cache_reset);

    // Verify file is gone
    std::ifstream check(stale_db);
    assert(!check.good());

    std::cout << "reinstall_recovery_policy_benchmark PASSED!" << std::endl;
    return 0;
}
