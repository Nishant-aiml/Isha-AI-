#include "../core/inference/nnapi_backend.hpp"
#include <iostream>
#include <cassert>

using namespace isha;

void testCapabilityScoring() {
    NnapiBackend backend;
    DeviceProfile profile;
    profile.tier = DeviceTier::HIGH;
    profile.has_nnapi = true;

    auto cap = backend.probe(profile);
    assert(cap.nnapi_available);
    assert(backend.getCapabilityScoreCategory() == CapabilityScoreCategory::FULL_ACCELERATION);
    assert(backend.getPrefillScore() > backend.getDecodeScore()); // Prefill score higher than decode

    std::cout << "testCapabilityScoring passed." << std::endl;
}

int main() {
    testCapabilityScoring();
    std::cout << "All NNAPI Capability Score Benchmarks passed." << std::endl;
    return 0;
}
