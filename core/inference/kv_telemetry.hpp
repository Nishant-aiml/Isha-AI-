#ifndef ISHA_AI_KV_TELEMETRY_HPP
#define ISHA_AI_KV_TELEMETRY_HPP

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <vector>
#include <mutex>

namespace isha {

struct KVTelemetry {
    std::atomic<uint32_t> current_cells{0};
    std::atomic<uint32_t> peak_cells{0};
    std::atomic<uint32_t> reserved_cells{0};
    std::atomic<uint32_t> allocation_count{0};
    std::atomic<uint32_t> cleanup_count{0};
    std::atomic<uint32_t> stale_retention_count{0};

    // Advanced Profiling Allocations
    std::atomic<uint64_t> ggml_graph_allocated_bytes{0};
    std::atomic<uint64_t> scratch_allocated_bytes{0};
    std::atomic<uint64_t> compute_allocated_bytes{0};
    std::atomic<uint64_t> batch_allocated_bytes{0};
    std::atomic<uint32_t> tokenizer_load_count{0};
    std::atomic<uint32_t> tokenizer_active_instances{0};
    std::atomic<uint32_t> mmap_reuse_count{0};
    std::atomic<uint32_t> mmap_remap_count{0};

    // Peak transient memory tracking
    std::atomic<uint64_t> peak_rss_bytes{0};
    std::atomic<uint64_t> peak_commit_bytes{0};
    std::atomic<uint64_t> peak_mmap_residency_bytes{0};
    std::atomic<uint64_t> peak_prefill_bytes{0};
    std::atomic<uint64_t> peak_cancellation_cleanup_bytes{0};
    std::atomic<uint64_t> peak_scheduler_staging_bytes{0};

    // Phase-separated peak RSS
    std::atomic<uint64_t> peak_cold_load_bytes{0};
    std::atomic<uint64_t> peak_mmap_activation_bytes{0};
    std::atomic<uint64_t> peak_tokenizer_init_bytes{0};
    std::atomic<uint64_t> peak_context_init_bytes{0};
    std::atomic<uint64_t> peak_prefill_phase_bytes{0};
    std::atomic<uint64_t> peak_decode_phase_bytes{0};
    std::atomic<uint64_t> peak_cancellation_phase_bytes{0};
    std::atomic<uint64_t> peak_unload_phase_bytes{0};
    std::atomic<uint64_t> peak_reload_overlap_bytes{0};

    // Phase timing breakdown
    std::atomic<float> phase_mmap_init_ms{0.0f};
    std::atomic<float> phase_tokenizer_init_ms{0.0f};
    std::atomic<float> phase_context_init_ms{0.0f};
    std::atomic<float> phase_prefill_ms{0.0f};
    std::atomic<float> phase_decode_ms{0.0f};
    std::atomic<float> phase_unload_ms{0.0f};
    std::atomic<float> phase_cleanup_ms{0.0f};

    // Page-fault telemetry
    std::atomic<uint64_t> hard_page_faults{0};
    std::atomic<uint64_t> soft_page_faults{0};

    // Prefill vs Decode split
    std::atomic<float> prefill_latency_ms{0.0f};
    std::atomic<float> prefill_thermal_c{0.0f};
    std::atomic<uint64_t> prefill_page_faults{0};
    std::atomic<float> decode_tok_per_sec{0.0f};
    std::atomic<float> decode_jitter_ms{0.0f};
    std::atomic<float> decode_thermal_c{0.0f};
    std::atomic<uint64_t> decode_page_faults{0};

    // Prefill telemetry
    std::atomic<uint32_t> prefill_chunk_count{0};
    std::atomic<float> prefill_total_ms{0.0f};
    std::atomic<float> decode_total_ms{0.0f};

    // Recovery Success Telemetry
    std::atomic<uint32_t> watchdog_trigger_count{0};
    std::atomic<uint32_t> backend_timeout_count{0};
    std::atomic<float> queue_drain_latency_ms{0.0f};
    std::atomic<uint32_t> teardown_success_count{0};
    std::atomic<uint32_t> backend_reset_success_count{0};
    std::atomic<uint32_t> fallback_success_count{0};
    std::atomic<uint32_t> recovery_success_count{0};
    std::atomic<uint32_t> unrecoverable_deadlock_count{0};
    std::atomic<uint32_t> process_restart_count{0};

    // Normalized efficiency
    std::atomic<float> normalized_tokens_per_watt{0.0f};

    // Memory Bandwidth Telemetry
    std::atomic<float> tensor_transfer_overhead_ms{0.0f};
    std::atomic<float> copy_latency_ms{0.0f};
    std::atomic<uint64_t> residency_migration_cost_bytes{0};
    std::atomic<uint32_t> queue_congestion_count{0};

    // ===== Phase 8B: Android Device Chaos Telemetry =====

    // Fixation 1: Adaptive partition overhead
    std::atomic<float> adaptive_partition_threshold{0.30f};   // dynamically tuned
    std::atomic<float> partition_overhead_pct{0.0f};          // measured overhead %
    std::atomic<uint32_t> adaptive_threshold_updates{0};      // threshold recalc count

    // Fixation 2: Thermal hysteresis
    std::atomic<float> thermal_decay_rate_c_per_min{0.0f};    // rate of thermal rise
    std::atomic<float> tok_per_s_at_thermal_throttle{0.0f};   // throughput at throttle
    std::atomic<float> thermal_stable_window_s{0.0f};         // stable temp duration before re-enable
    std::atomic<uint32_t> thermal_oscillation_events{0};      // oscillate suppress count
    std::atomic<bool> thermal_hysteresis_gate{false};         // NNAPI blocked until cleared

    // Fixation 3: LMK integrity checksums
    std::atomic<uint32_t> lmk_eviction_events{0};             // Android LMK eviction counter
    std::atomic<uint32_t> lmk_integrity_pass_count{0};        // post-LMK checks passed
    std::atomic<uint32_t> lmk_integrity_fail_count{0};        // post-LMK partial corruption detected

    // Fixation 4: Recovery quality drift
    std::atomic<float> fallback_latency_drift_ms{0.0f};       // drift from baseline fallback latency
    std::atomic<float> teardown_latency_drift_ms{0.0f};       // drift from baseline teardown latency
    std::atomic<float> reset_latency_drift_ms{0.0f};          // drift from baseline reset latency
    std::atomic<float> scheduler_recovery_latency_ms{0.0f};   // scheduler queue recovery latency
    std::atomic<float> cancellation_propagation_latency_ms{0.0f}; // cancel-to-acknowledge latency
    std::atomic<uint32_t> watchdog_escalation_frequency{0};   // escalations per soak window
    std::atomic<float> recovery_success_degradation{0.0f};    // success rate drop vs baseline

    // Fixation 5: Page-fault burst detection
    std::atomic<uint32_t> hard_fault_burst_count{0};          // burst events (clustered spikes)
    std::atomic<uint32_t> soft_fault_burst_count{0};          // burst events (clustered spikes)
    std::atomic<float> fault_burst_duration_ms{0.0f};         // duration of last burst
    std::atomic<float> burst_decode_jitter_ms{0.0f};          // decode jitter caused by burst
    std::atomic<float> burst_scheduler_jitter_ms{0.0f};       // scheduler jitter caused by burst
    std::atomic<uint64_t> burst_migration_spike_bytes{0};     // migration bytes during burst

    // Fixation 6: Thermal throughput half-life
    std::atomic<float> throughput_half_life_s{0.0f};          // seconds until tok/s halves
    std::atomic<float> initial_tok_per_s{0.0f};               // baseline at soak start
    std::atomic<float> current_tok_per_s{0.0f};               // live throughput reading

    // Fixation 7: First-token latency consistency
    std::atomic<float> first_token_p50_ms{0.0f};              // P50 first-token latency
    std::atomic<float> first_token_p95_ms{0.0f};              // P95 first-token latency
    std::atomic<float> cold_start_variance_ms{0.0f};          // variance across cold starts
    std::atomic<float> warm_start_variance_ms{0.0f};          // variance across warm starts
    std::atomic<float> graph_compile_variance_ms{0.0f};       // variance in graph compile times
    std::atomic<uint32_t> startup_latency_samples{0};         // samples collected

    // Fixation 8: Fallback storm tracking
    std::atomic<uint32_t> fallback_storm_count{0};            // storm cycles triggered
    std::atomic<float> fallback_storm_recovery_latency_ms{0.0f}; // avg recovery after storm
    std::atomic<uint32_t> acceleration_rejected_auto_count{0}; // auto-rejection triggers

    // Fixation 9: Device-tier memory collapse
    std::atomic<uint64_t> peak_staging_pressure_bytes{0};     // peak staging alloc under collapse
    std::atomic<uint32_t> tokenizer_reload_count{0};          // forced reloads under pressure
    std::atomic<float> allocator_churn_rate{0.0f};            // allocs/sec during collapse
    std::atomic<uint32_t> session_integrity_failures{0};      // session corrupted after collapse

    // Fixation 10: Efficiency decay curves
    std::atomic<float> battery_drain_pct_per_hour{0.0f};      // sustained drain rate
    std::atomic<float> scheduler_jitter_p99_ms{0.0f};         // P99 scheduler jitter
    std::atomic<float> lifecycle_interruption_latency_ms{0.0f}; // latency per lifecycle event
    std::atomic<uint32_t> lifecycle_interruption_count{0};    // background/foreground events
    std::atomic<float> thermal_adjusted_tok_per_watt{0.0f};   // tok/watt after thermal correction
    std::atomic<float> fallback_adjusted_tok_per_watt{0.0f};  // tok/watt after fallback overhead
    std::atomic<float> efficiency_collapse_rate{0.0f};        // tok/watt drop rate over soak
    std::atomic<float> efficiency_after_cooldown{0.0f};       // tok/watt recovery post-cooldown
    std::atomic<float> startup_cold_latency_ms{0.0f};         // full cold-start to first-token

    // Rolling window jitter
    float rolling_jitter_window[10] = {0.0f};
    int rolling_jitter_index = 0;
    mutable std::mutex jitter_mutex;

    // Rolling window for P99 scheduler jitter (16-sample)
    float scheduler_jitter_window[16] = {0.0f};
    int scheduler_jitter_index = 0;

    void addSchedulerJitterSample(float sample_ms) {
        std::lock_guard<std::mutex> lock(jitter_mutex);
        scheduler_jitter_window[scheduler_jitter_index] = sample_ms;
        scheduler_jitter_index = (scheduler_jitter_index + 1) % 16;
        // Approximate P99: max of window (conservative for 16 samples)
        float p99 = 0.0f;
        for (float v : scheduler_jitter_window) p99 = std::max(p99, v);
        scheduler_jitter_p99_ms.store(p99, std::memory_order_relaxed);
    }

    // First-token latency reservoir for P50/P95
    float first_token_samples[64] = {0.0f};
    int first_token_sample_idx = 0;
    int first_token_sample_count = 0;

    void addFirstTokenSample(float latency_ms) {
        std::lock_guard<std::mutex> lock(jitter_mutex);
        first_token_samples[first_token_sample_idx] = latency_ms;
        first_token_sample_idx = (first_token_sample_idx + 1) % 64;
        if (first_token_sample_count < 64) first_token_sample_count++;
        startup_latency_samples.fetch_add(1, std::memory_order_relaxed);

        // Compute P50 and P95 from filled portion
        int n = first_token_sample_count;
        float sorted[64];
        std::copy(first_token_samples, first_token_samples + n, sorted);
        std::sort(sorted, sorted + n);
        first_token_p50_ms.store(sorted[n / 2], std::memory_order_relaxed);
        int p95_idx = static_cast<int>(n * 0.95f);
        if (p95_idx >= n) p95_idx = n - 1;
        first_token_p95_ms.store(sorted[p95_idx], std::memory_order_relaxed);
    }

    void addJitterSample(float sample_ms) {
        std::lock_guard<std::mutex> lock(jitter_mutex);
        rolling_jitter_window[rolling_jitter_index] = sample_ms;
        rolling_jitter_index = (rolling_jitter_index + 1) % 10;
        
        float sum = 0.0f;
        for (float val : rolling_jitter_window) {
            sum += val;
        }
        decode_jitter_ms.store(sum / 10.0f, std::memory_order_relaxed);
    }

    enum class Phase {
        ColdLoad,
        MmapActivation,
        TokenizerInit,
        ContextInit,
        Prefill,
        Decode,
        Cancellation,
        Unload,
        ReloadOverlap
    };

    void recordPeak(std::atomic<uint64_t>& peak_var, uint64_t bytes) {
        uint64_t prev = peak_var.load(std::memory_order_relaxed);
        while (bytes > prev) {
            if (peak_var.compare_exchange_weak(prev, bytes, std::memory_order_relaxed)) break;
        }
    }

    void recordPhasePeak(Phase phase, uint64_t bytes) {
        switch (phase) {
            case Phase::ColdLoad: recordPeak(peak_cold_load_bytes, bytes); break;
            case Phase::MmapActivation: recordPeak(peak_mmap_activation_bytes, bytes); break;
            case Phase::TokenizerInit: recordPeak(peak_tokenizer_init_bytes, bytes); break;
            case Phase::ContextInit: recordPeak(peak_context_init_bytes, bytes); break;
            case Phase::Prefill: recordPeak(peak_prefill_phase_bytes, bytes); break;
            case Phase::Decode: recordPeak(peak_decode_phase_bytes, bytes); break;
            case Phase::Cancellation: recordPeak(peak_cancellation_phase_bytes, bytes); break;
            case Phase::Unload: recordPeak(peak_unload_phase_bytes, bytes); break;
            case Phase::ReloadOverlap: recordPeak(peak_reload_overlap_bytes, bytes); break;
        }
    }

    void recordPrefillChunk(float chunk_ms, uint32_t chunk_tokens) {
        prefill_chunk_count.fetch_add(1, std::memory_order_relaxed);
        float current = prefill_total_ms.load(std::memory_order_relaxed);
        prefill_total_ms.store(current + chunk_ms, std::memory_order_relaxed);
    }

    void recordPeakRSS(uint64_t rss_bytes) {
        recordPeak(peak_rss_bytes, rss_bytes);
    }

    void recordPeakPrefill(uint64_t bytes) {
        recordPeak(peak_prefill_bytes, bytes);
    }

    float cleanup_latency_ms = 0.0f;
    float fragmentation_ratio = 0.0f;   // (reserved - active) / reserved
    float reuse_ratio = 0.0f;           // reused / total allocated
    float cleanup_efficiency = 0.0f;    // freed / attempted

    void recordAllocation(uint32_t cells) {
        allocation_count.fetch_add(1, std::memory_order_relaxed);
        uint32_t new_current = current_cells.fetch_add(cells, std::memory_order_relaxed) + cells;

        // Update peak if new maximum
        uint32_t prev_peak = peak_cells.load(std::memory_order_relaxed);
        while (new_current > prev_peak) {
            if (peak_cells.compare_exchange_weak(prev_peak, new_current, 
                                                   std::memory_order_relaxed)) {
                break;
            }
        }
    }

    void recordCleanup(uint32_t freed, uint32_t attempted, float latency_ms) {
        cleanup_count.fetch_add(1, std::memory_order_relaxed);
        current_cells.fetch_sub(std::min(freed, current_cells.load(std::memory_order_relaxed)), 
                                 std::memory_order_relaxed);
        cleanup_latency_ms = latency_ms;
        cleanup_efficiency = (attempted > 0) 
                             ? static_cast<float>(freed) / static_cast<float>(attempted) 
                             : 0.0f;
    }

    void recordStaleRetention() {
        stale_retention_count.fetch_add(1, std::memory_order_relaxed);
    }

    void updateFragmentation() {
        uint32_t res = reserved_cells.load(std::memory_order_relaxed);
        uint32_t cur = current_cells.load(std::memory_order_relaxed);
        uint32_t denom = std::max(res, 1u);
        fragmentation_ratio = (res >= cur) 
                              ? static_cast<float>(res - cur) / static_cast<float>(denom) 
                              : 0.0f;
    }

    void reset() {
        current_cells.store(0, std::memory_order_relaxed);
        peak_cells.store(0, std::memory_order_relaxed);
        reserved_cells.store(0, std::memory_order_relaxed);
        allocation_count.store(0, std::memory_order_relaxed);
        cleanup_count.store(0, std::memory_order_relaxed);
        stale_retention_count.store(0, std::memory_order_relaxed);
        ggml_graph_allocated_bytes.store(0, std::memory_order_relaxed);
        scratch_allocated_bytes.store(0, std::memory_order_relaxed);
        compute_allocated_bytes.store(0, std::memory_order_relaxed);
        batch_allocated_bytes.store(0, std::memory_order_relaxed);
        tokenizer_load_count.store(0, std::memory_order_relaxed);
        tokenizer_active_instances.store(0, std::memory_order_relaxed);
        mmap_reuse_count.store(0, std::memory_order_relaxed);
        mmap_remap_count.store(0, std::memory_order_relaxed);
        peak_rss_bytes.store(0, std::memory_order_relaxed);
        peak_commit_bytes.store(0, std::memory_order_relaxed);
        peak_mmap_residency_bytes.store(0, std::memory_order_relaxed);
        peak_prefill_bytes.store(0, std::memory_order_relaxed);
        peak_cancellation_cleanup_bytes.store(0, std::memory_order_relaxed);
        peak_scheduler_staging_bytes.store(0, std::memory_order_relaxed);

        peak_cold_load_bytes.store(0, std::memory_order_relaxed);
        peak_mmap_activation_bytes.store(0, std::memory_order_relaxed);
        peak_tokenizer_init_bytes.store(0, std::memory_order_relaxed);
        peak_context_init_bytes.store(0, std::memory_order_relaxed);
        peak_prefill_phase_bytes.store(0, std::memory_order_relaxed);
        peak_decode_phase_bytes.store(0, std::memory_order_relaxed);
        peak_cancellation_phase_bytes.store(0, std::memory_order_relaxed);
        peak_unload_phase_bytes.store(0, std::memory_order_relaxed);
        peak_reload_overlap_bytes.store(0, std::memory_order_relaxed);

        phase_mmap_init_ms.store(0.0f, std::memory_order_relaxed);
        phase_tokenizer_init_ms.store(0.0f, std::memory_order_relaxed);
        phase_context_init_ms.store(0.0f, std::memory_order_relaxed);
        phase_prefill_ms.store(0.0f, std::memory_order_relaxed);
        phase_decode_ms.store(0.0f, std::memory_order_relaxed);
        phase_unload_ms.store(0.0f, std::memory_order_relaxed);
        phase_cleanup_ms.store(0.0f, std::memory_order_relaxed);

        hard_page_faults.store(0, std::memory_order_relaxed);
        soft_page_faults.store(0, std::memory_order_relaxed);

        prefill_latency_ms.store(0.0f, std::memory_order_relaxed);
        prefill_thermal_c.store(0.0f, std::memory_order_relaxed);
        prefill_page_faults.store(0, std::memory_order_relaxed);
        decode_tok_per_sec.store(0.0f, std::memory_order_relaxed);
        decode_jitter_ms.store(0.0f, std::memory_order_relaxed);
        decode_thermal_c.store(0.0f, std::memory_order_relaxed);
        decode_page_faults.store(0, std::memory_order_relaxed);

        prefill_chunk_count.store(0, std::memory_order_relaxed);
        prefill_total_ms.store(0.0f, std::memory_order_relaxed);
        decode_total_ms.store(0.0f, std::memory_order_relaxed);

        watchdog_trigger_count.store(0, std::memory_order_relaxed);
        backend_timeout_count.store(0, std::memory_order_relaxed);
        queue_drain_latency_ms.store(0.0f, std::memory_order_relaxed);
        teardown_success_count.store(0, std::memory_order_relaxed);
        backend_reset_success_count.store(0, std::memory_order_relaxed);
        fallback_success_count.store(0, std::memory_order_relaxed);
        recovery_success_count.store(0, std::memory_order_relaxed);
        unrecoverable_deadlock_count.store(0, std::memory_order_relaxed);
        process_restart_count.store(0, std::memory_order_relaxed);

        normalized_tokens_per_watt.store(0.0f, std::memory_order_relaxed);

        tensor_transfer_overhead_ms.store(0.0f, std::memory_order_relaxed);
        copy_latency_ms.store(0.0f, std::memory_order_relaxed);
        residency_migration_cost_bytes.store(0, std::memory_order_relaxed);
        queue_congestion_count.store(0, std::memory_order_relaxed);

        // Phase 8B resets
        adaptive_partition_threshold.store(0.30f, std::memory_order_relaxed);
        partition_overhead_pct.store(0.0f, std::memory_order_relaxed);
        adaptive_threshold_updates.store(0, std::memory_order_relaxed);
        thermal_decay_rate_c_per_min.store(0.0f, std::memory_order_relaxed);
        tok_per_s_at_thermal_throttle.store(0.0f, std::memory_order_relaxed);
        thermal_stable_window_s.store(0.0f, std::memory_order_relaxed);
        thermal_oscillation_events.store(0, std::memory_order_relaxed);
        thermal_hysteresis_gate.store(false, std::memory_order_relaxed);
        lmk_eviction_events.store(0, std::memory_order_relaxed);
        lmk_integrity_pass_count.store(0, std::memory_order_relaxed);
        lmk_integrity_fail_count.store(0, std::memory_order_relaxed);
        fallback_latency_drift_ms.store(0.0f, std::memory_order_relaxed);
        teardown_latency_drift_ms.store(0.0f, std::memory_order_relaxed);
        reset_latency_drift_ms.store(0.0f, std::memory_order_relaxed);
        scheduler_recovery_latency_ms.store(0.0f, std::memory_order_relaxed);
        cancellation_propagation_latency_ms.store(0.0f, std::memory_order_relaxed);
        watchdog_escalation_frequency.store(0, std::memory_order_relaxed);
        recovery_success_degradation.store(0.0f, std::memory_order_relaxed);
        hard_fault_burst_count.store(0, std::memory_order_relaxed);
        soft_fault_burst_count.store(0, std::memory_order_relaxed);
        fault_burst_duration_ms.store(0.0f, std::memory_order_relaxed);
        burst_decode_jitter_ms.store(0.0f, std::memory_order_relaxed);
        burst_scheduler_jitter_ms.store(0.0f, std::memory_order_relaxed);
        burst_migration_spike_bytes.store(0, std::memory_order_relaxed);
        throughput_half_life_s.store(0.0f, std::memory_order_relaxed);
        initial_tok_per_s.store(0.0f, std::memory_order_relaxed);
        current_tok_per_s.store(0.0f, std::memory_order_relaxed);
        first_token_p50_ms.store(0.0f, std::memory_order_relaxed);
        first_token_p95_ms.store(0.0f, std::memory_order_relaxed);
        cold_start_variance_ms.store(0.0f, std::memory_order_relaxed);
        warm_start_variance_ms.store(0.0f, std::memory_order_relaxed);
        graph_compile_variance_ms.store(0.0f, std::memory_order_relaxed);
        startup_latency_samples.store(0, std::memory_order_relaxed);
        fallback_storm_count.store(0, std::memory_order_relaxed);
        fallback_storm_recovery_latency_ms.store(0.0f, std::memory_order_relaxed);
        acceleration_rejected_auto_count.store(0, std::memory_order_relaxed);
        peak_staging_pressure_bytes.store(0, std::memory_order_relaxed);
        tokenizer_reload_count.store(0, std::memory_order_relaxed);
        allocator_churn_rate.store(0.0f, std::memory_order_relaxed);
        session_integrity_failures.store(0, std::memory_order_relaxed);
        battery_drain_pct_per_hour.store(0.0f, std::memory_order_relaxed);
        scheduler_jitter_p99_ms.store(0.0f, std::memory_order_relaxed);
        lifecycle_interruption_latency_ms.store(0.0f, std::memory_order_relaxed);
        lifecycle_interruption_count.store(0, std::memory_order_relaxed);
        thermal_adjusted_tok_per_watt.store(0.0f, std::memory_order_relaxed);
        fallback_adjusted_tok_per_watt.store(0.0f, std::memory_order_relaxed);
        efficiency_collapse_rate.store(0.0f, std::memory_order_relaxed);
        efficiency_after_cooldown.store(0.0f, std::memory_order_relaxed);
        startup_cold_latency_ms.store(0.0f, std::memory_order_relaxed);
        first_token_sample_idx = 0;
        first_token_sample_count = 0;
        scheduler_jitter_index = 0;
        for (float& v : first_token_samples) v = 0.0f;
        for (float& v : scheduler_jitter_window) v = 0.0f;

        cleanup_latency_ms = 0.0f;
        fragmentation_ratio = 0.0f;
        reuse_ratio = 0.0f;
        cleanup_efficiency = 0.0f;
    }

    struct Snapshot {
        uint32_t current, peak, reserved, allocations, cleanups, stale;
        uint64_t ggml_graph, scratch, compute, batch;
        uint32_t tokenizer_loads, tokenizer_active, mmap_reuse, mmap_remap;
        uint64_t peak_rss, peak_commit, peak_mmap_residency, peak_prefill, peak_cancellation_cleanup, peak_scheduler_staging;
        uint64_t peak_cold_load, peak_mmap_activation, peak_tokenizer_init, peak_context_init;
        uint64_t peak_prefill_phase, peak_decode_phase, peak_cancellation_phase, peak_unload_phase, peak_reload_overlap;
        float phase_mmap_init, phase_tokenizer_init, phase_context_init, phase_prefill, phase_decode, phase_unload, phase_cleanup;
        uint64_t hard_faults, soft_faults;
        float prefill_lat, prefill_therm;
        uint64_t prefill_faults;
        float decode_tps, decode_jitter, decode_therm;
        uint64_t decode_faults;
        float cleanup_latency, fragmentation, reuse, efficiency;
    };

    Snapshot snapshot() const {
        return {
            current_cells.load(std::memory_order_relaxed),
            peak_cells.load(std::memory_order_relaxed),
            reserved_cells.load(std::memory_order_relaxed),
            allocation_count.load(std::memory_order_relaxed),
            cleanup_count.load(std::memory_order_relaxed),
            stale_retention_count.load(std::memory_order_relaxed),
            ggml_graph_allocated_bytes.load(std::memory_order_relaxed),
            scratch_allocated_bytes.load(std::memory_order_relaxed),
            compute_allocated_bytes.load(std::memory_order_relaxed),
            batch_allocated_bytes.load(std::memory_order_relaxed),
            tokenizer_load_count.load(std::memory_order_relaxed),
            tokenizer_active_instances.load(std::memory_order_relaxed),
            mmap_reuse_count.load(std::memory_order_relaxed),
            mmap_remap_count.load(std::memory_order_relaxed),
            peak_rss_bytes.load(std::memory_order_relaxed),
            peak_commit_bytes.load(std::memory_order_relaxed),
            peak_mmap_residency_bytes.load(std::memory_order_relaxed),
            peak_prefill_bytes.load(std::memory_order_relaxed),
            peak_cancellation_cleanup_bytes.load(std::memory_order_relaxed),
            peak_scheduler_staging_bytes.load(std::memory_order_relaxed),
            peak_cold_load_bytes.load(std::memory_order_relaxed),
            peak_mmap_activation_bytes.load(std::memory_order_relaxed),
            peak_tokenizer_init_bytes.load(std::memory_order_relaxed),
            peak_context_init_bytes.load(std::memory_order_relaxed),
            peak_prefill_phase_bytes.load(std::memory_order_relaxed),
            peak_decode_phase_bytes.load(std::memory_order_relaxed),
            peak_cancellation_phase_bytes.load(std::memory_order_relaxed),
            peak_unload_phase_bytes.load(std::memory_order_relaxed),
            peak_reload_overlap_bytes.load(std::memory_order_relaxed),
            phase_mmap_init_ms.load(std::memory_order_relaxed),
            phase_tokenizer_init_ms.load(std::memory_order_relaxed),
            phase_context_init_ms.load(std::memory_order_relaxed),
            phase_prefill_ms.load(std::memory_order_relaxed),
            phase_decode_ms.load(std::memory_order_relaxed),
            phase_unload_ms.load(std::memory_order_relaxed),
            phase_cleanup_ms.load(std::memory_order_relaxed),
            hard_page_faults.load(std::memory_order_relaxed),
            soft_page_faults.load(std::memory_order_relaxed),
            prefill_latency_ms.load(std::memory_order_relaxed),
            prefill_thermal_c.load(std::memory_order_relaxed),
            prefill_page_faults.load(std::memory_order_relaxed),
            decode_tok_per_sec.load(std::memory_order_relaxed),
            decode_jitter_ms.load(std::memory_order_relaxed),
            decode_thermal_c.load(std::memory_order_relaxed),
            decode_page_faults.load(std::memory_order_relaxed),
            cleanup_latency_ms,
            fragmentation_ratio,
            reuse_ratio,
            cleanup_efficiency
        };
    }
};

} // namespace isha

#endif // ISHA_AI_KV_TELEMETRY_HPP
