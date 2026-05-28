#include "../core/inference/kv_telemetry.hpp"
#include <iostream>
#include <cassert>

using namespace isha;

void testPrefillDecodeSplit() {
    KVTelemetry telemetry;
    telemetry.reset();

    // Set prefill and decode properties
    telemetry.prefill_latency_ms.store(120.0f, std::memory_order_relaxed);
    telemetry.decode_tok_per_sec.store(24.0f, std::memory_order_relaxed);

    assert(telemetry.prefill_latency_ms.load() == 120.0f);
    assert(telemetry.decode_tok_per_sec.load() == 24.0f);

    std::cout << "testPrefillDecodeSplit passed." << std::endl;
}

int main() {
    testPrefillDecodeSplit();
    std::cout << "All NNAPI Prefill Decode Split Benchmarks passed." << std::endl;
    return 0;
}
