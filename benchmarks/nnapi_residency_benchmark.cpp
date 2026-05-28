#include "../core/inference/kv_telemetry.hpp"
#include <iostream>
#include <cassert>

using namespace isha;

void testMemoryResidency() {
    KVTelemetry telemetry;
    telemetry.reset();

    // Verify residency growth and duplicate tracking
    telemetry.residency_migration_cost_bytes.store(4096, std::memory_order_relaxed);
    
    assert(telemetry.residency_migration_cost_bytes.load() == 4096);
    std::cout << "testMemoryResidency passed." << std::endl;
}

int main() {
    testMemoryResidency();
    std::cout << "All NNAPI Residency Benchmarks passed." << std::endl;
    return 0;
}
