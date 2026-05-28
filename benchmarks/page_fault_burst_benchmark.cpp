// Phase 8B — Fixation 5: Page-Fault Burst Detection
// Tracks clustered hard/soft fault bursts, burst duration, burst-triggered
// decode jitter, burst-triggered scheduler jitter, and migration spikes.
// Critical on 4GB devices where residency pressure creates catastrophic bursts.

#include "../core/inference/kv_telemetry.hpp"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace isha;

// Simulate a time-series of fault events. Bursts = clusters of high-fault windows.
// A "burst" is defined as >= BURST_THRESHOLD faults within a BURST_WINDOW_MS window.
struct FaultEvent {
    float timestamp_ms;
    uint32_t hard_faults;
    uint32_t soft_faults;
    float migration_bytes;
};

// Detect bursts: returns burst count and max burst severity
static int detectBursts(
    const std::vector<FaultEvent>& events,
    float burst_window_ms,
    uint32_t burst_threshold,
    float& out_max_hard_burst,
    float& out_max_soft_burst,
    float& out_max_burst_duration_ms)
{
    int burst_count = 0;
    out_max_hard_burst = 0.0f;
    out_max_soft_burst = 0.0f;
    out_max_burst_duration_ms = 0.0f;

    for (size_t i = 0; i < events.size(); ++i) {
        // Slide window forward from event[i]
        uint32_t window_hard = 0, window_soft = 0;
        float window_start = events[i].timestamp_ms;
        float window_end   = window_start;

        for (size_t j = i; j < events.size() && events[j].timestamp_ms - window_start <= burst_window_ms; ++j) {
            window_hard += events[j].hard_faults;
            window_soft += events[j].soft_faults;
            window_end   = events[j].timestamp_ms;
        }

        if (window_hard >= burst_threshold || window_soft >= burst_threshold * 4) {
            burst_count++;
            out_max_hard_burst = std::max(out_max_hard_burst, static_cast<float>(window_hard));
            out_max_soft_burst = std::max(out_max_soft_burst, static_cast<float>(window_soft));
            out_max_burst_duration_ms = std::max(out_max_burst_duration_ms, window_end - window_start);
        }
    }
    return burst_count;
}

int main() {
    printf("[page_fault_burst_benchmark] Starting (4GB device simulation)...\n");

    KVTelemetry tel;
    tel.reset();

    // --- Build fault time series ---
    // Normal operation: ~2 hard faults / 100ms, ~20 soft faults / 100ms
    // Burst events at t=500ms, t=1500ms, t=3000ms
    std::vector<FaultEvent> events;

    for (int t = 0; t < 50; ++t) { // 5 seconds in 100ms steps
        FaultEvent ev;
        ev.timestamp_ms   = static_cast<float>(t * 100);
        ev.hard_faults    = 2;
        ev.soft_faults    = 20;
        ev.migration_bytes = 4096.0f * 2.0f; // 2 pages

        // Inject burst at t=500ms: 40 hard faults in one window
        if (t >= 5 && t <= 7) {
            ev.hard_faults  = 20;
            ev.soft_faults  = 120;
            ev.migration_bytes = 4096.0f * 80.0f;
        }
        // Inject burst at t=1500ms
        if (t >= 15 && t <= 17) {
            ev.hard_faults  = 18;
            ev.soft_faults  = 100;
            ev.migration_bytes = 4096.0f * 60.0f;
        }
        // Inject burst at t=3000ms
        if (t >= 30 && t <= 33) {
            ev.hard_faults  = 22;
            ev.soft_faults  = 140;
            ev.migration_bytes = 4096.0f * 90.0f;
        }
        events.push_back(ev);

        tel.hard_page_faults.fetch_add(ev.hard_faults, std::memory_order_relaxed);
        tel.soft_page_faults.fetch_add(ev.soft_faults, std::memory_order_relaxed);
    }

    // --- Burst detection ---
    float max_hard = 0.0f, max_soft = 0.0f, max_duration = 0.0f;
    int burst_count = detectBursts(events, 300.0f /*ms window*/, 30 /*threshold*/, 
                                   max_hard, max_soft, max_duration);

    printf("  Total hard faults: %llu\n", (unsigned long long)tel.hard_page_faults.load());
    printf("  Total soft faults: %llu\n", (unsigned long long)tel.soft_page_faults.load());
    printf("  Detected bursts:   %d\n", burst_count);
    printf("  Max hard/burst:    %.0f\n", max_hard);
    printf("  Max soft/burst:    %.0f\n", max_soft);
    printf("  Max burst duration:%.1fms\n", max_duration);

    // Update burst telemetry
    tel.hard_fault_burst_count.store(static_cast<uint32_t>(burst_count), std::memory_order_relaxed);
    tel.soft_fault_burst_count.store(static_cast<uint32_t>(burst_count), std::memory_order_relaxed);
    tel.fault_burst_duration_ms.store(max_duration, std::memory_order_relaxed);

    // Simulate burst-induced jitter (bursts push decode and scheduler off schedule)
    float burst_decode_jitter  = max_duration * 0.15f; // 15% of burst duration leaks into decode jitter
    float burst_sched_jitter   = max_duration * 0.08f;
    uint64_t burst_migration   = static_cast<uint64_t>(max_hard * 4096ULL * 10ULL);

    tel.burst_decode_jitter_ms.store(burst_decode_jitter, std::memory_order_relaxed);
    tel.burst_scheduler_jitter_ms.store(burst_sched_jitter, std::memory_order_relaxed);
    tel.burst_migration_spike_bytes.store(burst_migration, std::memory_order_relaxed);

    printf("  Burst decode jitter:    %.2fms\n", burst_decode_jitter);
    printf("  Burst scheduler jitter: %.2fms\n", burst_sched_jitter);
    printf("  Burst migration spike:  %llu bytes\n", (unsigned long long)burst_migration);

    // --- Assertions ---
    assert(burst_count >= 2 && "At least 2 bursts must be detected from injected events");
    assert(tel.hard_fault_burst_count.load() >= 2);
    assert(tel.fault_burst_duration_ms.load() > 0.0f);
    assert(tel.burst_decode_jitter_ms.load() > 0.0f);
    assert(tel.burst_scheduler_jitter_ms.load() > 0.0f);
    assert(tel.burst_migration_spike_bytes.load() > 0ULL);

    // Bound check: burst duration must remain below catastrophic threshold (1s)
    assert(max_duration < 1000.0f && "Burst duration exceeds 1s — catastrophic residency instability");

    printf("\n[page_fault_burst_benchmark] PASS\n");
    return 0;
}
