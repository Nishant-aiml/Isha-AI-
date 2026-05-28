#include <iostream>
#include <cassert>
#include "survival/background_resume_latency_policy.hpp"

using namespace isha;

int main() {
    std::cout << "Starting background_resume_latency_benchmark..." << std::endl;

    WakeLatencyLogs logs;
    logs.tokenizer_wake_ms = 100.0;
    logs.mmap_wake_ms = 150.0;
    logs.checkpoint_restore_ms = 200.0;
    logs.scheduler_resume_ms = 50.0;
    logs.acceleration_reactivate_ms = 300.0;

    auto mitigations = BackgroundResumeLatencyPolicy::evaluateResumeLatency(logs);
    assert(mitigations.latency_compliant);

    // Latency spikes past 2s limit trigger mitigation response
    logs.mmap_wake_ms = 1500.0;
    mitigations = BackgroundResumeLatencyPolicy::evaluateResumeLatency(logs);
    assert(!mitigations.latency_compliant);
    assert(mitigations.active_batch_cap == 2);
    assert(mitigations.restrict_acceleration);
    assert(mitigations.scale_down_checkpoint_writes);

    std::cout << "background_resume_latency_benchmark PASSED!" << std::endl;
    return 0;
}
