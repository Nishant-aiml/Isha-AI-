#include <iostream>
#include <cassert>
#include "survival/interruption_resilience_policy.hpp"

using namespace isha;

int main() {
    std::cout << "Starting interruption_resilience_policy_benchmark..." << std::endl;

    auto result = InterruptionResiliencePolicy::handleInterruption(InterruptionEvent::INCOMING_CALL);
    assert(result.interruption_handled);
    assert(result.pause_decode);
    assert(result.serialize_immediate_checkpoint);
    assert(result.scheduler_backoff_active);

    result = InterruptionResiliencePolicy::handleInterruption(InterruptionEvent::SCREEN_ROTATION);
    assert(result.interruption_handled);
    assert(!result.pause_decode);
    assert(result.serialize_immediate_checkpoint);

    std::cout << "interruption_resilience_policy_benchmark PASSED!" << std::endl;
    return 0;
}
