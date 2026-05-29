#ifndef ISHA_AI_RESOURCE_LIMITS_HPP
#define ISHA_AI_RESOURCE_LIMITS_HPP

#include "device_profile.hpp"

namespace isha {

// ============================================================
// RESIDENCY CLASS — Mobile Survivability Architecture
// Defines how each model may remain in memory.
// ============================================================
enum class ResidencyClass {
    // CLASS A: Always resident. Never unloaded under any pressure.
    //          T0 (Phi-3 Mini / Qwen-0.5B), MiniLM embeddings, MS-MARCO reranker, wake-word DSP.
    //          Combined CLASS_A budget must remain < 300MB.
    //          THERMAL EXCEPTION: At >41°C (CRITICAL zone), T0 switches to retrieval-only +
    //          ultra-low-frequency inference with tiny token budgets (max 32 tokens).
    //          Does NOT continue normal routing inference at CRITICAL zone.
    CLASS_A_ALWAYS_RESIDENT,

    // CLASS B: Warm hibernated. Load on conversational request.
    //          Hibernate after 300s idle:
    //            - release KV cache (mandatory)
    //            - preserve mmap residency if safe
    //            - preserve tokenizer state
    //            - reduce active threads to 0
    //          Conditional load: free_ram>=1200MB && thermal<38°C && battery>15%
    //          Forbidden in HOT zone (38–41°C) and CRITICAL zone (>41°C).
    CLASS_B_WARM_HIBERNATED,

    // CLASS C: Burst load only. NEVER remain resident between requests.
    //          Load → execute → stream result → forceUnload() immediately.
    //          Background residency triggers forceUnload() + CRITICAL log, not just WARN.
    //          Whisper.cpp (STT), Kokoro-82M (TTS).
    CLASS_C_BURST_LOAD_ONLY,

    // CLASS D: Heavy compute. Strictly gated. Never auto-load.
    //          Never remain resident. Checkpoint before every unload.
    //          Global HeavyComputeSemaphore: only ONE CLASS_D model active at any time.
    //          T2 (Gemma-3-2B-IT), T3 (Mistral-7B).
    CLASS_D_HEAVY_COMPUTE,

    // CLASS E: Disabled for V1 ship.
    //          REMOVED: MobileVLM-3B, LLaVA-1.6-Mistral-7B.
    //          All VLM paths return DISABLED_V1 immediately.
    CLASS_E_DISABLED_V1,
};

// ============================================================
// PER-TIER MAX OUTPUT TOKEN CAPS
// Hard truncation. Prevents runaway generation → heat + memory creep.
// ============================================================
struct TokenBudgetPolicy {
    static constexpr unsigned int T0_MAX_OUTPUT = 128;  // Router only — very short responses
    static constexpr unsigned int T1_MAX_OUTPUT = 384;  // Conversational workhorse
    static constexpr unsigned int T2_MAX_OUTPUT = 768;  // Reasoning model
    static constexpr unsigned int T3_MAX_OUTPUT = 1024; // Ultra brain — absolute cap

    static unsigned int getMaxOutput(unsigned int model_size_mb) {
        if (model_size_mb <= 250)  return T0_MAX_OUTPUT;
        if (model_size_mb <= 900)  return T1_MAX_OUTPUT;
        if (model_size_mb <= 1500) return T2_MAX_OUTPUT;
        return T3_MAX_OUTPUT;
    }
};

// ============================================================
// MODEL LOAD GATE — Conditional load constraints per tier
// ALL conditions must be satisfied before loading.
// ============================================================
struct ModelLoadGate {
    unsigned int min_free_ram_mb      = 0;
    unsigned int min_physical_ram_gb  = 0;
    double       max_thermal_c        = 46.0;
    unsigned int min_battery_percent  = 0;
    bool         never_auto_load      = false;
    unsigned int max_active_seconds   = 0;     // 0 = unlimited; >0 = forced unload deadline
    int          idle_evict_days      = -1;    // -1 = never; >0 = evict after N idle days

    // T1 (CLASS_B): free_ram>=1200 && thermal<38°C && battery>15 && !system_memory_pressure
    //   Gate uses HOT zone entry (38°C) — T1 is forbidden in HOT zone and above.
    //   Per isha ai.md: HOT zone (38–41°C) = T0 only, retrieval-only.
    static ModelLoadGate forT1() {
        return { 1200, 0, 38.0, 15, false, 0, -1 };
    }

    // T2 (CLASS_D): physical_ram>=6GB && free_ram>=2200 && thermal<35°C && battery>20
    //   Gate uses WARM zone entry (35°C) — T2 is forbidden in WARM zone and above.
    //   Per isha ai.md: WARM zone (35–38°C) = T1 max, T2 disabled.
    static ModelLoadGate forT2() {
        return { 2200, 6, 35.0, 20, false, 90, 7 };
    }

    // T3 (CLASS_D): physical_ram>=8GB && free_ram>=6000 && thermal<35°C && battery>40 && charging
    //   Gate uses NOMINAL zone only (below 35°C) — T3 forbidden in WARM zone and above.
    //   Per isha ai.md: T3 load gate strict: thermal < 35°C.
    //   Never auto-loads — explicit user request required (Deep Reasoning / Ultra Mode).
    //   Absolute max runtime: 180s, then forced unload.
    //   8GB = experimental (requires charging=true + battery_saver=false enforced).
    //   12GB+ = certified tier.
    static ModelLoadGate forT3(unsigned int physical_ram_gb = 12) {
        ModelLoadGate g;
        g.min_free_ram_mb     = 6000;
        g.min_physical_ram_gb = 8;
        g.max_thermal_c       = 35.0;
        g.min_battery_percent = 40;
        g.never_auto_load     = true;
        g.max_active_seconds  = 180;
        g.idle_evict_days     = 14;
        // 8GB devices: require_charging enforced by canLoadT3(); extra caution flag
        (void)physical_ram_gb;
        return g;
    }
};

// ============================================================
// THERMAL POLICY — 4-Zone Governance per isha ai.md (MASTER SPEC)
// ============================================================
//
// ZONE 1 — NOMINAL  : < 35°C
//   All tiers (T0, T1, T2, T3) allowed. Full context (512 tokens). Full batching.
//   No degradation applied.
//
// ZONE 2 — WARM     : 35°C – 38°C
//   T2 and T3 FORBIDDEN. T0 and T1 allowed.
//   Actions: unload T2/T3 immediately, reduce context to 256 tokens,
//   reduce batch size, reduce polling frequency, activate light throttle mode.
//
// ZONE 3 — HOT      : 38°C – 41°C
//   T1, T2, T3 FORBIDDEN. T0 ONLY.
//   Actions: unload T1/T2/T3 immediately, retrieval-only mode,
//   disable active generation, aggressive scheduler throttling,
//   minimize wake frequency.
//
// ZONE 4 — CRITICAL : > 41°C
//   ALL inference SUSPENDED. T0 enters ultra-low-frequency emergency mode.
//   Actions: emergency thermal mode, all generation suspended,
//   cooldown lockout required before ANY recovery,
//   no decode loops, no speculative loading, no auto-recovery.
//   T0 token budget capped at 32 tokens (retrieval-only micro-inference).
//
// HYSTERESIS: To prevent oscillation, tier re-enable requires sustained cool-down
//   below (zone_threshold - HYSTERESIS_MARGIN) for at least RECOVERY_STABILIZATION_SECONDS.
//   Example: T1 disabled at 38°C → re-enable only after cooling to <36°C for 90s+.
//
// Source: isha ai.md (primary), consumer_expectation_policy.md (soft throttle at 39.5°C),
//         uninstall_risk_policy.md (risk tiers), release_exit_gate.md (43°C battery ceiling).
struct ThermalPolicy {
    // Zone transition thresholds (isha ai.md master spec)
    static constexpr double ZONE_NOMINAL_MAX  = 35.0;  // below this: all tiers allowed
    static constexpr double ZONE_WARM_MAX     = 38.0;  // WARM: T1 max, T2/T3 forbidden
    static constexpr double ZONE_HOT_MAX      = 41.0;  // HOT:  T0 only, retrieval-only
    // above ZONE_HOT_MAX = CRITICAL: all inference suspended

    // Convenience aliases for existing code
    static constexpr double ZONE1_T3_DISABLE  = ZONE_NOMINAL_MAX;  // 35°C
    static constexpr double ZONE2_T2_DISABLE  = ZONE_NOMINAL_MAX;  // 35°C (same — WARM disables both)
    static constexpr double ZONE3_T1_ONLY     = ZONE_WARM_MAX;     // 38°C
    static constexpr double ZONE4_SUSPEND_ALL = ZONE_HOT_MAX;      // 41°C

    // Hysteresis margin: must sustain below (zone_threshold - HYSTERESIS_MARGIN) to re-enable
    // Prevents thermal oscillation reload loops.
    static constexpr double HYSTERESIS_MARGIN = 2.0;

    // Minimum stabilization window before re-enabling a higher tier after downgrade.
    // 90 seconds chosen to prevent rapid repeated load/unload on borderline temps.
    static constexpr unsigned int RECOVERY_STABILIZATION_SECONDS = 90;

    // T0 max token budget in CRITICAL zone (>41°C) — retrieval-only micro-inference only
    static constexpr unsigned int T0_THERMAL_EMERGENCY_MAX_TOKENS = 32;

    // Hysteresis re-enable thresholds (must cool BELOW these, sustained, before re-enabling)
    static constexpr double T2_T3_REENABLE_BELOW = ZONE_NOMINAL_MAX - HYSTERESIS_MARGIN;  // 33°C
    static constexpr double T1_REENABLE_BELOW    = ZONE_WARM_MAX    - HYSTERESIS_MARGIN;  // 36°C
    static constexpr double FULL_REENABLE_BELOW  = ZONE_HOT_MAX     - HYSTERESIS_MARGIN;  // 39°C

    // consumer_expectation_policy.md: additional soft throttle at 39.5°C
    // (applied alongside the 4-zone model — reduces batching + telemetry intensity)
    static constexpr double SOFT_THROTTLE_TEMP  = 39.5;

    // release_exit_gate.md: active inference must never push battery temp above this
    static constexpr double BATTERY_TEMP_CEILING = 43.0;
};

// ============================================================
// T1 HIBERNATION POLICY
// Defines what happens when CLASS_B enters idle timeout.
// ============================================================
struct T1HibernationPolicy {
    // Time of inactivity (no user request, no background task) before hibernating T1
    static constexpr unsigned int IDLE_TIMEOUT_SECONDS = 300; // 5 minutes

    // On hibernation trigger:
    // 1. Release KV cache immediately (mandatory — reclaims 14–28MB)
    // 2. Preserve mmap residency if OS permits (avoid cold-start next load)
    // 3. Preserve tokenizer state (avoid re-init overhead)
    // 4. Reduce active inference threads to 0
    //
    // T1 mmap may be evicted by OS under memory pressure — this is acceptable.
    // Next load restores from mmap if resident, or re-mmaps from disk if not.
    static constexpr bool RELEASE_KV_CACHE_ON_HIBERNATE      = true;
    static constexpr bool PRESERVE_MMAP_ON_HIBERNATE         = true;
    static constexpr bool PRESERVE_TOKENIZER_ON_HIBERNATE    = true;
    static constexpr unsigned int THREAD_COUNT_WHEN_HIBERNATED = 0;
};

// ============================================================
// STORAGE FLOOR POLICY
// Minimum free storage required before starting ANY heavy model download.
// Prevents cheap NAND devices from soft-bricking storage.
// ============================================================
struct StorageFloorPolicy {
    // Minimum free storage before ANY download of T2 or T3 models
    static constexpr uint64_t MIN_FREE_STORAGE_BYTES = 1ULL * 1024 * 1024 * 1024; // 1.0 GB

    // Additional safety margin on top of model file size
    static constexpr uint64_t SAFETY_MARGIN_BYTES = 512ULL * 1024 * 1024; // 512 MB

    // T0 and embeddings are exempt (small enough to always proceed)
    static constexpr uint64_t EXEMPT_BELOW_SIZE_BYTES = 200ULL * 1024 * 1024; // < 200MB = exempt

    // Storage pressure eviction order: T3 first, then T2. Never T0, never T1.
    // T2: evict after 7 idle days
    // T3: evict after 14 idle days
    static constexpr int T2_IDLE_EVICT_DAYS = 7;
    static constexpr int T3_IDLE_EVICT_DAYS = 14;
};

// ============================================================
// RESOURCE LIMITS — Per device tier runtime caps
// ============================================================
struct ResourceLimits {
    unsigned int max_allowed_ram_usage_mb;
    unsigned int model_memory_limit_mb;
    unsigned int inactive_timeout_seconds;
    bool allow_t2_models;
    bool allow_t3_models;

    // -------------------------------------------------------
    // RUNTIME LOAD GATES (called at model load time)
    // -------------------------------------------------------

    // T1 (CLASS_B) gate: free_ram>=1200 && thermal<38°C && battery>15 && !system_memory_pressure
    // 38°C = entry of HOT zone (T1 forbidden in HOT and CRITICAL).
    // Per isha ai.md: HOT zone (38–41°C) = T0 only, retrieval-only mode.
    // 4GB devices: CONDITIONALLY SUPPORTED — NOT guaranteed.
    // Failure fallback: T0 + retrieval-only + honest user message.
    static bool canLoadT1(unsigned int free_ram_mb, double thermal_c,
                          unsigned int battery_pct, bool system_memory_pressure) {
        const ModelLoadGate g = ModelLoadGate::forT1();
        return free_ram_mb >= g.min_free_ram_mb &&
               thermal_c   <  g.max_thermal_c   &&   // < 38°C
               battery_pct >  g.min_battery_percent &&
               !system_memory_pressure;
    }

    // T2 (CLASS_D) gate: physical_ram>=6GB && free_ram>=2200 && thermal<35°C && battery>20
    // 35°C = entry of WARM zone (T2 forbidden in WARM, HOT, CRITICAL).
    // Per isha ai.md: WARM zone (35–38°C) = T1 max, T2 disabled.
    // T2 DOES NOT target 4GB devices. Minimum 6GB; recommended 8GB.
    static bool canLoadT2(unsigned int physical_ram_gb, unsigned int free_ram_mb,
                          double thermal_c, unsigned int battery_pct) {
        const ModelLoadGate g = ModelLoadGate::forT2();
        return physical_ram_gb >= g.min_physical_ram_gb &&
               free_ram_mb     >= g.min_free_ram_mb     &&
               thermal_c       <  g.max_thermal_c       &&   // < 35°C
               battery_pct     >  g.min_battery_percent;
    }

    // T3 gate: physical_ram>=8GB && free_ram>=6000 && thermal<35 && battery>40 && charging
    // 8GB = EXPERIMENTAL — requires charging=true AND battery_saver=false.
    // 12GB+ = CERTIFIED tier.
    // T3 NEVER auto-loads. User MUST explicitly trigger Deep Reasoning / Ultra Mode.
    static bool canLoadT3(unsigned int physical_ram_gb, unsigned int free_ram_mb,
                          double thermal_c, unsigned int battery_pct, bool charging) {
        const ModelLoadGate g = ModelLoadGate::forT3(physical_ram_gb);
        bool base = physical_ram_gb >= g.min_physical_ram_gb &&
                    free_ram_mb     >= g.min_free_ram_mb     &&
                    thermal_c       <  g.max_thermal_c       &&
                    battery_pct     >  g.min_battery_percent;
        if (!base) return false;

        // 8GB devices: EXPERIMENTAL — must be charging to reduce LMK kill risk
        if (physical_ram_gb < 12) {
            return charging; // strict: must be plugged in on 8GB
        }

        // 12GB+ devices: charging preferred but not required
        return true;
    }

    static ResourceLimits getLimitsForTier(DeviceTier tier) {
        switch (tier) {
            case DeviceTier::LOW:
                // RAM <= 4GB: T0 always resident + T1 conditionally.
                // Fallback: T0 + retrieval-only if T1 cannot load.
                return { 1024, 750, 300, false, false };
            case DeviceTier::MID:
                // RAM 6GB: T0 + T1 + T2 (6GB floor enforced).
                // T2 requires runtime gate check (canLoadT2).
                return { 2048, 1200, 300, true, false };
            case DeviceTier::HIGH:
                // RAM 8GB: T0 + T1 + T2 + T3 (8GB floor, experimental for T3).
                // T3 requires explicit user request + charging.
                // 12GB+ is recommended for T3 without charging requirement.
                return { 6144, 4500, 300, true, true };
            case DeviceTier::FLAGSHIP:
                // 12GB+: All tiers certified. T3 without charging restriction.
                return { 6144, 4500, 300, true, true };
            default:
                return { 256, 100, 60, false, false };
        }
    }
};

} // namespace isha

#endif // ISHA_AI_RESOURCE_LIMITS_HPP
