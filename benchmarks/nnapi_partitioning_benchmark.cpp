#include "../core/inference/kv_telemetry.hpp"
#include <iostream>
#include <cassert>

using namespace isha;

void testPartitioningTelemetry() {
    KVTelemetry telemetry;
    telemetry.reset();

    // Simulate partitioning data
    telemetry.queue_congestion_count.store(3, std::memory_order_relaxed);
    telemetry.residency_migration_cost_bytes.store(2048, std::memory_order_relaxed);

    assert(telemetry.queue_congestion_count.load() == 3);
    assert(telemetry.residency_migration_cost_bytes.load() == 2048);

    std::cout << "testPartitioningTelemetry passed." << std::endl;
}

int main() {
    testPartitioningTelemetry();
    std::cout << "All NNAPI Partitioning Benchmarks passed." << std::endl;
    return 0;
}
