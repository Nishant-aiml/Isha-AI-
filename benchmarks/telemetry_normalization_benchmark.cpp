#include "../core/inference/acceleration_probe.hpp"
#include "../core/inference/kv_telemetry.hpp"
#include <iostream>
#include <cassert>

using namespace isha;

void testNormalization() {
    // Large model vs Small model
    auto raw_eff_small = AccelerationProbe::calculateNormalizedEfficiency(
        15.0f, 1000.0f, 35.0f, 36.0f, 1.0f, 500, 64, 512, 1, 4
    );

    auto raw_eff_large = AccelerationProbe::calculateNormalizedEfficiency(
        15.0f, 1000.0f, 35.0f, 36.0f, 1.0f, 1500, 64, 512, 1, 4
    );

    assert(raw_eff_large.tokens_per_watt > raw_eff_small.tokens_per_watt);
    std::cout << "testNormalization passed." << std::endl;
}

void testRollingJitter() {
    KVTelemetry telemetry;
    telemetry.reset();

    for (int i = 0; i < 10; ++i) {
        telemetry.addJitterSample(10.0f + static_cast<float>(i));
    }

    float avg = telemetry.decode_jitter_ms.load(std::memory_order_relaxed);
    assert(avg > 0.0f);
    std::cout << "testRollingJitter passed." << std::endl;
}

int main() {
    testNormalization();
    testRollingJitter();
    std::cout << "All Telemetry Normalization Benchmarks passed." << std::endl;
    return 0;
}
