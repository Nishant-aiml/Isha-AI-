#include "../core/inference/acceleration_quarantine.hpp"
#include "../core/inference/driver_tracker.hpp"
#include "../core/inference/capability_matrix.hpp"
#include <iostream>
#include <cassert>

using namespace isha;

void testAdaptiveSeverity() {
    auto& quarantine = AccelerationQuarantine::getInstance();
    std::string sig = "Qualcomm|512.0.0|1.1.0|1.3.0|SM8450|OxygenOS_12|12|5.10.43|1.0.0";
    
    quarantine.resetReputation(sig);
    assert(quarantine.getReputation(sig) == 1.0f);
    assert(!quarantine.isQuarantined(sig));

    // Minor timeout
    quarantine.reportFailure(sig, FailureSeverity::MINOR_TIMEOUT);
    assert(quarantine.getReputation(sig) == 0.9f);
    assert(!quarantine.isQuarantined(sig));

    // Initialization corruption (Severe)
    quarantine.reportFailure(sig, FailureSeverity::INIT_CORRUPTION);
    assert(quarantine.getReputation(sig) == 0.0f); // severe penalty
    assert(quarantine.isQuarantined(sig));
    
    std::cout << "testAdaptiveSeverity passed." << std::endl;
}

void testDecay() {
    auto& quarantine = AccelerationQuarantine::getInstance();
    std::string sig = "Qualcomm|512.0.0|1.1.0|1.3.0|SM8450|OxygenOS_12|12|5.10.43|1.0.0";
    
    quarantine.resetReputation(sig);
    quarantine.reportFailure(sig, FailureSeverity::MINOR_TIMEOUT);
    float rep1 = quarantine.getReputation(sig);
    
    quarantine.reportSuccess(sig);
    float rep2 = quarantine.getReputation(sig);
    assert(rep2 > rep1);
    
    std::cout << "testDecay passed." << std::endl;
}

int main() {
    testAdaptiveSeverity();
    testDecay();
    std::cout << "All Quarantine Adaptive Benchmarks passed." << std::endl;
    return 0;
}
