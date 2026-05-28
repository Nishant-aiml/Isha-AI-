#include <iostream>
#include <cassert>
#include "survival/ui_starvation_guard.hpp"

using namespace isha;

int main() {
    std::cout << "Starting ui_starvation_guard_benchmark..." << std::endl;

    StarvationMetrics metrics;
    metrics.frame_stall_ms = 12.0;
    metrics.excessive_jni_contention = false;
    metrics.ui_scheduler_starved = false;
    metrics.decode_monopolized = false;
    metrics.touch_response_delay_ms = 15.0;
    metrics.render_loop_stall_ms = 8.0;

    auto action = UIStarvationGuard::evaluateStarvation(metrics);
    assert(!action.starvation_threat_active);

    // Frame stall above 100ms triggers UI protection guard
    metrics.frame_stall_ms = 120.0;
    action = UIStarvationGuard::evaluateStarvation(metrics);
    assert(action.starvation_threat_active);
    assert(action.capped_batch_size == 2);
    assert(action.pause_background_work);

    std::cout << "ui_starvation_guard_benchmark PASSED!" << std::endl;
    return 0;
}
