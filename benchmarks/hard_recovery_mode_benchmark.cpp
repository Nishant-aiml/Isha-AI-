#include <iostream>
#include <cassert>
#include "survival/hard_recovery_mode.hpp"

using namespace isha;

int main() {
    std::cout << "Starting hard_recovery_mode_benchmark..." << std::endl;

    HardFailureCounters counters = {0, 0, 0, 0, 0};
    auto config = HardRecoveryMode::evaluateFailures(counters);
    assert(!config.hard_recovery_active);

    // Repeated safe mode failures triggers loop containment
    counters.safemode_failures = 5;
    config = HardRecoveryMode::evaluateFailures(counters);
    assert(config.hard_recovery_active);
    assert(config.force_cpu_only);
    assert(config.force_cache_purge);
    assert(config.isolate_corrupt_model);

    std::cout << "hard_recovery_mode_benchmark PASSED!" << std::endl;
    return 0;
}
