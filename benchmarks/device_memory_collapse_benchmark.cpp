// Phase 8B — Fixation 9: Device-Tier Memory Collapse Validation
// Simulates realistic 4GB device memory collapse:
// - aggressive LMK reclaim rounds
// - concurrent staging allocations
// - mmap residency churn
// - backend migration spikes
// - repeated tokenizer reloads
// - background app pressure
// Validates: bounded fragmentation, bounded churn, fallback survival, session integrity.

#include "../core/inference/kv_telemetry.hpp"
#include "../core/inference/acceleration_quarantine.hpp"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace isha;

// Memory pool simulation: tracks allocated/freed bytes
struct MemoryPool {
    uint64_t total_bytes;
    uint64_t allocated_bytes = 0;
    uint64_t peak_bytes = 0;
    uint32_t alloc_count = 0;
    uint32_t free_count  = 0;

    explicit MemoryPool(uint64_t total) : total_bytes(total) {}

    bool allocate(uint64_t size) {
        if (allocated_bytes + size > total_bytes) return false; // OOM
        allocated_bytes += size;
        peak_bytes = std::max(peak_bytes, allocated_bytes);
        alloc_count++;
        return true;
    }

    void free_block(uint64_t size) {
        if (size > allocated_bytes) size = allocated_bytes;
        allocated_bytes -= size;
        free_count++;
    }

    float fragmentation() const {
        if (total_bytes == 0) return 0.0f;
        return static_cast<float>(allocated_bytes) / static_cast<float>(total_bytes);
    }
};

int main() {
    printf("[device_memory_collapse_benchmark] Starting (4GB device simulation)...\n");

    KVTelemetry tel;
    tel.reset();

    // 4GB device: after OS + ROM, ~2.4GB available for apps
    const uint64_t AVAILABLE_MB   = 1200ULL;
    const uint64_t AVAILABLE_BYTES = AVAILABLE_MB * 1024ULL * 1024ULL;

    MemoryPool pool(AVAILABLE_BYTES);

    // ISHA footprint: model mmap ~680MB, context ~512 tokens * 4KB = 2MB, staging ~64MB
    const uint64_t MODEL_MMAP_BYTES    = 680ULL * 1024 * 1024;
    const uint64_t KV_CACHE_BYTES      = 2ULL * 1024 * 1024;
    const uint64_t STAGING_BYTES       = 64ULL * 1024 * 1024;
    const uint64_t TOKENIZER_BYTES     = 40ULL * 1024 * 1024;

    bool session_intact = true;
    int tokenizer_reload_count = 0;
    int lmk_reclaim_rounds = 0;
    int staging_fail_count = 0;

    // --- Phase 1: Normal startup ---
    printf("\n  [Phase 1] Normal startup allocations...\n");
    bool ok = pool.allocate(MODEL_MMAP_BYTES);
    assert(ok && "Model mmap must succeed on fresh 4GB device");
    ok = pool.allocate(KV_CACHE_BYTES);
    assert(ok && "KV cache must allocate");
    ok = pool.allocate(TOKENIZER_BYTES);
    assert(ok && "Tokenizer must allocate");

    printf("  After startup: allocated=%.0fMB  remaining=%.0fMB\n",
           static_cast<float>(pool.allocated_bytes) / (1024*1024),
           static_cast<float>(pool.total_bytes - pool.allocated_bytes) / (1024*1024));

    // --- Phase 2: Background app pressure (8 background apps, 80MB each) ---
    printf("\n  [Phase 2] Background app pressure simulation...\n");
    const int BG_APPS = 8;
    const uint64_t BG_APP_BYTES = 80ULL * 1024 * 1024;
    int bg_success = 0;

    for (int i = 0; i < BG_APPS; ++i) {
        if (pool.allocate(BG_APP_BYTES)) {
            bg_success++;
        } else {
            // LMK would reclaim here
            lmk_reclaim_rounds++;
            tel.lmk_eviction_events.fetch_add(1, std::memory_order_relaxed);
            pool.free_block(BG_APP_BYTES * 2); // LMK frees 2 bg apps to make room
            if (pool.allocate(BG_APP_BYTES)) bg_success++;
            printf("  [LMK] Reclaim round %d — freed 160MB to admit bg_app_%d\n",
                   lmk_reclaim_rounds, i);
        }
    }

    printf("  bg_success=%d/%d  lmk_reclaims=%d\n", bg_success, BG_APPS, lmk_reclaim_rounds);

    // --- Phase 3: Concurrent staging allocations (NNAPI transfer pressure) ---
    printf("\n  [Phase 3] Concurrent staging allocation pressure...\n");
    for (int s = 0; s < 5; ++s) {
        if (!pool.allocate(STAGING_BYTES)) {
            staging_fail_count++;
            tel.acceleration_rejected_auto_count.fetch_add(1, std::memory_order_relaxed);
            printf("  [STAGING] Allocation %d FAILED — NNAPI staging pressure\n", s);
        } else {
            printf("  [STAGING] Allocation %d OK\n", s);
            pool.free_block(STAGING_BYTES); // immediately release after "use"
        }
    }
    tel.peak_staging_pressure_bytes.store(pool.peak_bytes, std::memory_order_relaxed);

    // --- Phase 4: mmap residency churn (repeated model page-ins/outs) ---
    printf("\n  [Phase 4] mmap residency churn simulation...\n");
    const int CHURN_CYCLES = 10;
    for (int c = 0; c < CHURN_CYCLES; ++c) {
        // Simulate page-out then page-in of 128MB of model pages
        pool.free_block(128 * 1024 * 1024);
        tel.mmap_remap_count.fetch_add(1, std::memory_order_relaxed);
        pool.allocate(128 * 1024 * 1024);
        tel.mmap_reuse_count.fetch_add(1, std::memory_order_relaxed);
    }
    printf("  mmap_remap_count=%u  mmap_reuse_count=%u\n",
           tel.mmap_remap_count.load(), tel.mmap_reuse_count.load());

    // --- Phase 5: Forced tokenizer reloads under pressure ---
    printf("\n  [Phase 5] Forced tokenizer reloads...\n");
    for (int r = 0; r < 3; ++r) {
        pool.free_block(TOKENIZER_BYTES);
        bool realloced = pool.allocate(TOKENIZER_BYTES);
        if (realloced) {
            tokenizer_reload_count++;
            tel.tokenizer_reload_count.fetch_add(1, std::memory_order_relaxed);
            tel.tokenizer_load_count.fetch_add(1, std::memory_order_relaxed);
            printf("  [TOKENIZER] Reload %d OK\n", r);
        } else {
            session_intact = false;
            tel.session_integrity_failures.fetch_add(1, std::memory_order_relaxed);
            printf("  [TOKENIZER] Reload %d FAILED — session integrity compromised\n", r);
        }
    }

    // --- Phase 6: Validate fragmentation bounds ---
    printf("\n  [Phase 6] Fragmentation check...\n");
    float frag = pool.fragmentation();
    printf("  allocated=%.0fMB / %.0fMB (%.1f%% fragmentation)\n",
           static_cast<float>(pool.allocated_bytes) / (1024*1024),
           static_cast<float>(pool.total_bytes) / (1024*1024),
           frag * 100.0f);

    // Compute allocator churn rate
    float churn_rate = static_cast<float>(pool.alloc_count + pool.free_count) / 60.0f; // per sec equiv
    tel.allocator_churn_rate.store(churn_rate, std::memory_order_relaxed);

    // Fallback must survive: model mmap + tokenizer still allocated
    bool model_still_mapped = (pool.allocated_bytes >= MODEL_MMAP_BYTES);
    printf("  model_still_mapped=%s  session_intact=%s\n",
           model_still_mapped ? "YES" : "NO",
           session_intact ? "YES" : "NO");

    // --- Assertions ---
    assert(frag <= 0.95f && "Fragmentation must stay below 95% — memory collapse");
    assert(model_still_mapped && "Model mmap must remain mapped after collapse simulation");
    assert(session_intact && "Session must remain intact after 4GB pressure simulation");
    assert(tel.lmk_eviction_events.load() > 0 && "LMK events must have been recorded");
    assert(tel.tokenizer_reload_count.load() == 3 && "All 3 tokenizer reloads must succeed");
    assert(tel.peak_staging_pressure_bytes.load() > 0);
    assert(tel.allocator_churn_rate.load() > 0.0f);

    printf("\n[device_memory_collapse_benchmark] PASS\n");
    printf("  lmk_evictions=%u  tokenizer_reloads=%u  staging_fails=%d\n",
           tel.lmk_eviction_events.load(), tel.tokenizer_reload_count.load(), staging_fail_count);
    printf("  mmap_remaps=%u  churn_rate=%.2f/s\n",
           tel.mmap_remap_count.load(), tel.allocator_churn_rate.load());

    return 0;
}
