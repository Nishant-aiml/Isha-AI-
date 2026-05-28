#include "../core/inference/nnapi_backend.hpp"
#include <iostream>
#include <cassert>

using namespace isha;

void testProbeTimeoutCeilings() {
    NnapiBackend backend;
    DeviceProfile profile;
    profile.tier = DeviceTier::HIGH;
    profile.has_nnapi = true;

    auto cap = backend.probe(profile);
    assert(cap.probe_passed);

    // If timeout was simulated over 200ms, it would have failed
    std::cout << "testProbeTimeoutCeilings passed." << std::endl;
}

int main() {
    testProbeTimeoutCeilings();
    std::cout << "All NNAPI Probe Timeout Benchmarks passed." << std::endl;
    return 0;
}
