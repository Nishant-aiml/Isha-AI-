#include "survival/install_failure_recovery.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Starting install_failure_recovery_benchmark..." << std::endl;

    std::string test_archive = "test_model.zip";
    std::string test_dest = "unpacked_model.gguf";

    // 1. Storage too low for unpack
    size_t storage_low = 100 * 1024 * 1024; // 100MB
    auto res_low = isha::InstallFailureRecovery::stageAndUnpack(test_archive, test_dest, storage_low);
    assert(!res_low.success);
    assert(res_low.rolled_back);
    assert(res_low.storage_freed);

    // 2. Storage sufficient
    size_t storage_high = 500 * 1024 * 1024; // 500MB
    auto res_high = isha::InstallFailureRecovery::stageAndUnpack(test_archive, test_dest, storage_high);
    assert(res_high.success);
    assert(!res_high.rolled_back);

    // Cleanup
    std::remove(test_dest.c_str());

    std::cout << "install_failure_recovery_benchmark PASSED!" << std::endl;
    return 0;
}
