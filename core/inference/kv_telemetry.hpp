#ifndef ISHA_AI_KV_TELEMETRY_HPP
#define ISHA_AI_KV_TELEMETRY_HPP

#include <algorithm>
#include <atomic>
#include <cstdint>

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

    // Peak transient memory tracking (FIXATION 1)
    std::atomic<uint64_t> peak_rss_bytes{0};
    std::atomic<uint64_t> peak_commit_bytes{0};
    std::atomic<uint64_t> peak_mmap_residency_bytes{0};
    std::atomic<uint64_t> peak_prefill_bytes{0};
    std::atomic<uint64_t> peak_cancellation_cleanup_bytes{0};
    std::atomic<uint64_t> peak_scheduler_staging_bytes{0};

    // Phase-separated peak RSS (FIXATION 2)
    std::atomic<uint64_t> peak_cold_load_bytes{0};
    std::atomic<uint64_t> peak_mmap_activation_bytes{0};
    std::atomic<uint64_t> peak_tokenizer_init_bytes{0};
    std::atomic<uint64_t> peak_context_init_bytes{0};
    std::atomic<uint64_t> peak_prefill_phase_bytes{0};
    std::atomic<uint64_t> peak_decode_phase_bytes{0};
    std::atomic<uint64_t> peak_cancellation_phase_bytes{0};
    std::atomic<uint64_t> peak_unload_phase_bytes{0};
    std::atomic<uint64_t> peak_reload_overlap_bytes{0};

    // Phase timing breakdown (FIXATION 9)
    std::atomic<float> phase_mmap_init_ms{0.0f};
    std::atomic<float> phase_tokenizer_init_ms{0.0f};
    std::atomic<float> phase_context_init_ms{0.0f};
    std::atomic<float> phase_prefill_ms{0.0f};
    std::atomic<float> phase_decode_ms{0.0f};
    std::atomic<float> phase_unload_ms{0.0f};
    std::atomic<float> phase_cleanup_ms{0.0f};

    // Page-fault telemetry (FIXATION 6)
    std::atomic<uint64_t> hard_page_faults{0};
    std::atomic<uint64_t> soft_page_faults{0};

    // Prefill vs Decode split (FIXATION 4)
    std::atomic<float> prefill_latency_ms{0.0f};
    std::atomic<float> prefill_thermal_c{0.0f};
    std::atomic<uint64_t> prefill_page_faults{0};
    std::atomic<float> decode_tok_per_sec{0.0f};
    std::atomic<float> decode_jitter_ms{0.0f};
    std::atomic<float> decode_thermal_c{0.0f};
    std::atomic<uint64_t> decode_page_faults{0};

    // Prefill telemetry (FIXATION 6)
    std::atomic<uint32_t> prefill_chunk_count{0};
    std::atomic<float> prefill_total_ms{0.0f};
    std::atomic<float> decode_total_ms{0.0f};

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

        cleanup_latency_ms = 0.0f;
        fragmentation_ratio = 0.0f;
        reuse_ratio = 0.0f;
        cleanup_efficiency = 0.0f;
    }

    // Snapshot for telemetry export
    struct Snapshot {
        uint32_t current, peak, reserved, allocations, cleanups, stale;
        uint64_t ggml_graph, scratch, compute, batch;
        uint32_t tokenizer_loads, tokenizer_active, mmap_reuse, mmap_remap;
        uint64_t peak_rss, peak_commit, peak_mmap_residency, peak_prefill, peak_cancellation_cleanup, peak_scheduler_staging;
        
        // Expanded Phase-separated fields
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
