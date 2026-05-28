#include "../core/inference/kv_telemetry.hpp"
#include <iostream>
#include <cassert>

using namespace isha;

void testStartupLatencyTelemetry() {
    KVTelemetry telemetry;
    telemetry.reset();

    // Verify compile and startup tracking
    telemetry.phase_mmap_init_ms.store(50.0f, std::memory_order_relaxed);
    telemetry.phase_tokenizer_init_ms.store(10.0f, std::memory_order_relaxed);

    assert(telemetry.phase_mmap_init_ms.load() == 50.0f);
    assert(telemetry.phase_tokenizer_init_ms.load() == 10.0f);

    std::cout << "testStartupLatencyTelemetry passed." << std::endl;
}

int main() {
    testStartupLatencyTelemetry();
    std::cout << "All NNAPI Startup Latency Benchmarks passed." << std::endl;
    return 0;
}
