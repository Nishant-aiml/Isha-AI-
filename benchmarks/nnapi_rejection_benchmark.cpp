#include "../core/inference/nnapi_backend.hpp"
#include <iostream>
#include <cassert>

using namespace isha;

void testNnapiRejection() {
    NnapiBackend backend;
    DeviceProfile profile;
    profile.tier = DeviceTier::LOW;
    profile.has_nnapi = false; // incompatible device profile

    auto cap = backend.probe(profile);
    assert(!cap.probe_passed);
    assert(backend.getCapabilityScoreCategory() == CapabilityScoreCategory::ACCELERATION_REJECTED);

    std::cout << "testNnapiRejection passed." << std::endl;
}

int main() {
    testNnapiRejection();
    std::cout << "All NNAPI Rejection Benchmarks passed." << std::endl;
    return 0;
}
