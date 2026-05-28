#include <iostream>
#include <cassert>
#include "survival/storage_recovery_assistant.hpp"

using namespace isha;

int main() {
    std::cout << "Starting storage_recovery_assistant_benchmark..." << std::endl;

    auto diags = StorageRecoveryAssistant::analyzeStorage(1.5, 0.4, 0.2);
    assert(diags.current_model_size_gb == 0.4);
    assert(diags.free_storage_gb == 1.5);
    assert(diags.recoverable_storage_gb > 0.0);

    SafeCleanupOptions options = {true, true, true, true, true};
    double reclaimed = StorageRecoveryAssistant::performSafeCleanup(options);
    assert(reclaimed > 0.5); // Ensure safe cleanup returns proper cumulative reclaimed size

    std::cout << "storage_recovery_assistant_benchmark PASSED!" << std::endl;
    return 0;
}
