// survivability_residency_class_benchmark.cpp
//
// Validates the Mobile Survivability Architecture Correction Patch:
// - Residency classes A/B/C/D/E
// - T1/T2/T3 load gates (all conditions)
// - T3 8GB=experimental/12GB=certified
// - T0 thermal emergency mode at >46°C
// - T1 hibernation policy (300s, KV release, preserve mmap/tokenizer)
// - HeavyComputeSemaphore (only one CLASS_D active)
// - Storage floor 1GB enforcement
// - Thermal 5-zone hysteresis (120s stabilization)
// - iOS nuanced background (short=suspend, long=unload)
// - STT/TTS CLASS_C force teardown (not WARN)
// - Per-tier token budget caps (T1=384, T2=768, T3=1024)

#include <iostream>
#include <cassert>
#include <string>
#include <chrono>
#include <thread>

#include "../core/config/resource_limits.hpp"
#include "../core/config/device_profile.hpp"
#include "../core/model/model_manager.hpp"
#include "../core/model/model_registry.hpp"
#include "../core/model/download_manager.hpp"
#include "../core/survival/model_recommendation_policy.hpp"
#include "../core/voice/stt_engine.hpp"
#include "../core/voice/tts_engine.hpp"
#include "../core/multimodal/multimodal_context_governor.hpp"
#include "../core/inference/ios_coreml_manager.hpp"

using namespace isha;

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(condition, label) \
    do { \
        if (condition) { \
            std::cout << "[PASS] " << label << "\n"; \
            ++g_pass; \
        } else { \
            std::cout << "[FAIL] " << label << "\n"; \
            ++g_fail; \
        } \
    } while(0)

// ============================================================
// SECTION 1: ResidencyClass enum and TokenBudgetPolicy
// ============================================================
void test_residency_class_enum() {
    std::cout << "\n== SECTION 1: ResidencyClass enum + TokenBudgetPolicy ==\n";

    // Residency class values compile and compare correctly
    ResidencyClass a = ResidencyClass::CLASS_A_ALWAYS_RESIDENT;
    ResidencyClass b = ResidencyClass::CLASS_B_WARM_HIBERNATED;
    ResidencyClass c = ResidencyClass::CLASS_C_BURST_LOAD_ONLY;
    ResidencyClass d = ResidencyClass::CLASS_D_HEAVY_COMPUTE;
    ResidencyClass e = ResidencyClass::CLASS_E_DISABLED_V1;

    CHECK(a != b, "CLASS_A != CLASS_B");
    CHECK(c != d, "CLASS_C != CLASS_D");
    CHECK(e != a, "CLASS_E != CLASS_A");

    // Token budget caps
    CHECK(TokenBudgetPolicy::T0_MAX_OUTPUT == 128,  "T0 max output = 128");
    CHECK(TokenBudgetPolicy::T1_MAX_OUTPUT == 384,  "T1 max output = 384");
    CHECK(TokenBudgetPolicy::T2_MAX_OUTPUT == 768,  "T2 max output = 768");
    CHECK(TokenBudgetPolicy::T3_MAX_OUTPUT == 1024, "T3 max output = 1024");

    // getMaxOutput() dispatch
    CHECK(TokenBudgetPolicy::getMaxOutput(84)   == 128,  "getMaxOutput(T0 84MB)=128");
    CHECK(TokenBudgetPolicy::getMaxOutput(720)  == 384,  "getMaxOutput(T1 720MB)=384");
    CHECK(TokenBudgetPolicy::getMaxOutput(1150) == 768,  "getMaxOutput(T2 1150MB)=768");
    CHECK(TokenBudgetPolicy::getMaxOutput(4200) == 1024, "getMaxOutput(T3 4200MB)=1024");
}

// ============================================================
// SECTION 2: T1 Load Gate
// ============================================================
void test_t1_load_gate() {
    std::cout << "\n== SECTION 2: T1 Load Gate ==\n";

    // All conditions good → allow
    CHECK(ResourceLimits::canLoadT1(1200, 43.9, 16, false), "T1 gate: all conditions met");

    // free_ram too low
    CHECK(!ResourceLimits::canLoadT1(1199, 43.9, 16, false), "T1 gate: free_ram<1200 → deny");

    // thermal too high
    CHECK(!ResourceLimits::canLoadT1(1500, 44.0, 50, false), "T1 gate: thermal>=44°C → deny");
    CHECK(!ResourceLimits::canLoadT1(1500, 45.0, 50, false), "T1 gate: thermal>44°C → deny");

    // battery too low
    CHECK(!ResourceLimits::canLoadT1(1500, 43.0, 15, false), "T1 gate: battery<=15% → deny");

    // system memory pressure
    CHECK(!ResourceLimits::canLoadT1(1500, 43.0, 50, true), "T1 gate: system_memory_pressure → deny");

    // boundary: thermal just below 44°C
    CHECK(ResourceLimits::canLoadT1(1200, 43.99, 16, false), "T1 gate: thermal=43.99°C → allow");
}

// ============================================================
// SECTION 3: T2 Load Gate (6GB physical RAM minimum)
// ============================================================
void test_t2_load_gate() {
    std::cout << "\n== SECTION 3: T2 Load Gate (6GB minimum) ==\n";

    // All conditions good
    CHECK(ResourceLimits::canLoadT2(6, 2200, 41.9, 21), "T2 gate: 6GB good → allow");
    CHECK(ResourceLimits::canLoadT2(8, 3000, 38.0, 30), "T2 gate: 8GB good → allow");

    // physical RAM too low (4GB device)
    CHECK(!ResourceLimits::canLoadT2(4, 2200, 41.0, 25), "T2 gate: 4GB device → deny");
    CHECK(!ResourceLimits::canLoadT2(5, 2200, 41.0, 25), "T2 gate: 5GB device → deny");

    // free RAM too low
    CHECK(!ResourceLimits::canLoadT2(6, 2199, 41.0, 25), "T2 gate: free_ram<2200 → deny");

    // thermal too high
    CHECK(!ResourceLimits::canLoadT2(6, 2500, 42.0, 25), "T2 gate: thermal>=42°C → deny");

    // battery too low
    CHECK(!ResourceLimits::canLoadT2(6, 2500, 41.0, 20), "T2 gate: battery<=20% → deny");
}

// ============================================================
// SECTION 4: T3 Load Gate (8GB=experimental, 12GB=certified)
// ============================================================
void test_t3_load_gate() {
    std::cout << "\n== SECTION 4: T3 Load Gate (8GB experimental, 12GB certified) ==\n";

    // 12GB certified — charging not required but all else must pass
    CHECK(ResourceLimits::canLoadT3(12, 6000, 34.9, 41, false), "T3 gate: 12GB, no charging → allow");
    CHECK(ResourceLimits::canLoadT3(12, 6000, 34.9, 41, true),  "T3 gate: 12GB, charging → allow");

    // 8GB experimental — REQUIRES charging
    CHECK(ResourceLimits::canLoadT3(8, 6000, 34.9, 41, true),   "T3 gate: 8GB + charging → allow");
    CHECK(!ResourceLimits::canLoadT3(8, 6000, 34.9, 41, false),  "T3 gate: 8GB, no charging → DENY (experimental)");

    // physical RAM too low
    CHECK(!ResourceLimits::canLoadT3(6, 6000, 34.0, 50, true),  "T3 gate: 6GB → deny");
    CHECK(!ResourceLimits::canLoadT3(7, 6000, 34.0, 50, true),  "T3 gate: 7GB → deny");

    // free RAM too low
    CHECK(!ResourceLimits::canLoadT3(12, 5999, 34.0, 50, true), "T3 gate: free_ram<6000 → deny");

    // thermal too high (must be < 35°C)
    CHECK(!ResourceLimits::canLoadT3(12, 6000, 35.0, 50, true), "T3 gate: thermal>=35°C → deny");
    CHECK(!ResourceLimits::canLoadT3(12, 6000, 39.0, 50, true), "T3 gate: thermal=39°C → deny");

    // battery too low
    CHECK(!ResourceLimits::canLoadT3(12, 6000, 34.0, 40, true), "T3 gate: battery<=40% → deny");
}

// ============================================================
// SECTION 5: Thermal 5-Zone Policy Constants + Hysteresis
// ============================================================
void test_thermal_policy() {
    std::cout << "\n== SECTION 5: Thermal 5-Zone Policy ==\n";

    CHECK(ThermalPolicy::ZONE1_T3_DISABLE    == 39.0, "ZONE1 T3 disable = 39°C");
    CHECK(ThermalPolicy::ZONE2_T2_DISABLE    == 42.0, "ZONE2 T2 disable = 42°C");
    CHECK(ThermalPolicy::ZONE3_T1_ONLY       == 44.0, "ZONE3 T1-only = 44°C");
    CHECK(ThermalPolicy::ZONE4_SUSPEND_ALL   == 46.0, "ZONE4 suspend-all = 46°C");
    CHECK(ThermalPolicy::HYSTERESIS_MARGIN   == 2.0,  "Hysteresis margin = 2°C");
    CHECK(ThermalPolicy::RECOVERY_STABILIZATION_SECONDS == 120, "Recovery stabilization = 120s");
    CHECK(ThermalPolicy::T0_THERMAL_EMERGENCY_MAX_TOKENS == 32,
          "T0 emergency max tokens = 32");

    // Verify re-enable thresholds
    CHECK(ThermalPolicy::T3_REENABLE_BELOW   == 37.0, "T3 re-enable < 37°C");
    CHECK(ThermalPolicy::T2_REENABLE_BELOW   == 40.0, "T2 re-enable < 40°C");
    CHECK(ThermalPolicy::T1_REENABLE_BELOW   == 42.0, "T1 re-enable < 42°C");
    CHECK(ThermalPolicy::FULL_REENABLE_BELOW == 44.0, "Full re-enable < 44°C");
}

// ============================================================
// SECTION 6: ModelManager Thermal Unload — 5 zones verified
// ============================================================
void test_model_manager_thermal_zones() {
    std::cout << "\n== SECTION 6: ModelManager Thermal Zones ==\n";

    DeviceProfile profile;
    profile.total_ram_mb = 8192;
    profile.tier = DeviceTier::HIGH;

    // T3 model (4200MB)
    ModelManager mgr_t3(profile);
    mgr_t3.loadModel("mistral-7b-q4", 4200);
    CHECK(mgr_t3.getState() == ModelState::ACTIVE, "T3 loaded → ACTIVE");

    // Zone 1: <35°C — no unload
    mgr_t3.thermalUnload(34.0);
    CHECK(mgr_t3.getState() == ModelState::ACTIVE, "Zone1 <35°C: T3 stays active");

    // Zone 2: 35–38°C — T3 unloads
    mgr_t3.thermalUnload(36.0);
    CHECK(mgr_t3.getState() == ModelState::UNLOADED, "Zone2 36°C: T3 unloaded");

    // T2 model (1150MB) — Zone 1 should NOT unload T2
    ModelManager mgr_t2(profile);
    mgr_t2.loadModel("gemma-3-2b-q4", 1150);
    mgr_t2.thermalUnload(34.0);
    CHECK(mgr_t2.getState() == ModelState::ACTIVE, "Zone1 <35°C: T2 stays active");

    // Zone 2: 35-38°C — T2 unloads
    mgr_t2.thermalUnload(36.0);
    CHECK(mgr_t2.getState() == ModelState::UNLOADED, "Zone2 36°C: T2 unloaded");

    // T1 model (720MB) — Zone 2 stays active
    ModelManager mgr_t1(profile);
    mgr_t1.loadModel("qwen-1.5b-q4", 720);
    mgr_t1.thermalUnload(36.0);
    CHECK(mgr_t1.getState() == ModelState::ACTIVE, "Zone2 36°C: T1 stays active");

    // Zone 3: 38–41°C — T1 unloads
    mgr_t1.thermalUnload(39.0);
    CHECK(mgr_t1.getState() == ModelState::UNLOADED, "Zone3 39°C: T1 unloaded");

    // Zone 4: >41°C — T0 enters emergency mode (not unloaded)
    ModelManager mgr_t0(profile);
    mgr_t0.loadModel("qwen-0.5b-q4", 84);
    CHECK(!mgr_t0.isT0ThermalEmergencyMode(), "Before Zone4: T0 not in emergency mode");
    mgr_t0.thermalUnload(42.0);
    CHECK(mgr_t0.isT0ThermalEmergencyMode(), "Zone4 42°C: T0 enters RETRIEVAL-ONLY emergency mode");
    // T0 itself must NOT be unloaded (CLASS_A protection)
    CHECK(mgr_t0.getState() == ModelState::ACTIVE, "Zone4: T0 NOT unloaded (CLASS_A)");
}

// ============================================================
// SECTION 7: T1 Hibernation Policy
// ============================================================
void test_t1_hibernation() {
    std::cout << "\n== SECTION 7: T1 Hibernation Policy ==\n";

    CHECK(T1HibernationPolicy::IDLE_TIMEOUT_SECONDS         == 300, "T1 idle timeout = 300s");
    CHECK(T1HibernationPolicy::RELEASE_KV_CACHE_ON_HIBERNATE == true, "KV cache release on hibernate = true");
    CHECK(T1HibernationPolicy::PRESERVE_MMAP_ON_HIBERNATE    == true, "mmap preserved on hibernate = true");
    CHECK(T1HibernationPolicy::PRESERVE_TOKENIZER_ON_HIBERNATE == true, "tokenizer preserved on hibernate = true");
    CHECK(T1HibernationPolicy::THREAD_COUNT_WHEN_HIBERNATED  == 0,   "threads = 0 when hibernated");

    DeviceProfile profile;
    profile.total_ram_mb = 4096;
    profile.tier = DeviceTier::LOW;

    ModelManager mgr(profile);
    mgr.loadModel("qwen-1.5b-q4", 720);
    CHECK(mgr.getState() == ModelState::ACTIVE, "T1 loaded → ACTIVE");

    mgr.setResidencyClass(ResidencyClass::CLASS_B_WARM_HIBERNATED);
    mgr.hibernateT1();
    CHECK(mgr.getState() == ModelState::HIBERNATED, "T1 hibernates → HIBERNATED state");
    CHECK(mgr.getHibernationCount() == 1, "Hibernation count = 1");

    // Wake from hibernation — fast path
    bool woke = mgr.wakeFromHibernation();
    CHECK(woke, "T1 wakes from hibernation");
    CHECK(mgr.getState() == ModelState::ACTIVE, "T1 woke → ACTIVE");
}

// ============================================================
// SECTION 8: HeavyComputeSemaphore — only ONE CLASS_D active
// ============================================================
void test_heavy_compute_semaphore() {
    std::cout << "\n== SECTION 8: HeavyComputeSemaphore ==\n";

    auto& sem = HeavyComputeSemaphore::getInstance();
    sem.release("any"); // ensure clean state

    CHECK(!sem.isHeld(), "Semaphore initially free");

    bool acquired1 = sem.tryAcquire("gemma-3-2b");
    CHECK(acquired1, "First acquire succeeds (gemma-3-2b)");
    CHECK(sem.isHeld(), "Semaphore now held");
    CHECK(sem.currentHolder() == "gemma-3-2b", "Holder = gemma-3-2b");

    // Second acquire must FAIL (only one CLASS_D at a time)
    bool acquired2 = sem.tryAcquire("mistral-7b");
    CHECK(!acquired2, "Second acquire blocked (mistral-7b)");
    CHECK(sem.currentHolder() == "gemma-3-2b", "Holder still gemma-3-2b");

    // Also blocks Whisper trying to load alongside T2
    bool acquired3 = sem.tryAcquire("whisper-cpp");
    CHECK(!acquired3, "Whisper blocked while T2 active");

    sem.release("gemma-3-2b");
    CHECK(!sem.isHeld(), "Semaphore released");

    // Now T3 can acquire
    bool acquired4 = sem.tryAcquire("mistral-7b");
    CHECK(acquired4, "T3 acquires after T2 released");
    sem.release("mistral-7b");
}

// ============================================================
// SECTION 9: T3 180-second Forced Unload
// ============================================================
void test_t3_time_limit() {
    std::cout << "\n== SECTION 9: T3 180s Forced Unload ==\n";

    DeviceProfile profile;
    profile.total_ram_mb = 12288;
    profile.tier = DeviceTier::FLAGSHIP;

    ModelManager mgr(profile);
    mgr.loadModel("mistral-7b-q4", 4200);
    mgr.setResidencyClass(ResidencyClass::CLASS_D_HEAVY_COMPUTE);
    CHECK(mgr.getState() == ModelState::ACTIVE, "T3 loaded");

    // Right after load, time remaining should be ~180s
    double remaining = mgr.getT3SecondsRemaining();
    CHECK(remaining > 170.0 && remaining <= 181.0,
          "T3 deadline ~180s remaining at load time");

    // Before deadline: enforceT3TimeLimit() should NOT fire
    bool fired = mgr.enforceT3TimeLimit();
    CHECK(!fired, "T3 time limit not fired yet (within 180s)");
    CHECK(mgr.getState() == ModelState::ACTIVE, "T3 still active");
}

// ============================================================
// SECTION 10: EmergencyUnload CLASS_A Protection
// ============================================================
void test_class_a_emergency_protection() {
    std::cout << "\n== SECTION 10: CLASS_A Emergency Protection ==\n";

    DeviceProfile profile;
    profile.total_ram_mb = 4096;
    profile.tier = DeviceTier::LOW;

    // T0 router (84MB) — must NOT be unloaded
    ModelManager mgr_t0(profile);
    mgr_t0.loadModel("qwen-0.5b", 84);
    mgr_t0.setResidencyClass(ResidencyClass::CLASS_A_ALWAYS_RESIDENT);
    mgr_t0.emergencyUnload();
    CHECK(mgr_t0.getState() == ModelState::ACTIVE,
          "CLASS_A T0 (84MB) survives emergency unload");
    CHECK(mgr_t0.getEmergencyUnloadCount() == 0,
          "Emergency unload count = 0 for CLASS_A T0");

    // Embedding (28MB) — also CLASS_A, must NOT be unloaded
    ModelManager mgr_emb(profile);
    mgr_emb.loadModel("minilm-l6-v2", 28);
    mgr_emb.setResidencyClass(ResidencyClass::CLASS_A_ALWAYS_RESIDENT);
    mgr_emb.emergencyUnload();
    CHECK(mgr_emb.getState() == ModelState::ACTIVE,
          "CLASS_A embedding (28MB) survives emergency unload");

    // T1 (720MB) — CLASS_B, SHOULD be unloaded
    ModelManager mgr_t1(profile);
    mgr_t1.loadModel("qwen-1.5b-q4", 720);
    mgr_t1.setResidencyClass(ResidencyClass::CLASS_B_WARM_HIBERNATED);
    mgr_t1.emergencyUnload();
    CHECK(mgr_t1.getState() == ModelState::UNLOADED,
          "CLASS_B T1 (720MB) is unloaded on emergency");
}

// ============================================================
// SECTION 11: STT/TTS CLASS_C Burst-Only Enforcement
// ============================================================
void test_stt_tts_class_c() {
    std::cout << "\n== SECTION 11: STT/TTS CLASS_C Burst-Only ==\n";

    CHECK(!WhisperSTTEngine::isResidentAllowed(),
          "STT: isResidentAllowed() = false");
    CHECK(!KokoroTTSEngine::isResidentAllowed(),
          "TTS: isResidentAllowed() = false");

    // STT: no background residency during active session → force teardown
    WhisperSTTEngine stt;
    stt.beginSession();
    CHECK(stt.isSessionActive(), "STT session active after beginSession()");

    // Background detected with active session → CRITICAL + force teardown
    bool tore_down = stt.enforceNoBgResidency(true);
    CHECK(tore_down, "STT bg residency → force teardown (not just WARN)");
    CHECK(!stt.isSessionActive(), "STT session inactive after force teardown");

    // No background — no teardown
    stt.beginSession();
    bool no_teardown = stt.enforceNoBgResidency(false);
    CHECK(!no_teardown, "STT foreground: no teardown needed");
    stt.endSession();

    // TTS: same policy
    KokoroTTSEngine tts;
    tts.beginSession();
    bool tts_teardown = tts.enforceNoBgResidency(true);
    CHECK(tts_teardown, "TTS bg residency → force teardown (not just WARN)");
    CHECK(!tts.isSessionActive(), "TTS session inactive after force teardown");
}

// ============================================================
// SECTION 12: VLM CLASS_E — Always Disabled
// ============================================================
void test_vlm_class_e_disabled() {
    std::cout << "\n== SECTION 12: VLM CLASS_E Always Disabled ==\n";

    // MultimodalContextGovernor: VLM routing disabled
    CHECK(MultimodalContextGovernor::isVlmDisabledV1(),
          "MultimodalContextGovernor::isVlmDisabledV1() = true");

    // Vision description is cleared when VLM disabled
    MultimodalContextGovernor gov;
    std::vector<DocumentChunk> chunks;
    std::vector<std::pair<std::string,std::string>> history;
    auto ctx = gov.assembleMultimodalContext(
        "system", chunks, history, "query", "", "vision text here");
    CHECK(ctx.vision_description.empty(),
          "Vision description cleared when VLM disabled (CLASS_E)");

    // Download manager: VLM downloads rejected
    DownloadManager dm;
    DownloadConfig cfg;
    cfg.url = "https://huggingface.co/mobilevlm-3b.gguf";
    cfg.output_path = "models/vlm/mobilevlm-3b.gguf";
    cfg.expected_size_bytes = 0; // small so storage floor doesn't fire
    auto result = dm.download(cfg);
    CHECK(!result.success, "MobileVLM download rejected by DownloadManager");
    CHECK(result.error.find("CLASS_E") != std::string::npos,
          "Rejection error mentions CLASS_E");

    DownloadConfig cfg2;
    cfg2.url = "https://huggingface.co/llava-1.6-mistral-7b.gguf";
    cfg2.output_path = "models/vlm/llava-1.6.gguf";
    cfg2.expected_size_bytes = 0;
    auto result2 = dm.download(cfg2);
    CHECK(!result2.success, "LLaVA download rejected by DownloadManager");
}

// ============================================================
// SECTION 13: Storage Floor Enforcement (Issue 4)
// ============================================================
void test_storage_floor_policy() {
    std::cout << "\n== SECTION 13: Storage Floor Policy ==\n";

    CHECK(StorageFloorPolicy::MIN_FREE_STORAGE_BYTES ==
          (1ULL * 1024 * 1024 * 1024), "Storage floor = 1GB");
    CHECK(StorageFloorPolicy::SAFETY_MARGIN_BYTES ==
          (512ULL * 1024 * 1024), "Safety margin = 512MB");
    CHECK(StorageFloorPolicy::EXEMPT_BELOW_SIZE_BYTES ==
          (200ULL * 1024 * 1024), "Exempt threshold = 200MB");
    CHECK(StorageFloorPolicy::T2_IDLE_EVICT_DAYS == 7,  "T2 evict after 7 idle days");
    CHECK(StorageFloorPolicy::T3_IDLE_EVICT_DAYS == 14, "T3 evict after 14 idle days");

    // Download manager enforces storage floor for heavy models
    // (actual disk check depends on filesystem — just verify rejection logic path)
    DownloadManager dm;
    DownloadConfig heavy_cfg;
    heavy_cfg.url = "https://example.com/gemma-3-2b.gguf";
    heavy_cfg.output_path = "/nonexistent/path/gemma-3-2b.gguf";
    // 1GB file size — triggers storage floor check
    heavy_cfg.expected_size_bytes = 1ULL * 1024 * 1024 * 1024;
    // hasSufficientDiskSpace on nonexistent path returns false → storage floor enforced
    auto res = dm.download(heavy_cfg);
    CHECK(!res.success, "Heavy model download fails when storage floor not met");
}

// ============================================================
// SECTION 14: iOS CoreML Nuanced Background (Issue 6)
// ============================================================
void test_ios_nuanced_background() {
    std::cout << "\n== SECTION 14: iOS Nuanced Background Entry ==\n";

    DeviceProfile profile;
    profile.total_ram_mb = 8192;
    profile.tier = DeviceTier::HIGH;

    // T2 (CLASS_D): must checkpoint + unload on background entry
    ModelManager mgr_t2(profile);
    mgr_t2.loadModel("gemma-3-2b-q4", 1150);
    mgr_t2.setResidencyClass(ResidencyClass::CLASS_D_HEAVY_COMPUTE);
    IosCoremlManager ios_t2(mgr_t2);
    ios_t2.handleBackgroundEntry();
    CHECK(mgr_t2.getState() == ModelState::UNLOADED,
          "iOS background: CLASS_D T2 checkpoints + unloads");

    // T1 (CLASS_B): suspends only (remains in IDLE for 30s window)
    ModelManager mgr_t1(profile);
    mgr_t1.loadModel("qwen-1.5b-q4", 720);
    mgr_t1.setResidencyClass(ResidencyClass::CLASS_B_WARM_HIBERNATED);
    IosCoremlManager ios_t1(mgr_t1);
    ios_t1.handleBackgroundEntry();
    // T1 goes to IDLE (suspended), not UNLOADED
    CHECK(mgr_t1.getState() == ModelState::IDLE,
          "iOS background: CLASS_B T1 suspends (not unloads)");

    // Background expiration: force unload even T1
    ios_t1.handleBackgroundExpiration();
    CHECK(mgr_t1.getState() == ModelState::UNLOADED,
          "iOS background expiration: T1 force-unloaded");
}

// ============================================================
// SECTION 15: RecommendationPolicy Gates + Token Caps
// ============================================================
void test_recommendation_policy() {
    std::cout << "\n== SECTION 15: RecommendationPolicy Gates + Token Caps ==\n";

    // Budget_4GB: max_output = T1_MAX_OUTPUT (384)
    auto d4 = ModelRecommendationPolicy::getRecommendation(DeviceRamClass::BUDGET_4GB);
    CHECK(d4.max_output_tokens == TokenBudgetPolicy::T1_MAX_OUTPUT,
          "BUDGET_4GB recommendation: max_output=384");
    CHECK(d4.thermal_threshold_c == ThermalPolicy::ZONE3_T1_ONLY,
          "BUDGET_4GB thermal gate = 44°C");

    // MIDRANGE_6GB: max_output = T1_MAX_OUTPUT
    auto d6 = ModelRecommendationPolicy::getRecommendation(DeviceRamClass::MIDRANGE_6GB);
    CHECK(d6.max_output_tokens == TokenBudgetPolicy::T1_MAX_OUTPUT,
          "MIDRANGE_6GB recommendation: max_output=384");

    // FLAGSHIP_8GB_PLUS: max_output = T2_MAX_OUTPUT (768 for T2 default)
    auto d8 = ModelRecommendationPolicy::getRecommendation(DeviceRamClass::FLAGSHIP_8GB_PLUS);
    CHECK(d8.max_output_tokens == TokenBudgetPolicy::T2_MAX_OUTPUT,
          "FLAGSHIP_8GB+ recommendation: max_output=768");
    CHECK(d8.thermal_threshold_c == ThermalPolicy::ZONE2_T2_DISABLE,
          "FLAGSHIP_8GB+ thermal gate = 42°C");

    // canLoadT1 gate methods on policy class
    CHECK(ModelRecommendationPolicy::canLoadT1(1500, 43.0, 20, false),
          "Policy canLoadT1: all conditions ok");
    CHECK(!ModelRecommendationPolicy::canLoadT1(1000, 43.0, 20, false),
          "Policy canLoadT1: low free_ram → deny");

    // canLoadT2: 4GB device denied
    CHECK(!ModelRecommendationPolicy::canLoadT2(4, 2200, 41.0, 25),
          "Policy canLoadT2: 4GB → deny");
    CHECK(ModelRecommendationPolicy::canLoadT2(6, 2500, 41.0, 25),
          "Policy canLoadT2: 6GB → allow");

    // canLoadT3: 8GB without charging → deny
    CHECK(!ModelRecommendationPolicy::canLoadT3(8, 6000, 34.0, 50, false),
          "Policy canLoadT3: 8GB no charge → deny");
    CHECK(ModelRecommendationPolicy::canLoadT3(8, 6000, 34.0, 50, true),
          "Policy canLoadT3: 8GB + charging → allow");
    CHECK(ModelRecommendationPolicy::canLoadT3(12, 6000, 34.0, 50, false),
          "Policy canLoadT3: 12GB no charge → allow (certified)");
}

// ============================================================
// MAIN
// ============================================================
int main() {
    std::cout << "================================================\n";
    std::cout << "MOBILE SURVIVABILITY RESIDENCY CLASS BENCHMARK\n";
    std::cout << "================================================\n";

    test_residency_class_enum();
    test_t1_load_gate();
    test_t2_load_gate();
    test_t3_load_gate();
    test_thermal_policy();
    test_model_manager_thermal_zones();
    test_t1_hibernation();
    test_heavy_compute_semaphore();
    test_t3_time_limit();
    test_class_a_emergency_protection();
    test_stt_tts_class_c();
    test_vlm_class_e_disabled();
    test_storage_floor_policy();
    test_ios_nuanced_background();
    test_recommendation_policy();

    std::cout << "\n================================================\n";
    std::cout << "SURVIVABILITY RESIDENCY CLASS RESULTS: "
              << g_pass << " PASS, " << g_fail << " FAIL\n";
    std::cout << "================================================\n";

    return (g_fail == 0) ? 0 : 1;
}
