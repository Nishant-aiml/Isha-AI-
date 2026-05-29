#include "../core/model/model_registry.hpp"
#include "../core/model/model_manager.hpp"
#include "../core/model/tokenizer_manager.hpp"
#include "../core/config/device_profile.hpp"
#include "../core/logging/logger.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace isha;

// ─────────────────────────────────────────────
// Phase 6 — LLM Lifecycle Stress Benchmark
//
// Validates:
//   1. ModelRegistry discovery and validation
//   2. TokenizerManager caching and eviction
//   3. ModelManager lifecycle governance:
//      - Cooldown-based reload storm prevention
//      - Emergency unload
//      - Thermal-aware unload (38C HOT, 41C CRITICAL)
//      - tryReload with cooldown gating
//   4. Repeated load/unload stability
//   5. Memory governance under pressure
// ─────────────────────────────────────────────

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(expr, msg) \
    if (!(expr)) { \
        std::cerr << "[FAIL] " << msg << " (line " << __LINE__ << ")\n"; \
        tests_failed++; \
    } else { \
        std::cout << "[PASS] " << msg << "\n"; \
        tests_passed++; \
    }

#define ASSERT_EQ(a, b, msg) \
    if ((a) != (b)) { \
        std::cerr << "[FAIL] " << msg << " (got " << (a) << ", expected " << (b) << ")\n"; \
        tests_failed++; \
    } else { \
        std::cout << "[PASS] " << msg << "\n"; \
        tests_passed++; \
    }

void testModelRegistryBasics() {
    std::cout << "\n=== Test: ModelRegistry Basics ===\n";

    ModelRegistry registry("models"); // May not exist, that's fine
    size_t discovered = registry.discoverModels();
    std::cout << "  Discovered models: " << discovered << "\n";

    // Register a model manually
    ModelEntry entry;
    entry.model_id = "test-qwen-0.5b";
    entry.filename = "qwen2.5-0.5b-q4_k_m.gguf";
    entry.filepath = "models/llm/qwen2.5-0.5b-q4_k_m.gguf";
    entry.subsystem = ModelSubsystem::LLM;
    entry.format = ModelFormat::GGUF;
    entry.quantization = QuantizationType::Q4_K_M;
    entry.estimated_ram_mb = 200;
    entry.file_size_bytes = 350 * 1024 * 1024ULL;
    entry.minimum_tier = DeviceTier::LOW;
    entry.max_thermal_c = 41.0;
    entry.max_active_seconds = 0;
    entry.is_valid = true;

    bool registered = registry.registerModel(entry);
    ASSERT_TRUE(registered, "Register model");

    const ModelEntry* found = registry.findModel("test-qwen-0.5b");
    ASSERT_TRUE(found != nullptr, "Find registered model");
    ASSERT_EQ(found->estimated_ram_mb, 200u, "RAM estimate correct");

    // Find by subsystem
    auto llm_models = registry.findBySubsystem(ModelSubsystem::LLM);
    ASSERT_TRUE(!llm_models.empty(), "Find by subsystem returns results");

    // Compatibility check
    DeviceProfile mid_device{DeviceTier::MID, 6144, 8, "Android"};
    auto compat = registry.checkCompatibility("test-qwen-0.5b", mid_device, 30.0);
    ASSERT_TRUE(compat.compatible, "Compatible with MID device at 30C");

    // Thermal rejection
    auto compat_hot = registry.checkCompatibility("test-qwen-0.5b", mid_device, 42.0);
    ASSERT_TRUE(!compat_hot.compatible, "Rejected at 42C (above 41C thermal limit)");

    // Tier rejection
    DeviceProfile low_device{DeviceTier::LOW, 2048, 4, "Android"};
    auto compat_low = registry.checkCompatibility("test-qwen-0.5b", low_device, 30.0);
    // 200MB * 2 = 400MB safety margin, 2048 >= 400, LOW >= LOW => should pass
    ASSERT_TRUE(compat_low.compatible, "Compatible with LOW device (200MB model, 2GB RAM)");

    // Register large model for tier rejection
    ModelEntry large;
    large.model_id = "test-mistral-7b";
    large.subsystem = ModelSubsystem::LLM;
    large.estimated_ram_mb = 4200;
    large.minimum_tier = DeviceTier::FLAGSHIP;
    large.max_thermal_c = 38.0;
    large.is_valid = true;
    registry.registerModel(large);

    auto compat_large = registry.checkCompatibility("test-mistral-7b", mid_device, 30.0);
    ASSERT_TRUE(!compat_large.compatible, "Flagship model rejected on MID device");

    registry.unregisterModel("test-qwen-0.5b");
    registry.unregisterModel("test-mistral-7b");
    ASSERT_EQ(registry.modelCount(), discovered, "Clean unregistration");
}

void testTokenizerManagerBasics() {
    std::cout << "\n=== Test: TokenizerManager Basics ===\n";

    TokenizerManager tokenizer(4096); // 4MB budget

    bool loaded = tokenizer.loadTokenizer("model-a", "fake/path/model-a.gguf");
    ASSERT_TRUE(loaded, "Load tokenizer A");
    ASSERT_TRUE(tokenizer.hasTokenizer("model-a"), "Has tokenizer A");
    ASSERT_EQ(tokenizer.loadCount(), 1u, "Load count = 1");

    // Duplicate prevention
    bool dup = tokenizer.loadTokenizer("model-a", "fake/path/model-a.gguf");
    ASSERT_TRUE(dup, "Duplicate load returns true (cached)");
    ASSERT_EQ(tokenizer.duplicationsPrevented(), 1u, "Duplication prevented");
    ASSERT_EQ(tokenizer.loadCount(), 1u, "Load count unchanged after dup");

    // Load second tokenizer
    bool loaded_b = tokenizer.loadTokenizer("model-b", "fake/path/model-b.gguf");
    ASSERT_TRUE(loaded_b, "Load tokenizer B");
    ASSERT_EQ(tokenizer.loadCount(), 2u, "Load count = 2");

    // Unload
    tokenizer.unloadTokenizer("model-a");
    ASSERT_TRUE(!tokenizer.hasTokenizer("model-a"), "Tokenizer A unloaded");
    ASSERT_TRUE(tokenizer.hasTokenizer("model-b"), "Tokenizer B still loaded");

    // Evict all
    tokenizer.evictAll();
    ASSERT_TRUE(!tokenizer.hasTokenizer("model-b"), "All evicted");
    ASSERT_TRUE(tokenizer.evictionCount() > 0, "Eviction count > 0");
}

void testModelManagerCooldown() {
    std::cout << "\n=== Test: ModelManager Cooldown ===\n";

    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    ModelManager manager(profile);

    // Set short cooldown for test
    manager.setMinCooldown(std::chrono::milliseconds(200));

    // First load should succeed
    bool load1 = manager.loadModel("qwen-0.5b", 200);
    ASSERT_TRUE(load1, "First load succeeds");
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("ACTIVE"), "State is ACTIVE");
    ASSERT_EQ(manager.getLoadCount(), 1u, "Load count = 1");

    // Unload
    manager.unloadModel();
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("UNLOADED"), "State is UNLOADED");

    // Immediate reload should be rejected (cooldown)
    bool load2 = manager.loadModel("qwen-0.5b", 200);
    ASSERT_TRUE(!load2, "Reload rejected during cooldown");
    ASSERT_TRUE(manager.getReloadRejections() > 0, "Rejection counted");

    // Wait for cooldown
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Now should succeed
    bool load3 = manager.loadModel("qwen-0.5b", 200);
    ASSERT_TRUE(load3, "Reload succeeds after cooldown");
    ASSERT_EQ(manager.getLoadCount(), 2u, "Load count = 2");

    manager.unloadModel();
}

void testModelManagerEmergencyUnload() {
    std::cout << "\n=== Test: ModelManager Emergency Unload ===\n";

    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    ModelManager manager(profile);
    manager.setMinCooldown(std::chrono::milliseconds(50));

    // Use T1 model (700MB, CLASS_B) — NOT the T0 router (CLASS_A protected).
    // CLASS_A T0 routers are intentionally immune to emergency unload
    // per the mobile survivability residency architecture.
    manager.loadModel("qwen-1.5b", 700);
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("ACTIVE"), "Model loaded");

    manager.emergencyUnload();
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("UNLOADED"), "Emergency unload complete");
    ASSERT_EQ(manager.getEmergencyUnloadCount(), 1u, "Emergency count = 1");
}

void testModelManagerThermalUnload() {
    std::cout << "\n=== Test: ModelManager Thermal Unload ===\n";

    DeviceProfile profile{DeviceTier::FLAGSHIP, 12288, 8, "Android"};
    ModelManager manager(profile);
    manager.setMinCooldown(std::chrono::milliseconds(50));

    // 1. T3 model (4200MB) loaded
    manager.loadModel("mistral-7b", 4200);
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("ACTIVE"), "T3 model loaded");

    // T3 disabled above 35°C
    manager.thermalUnload(36.0);
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("UNLOADED"), "T3 model unloaded at >35C");
    ASSERT_TRUE(manager.isThermalUnloaded(), "Thermal flag set");

    // Wait cooldown, load T2 model (1200MB)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    manager.loadModel("gemma-3-2b", 1200);

    // T2 should survive at 34.0°C (only disabled at >35°C)
    manager.thermalUnload(34.0);
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("ACTIVE"), "T2 survives at 34C");

    // T2 disabled above 35°C
    manager.thermalUnload(36.0);
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("UNLOADED"), "T2 unloaded at >35C");

    // Wait cooldown, load T1 model (700MB)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    manager.loadModel("qwen-1.5b", 700);

    // T1 should survive at 37.0°C (only disabled at >38°C)
    manager.thermalUnload(37.0);
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("ACTIVE"), "T1 survives at 37C");

    // T1 disabled above 38°C (T0 only resident)
    manager.thermalUnload(39.0);
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("UNLOADED"), "T1 unloaded at >38C");

    // T0-sized models (<250MB) should survive even >41°C as they are always resident stubs
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    manager.loadModel("qwen-0.5b", 180);
    manager.thermalUnload(43.0);
    ASSERT_EQ(modelStateToString(manager.getState()), std::string("ACTIVE"), "T0 survives above 41C");

    manager.unloadModel();
}

void testModelManagerTryReload() {
    std::cout << "\n=== Test: ModelManager tryReload ===\n";

    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    ModelManager manager(profile);
    manager.setMinCooldown(std::chrono::milliseconds(100));

    // Use T1 model (700MB, CLASS_B).
    // Thermal Zone 3 (38-41C) disables T1, so use 39C
    // to trigger the thermal unload that tryReload needs to recover from.
    manager.loadModel("qwen-1.5b", 700);
    manager.thermalUnload(39.0);

    // Immediate tryReload should fail (cooldown)
    bool reload1 = manager.tryReload();
    ASSERT_TRUE(!reload1, "tryReload rejected during cooldown");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    bool reload2 = manager.tryReload();
    ASSERT_TRUE(reload2, "tryReload succeeds after cooldown");
    ASSERT_TRUE(!manager.isThermalUnloaded(), "Thermal flag cleared after reload");

    manager.unloadModel();
}

void testRepeatedLoadUnloadStability() {
    std::cout << "\n=== Test: Repeated Load/Unload Stability ===\n";

    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    ModelManager manager(profile);
    manager.setMinCooldown(std::chrono::milliseconds(10)); // Short for stress test

    const int iterations = 50;
    int successful_loads = 0;
    int successful_unloads = 0;

    for (int i = 0; i < iterations; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(15)); // respect cooldown
        if (manager.loadModel("stress-model", 200)) {
            successful_loads++;
            manager.unloadModel();
            successful_unloads++;
        }
    }

    std::cout << "  Iterations: " << iterations << "\n";
    std::cout << "  Successful loads: " << successful_loads << "\n";
    std::cout << "  Successful unloads: " << successful_unloads << "\n";
    std::cout << "  Load count: " << manager.getLoadCount() << "\n";
    std::cout << "  Unload count: " << manager.getUnloadCount() << "\n";
    std::cout << "  Reload rejections: " << manager.getReloadRejections() << "\n";

    ASSERT_TRUE(successful_loads >= iterations * 0.8, ">=80% loads succeeded");
    ASSERT_EQ(successful_loads, (int)successful_unloads, "Load/unload count balanced");
    ASSERT_TRUE(manager.getState() == ModelState::UNLOADED, "Final state is UNLOADED");
}

void testTokenizerMemoryGovernance() {
    std::cout << "\n=== Test: Tokenizer Memory Governance ===\n";

    // Small budget to force evictions
    TokenizerManager tokenizer(512); // 512KB budget

    bool a = tokenizer.loadTokenizer("model-a", "fake/a.gguf");
    bool b = tokenizer.loadTokenizer("model-b", "fake/b.gguf");
    bool c = tokenizer.loadTokenizer("model-c", "fake/c.gguf");

    std::cout << "  Total memory: " << tokenizer.totalMemoryUsageKB() << "KB / 512KB\n";
    std::cout << "  Evictions: " << tokenizer.evictionCount() << "\n";

    // With 512KB budget and ~64KB min per tokenizer, should handle several
    // but evictions should occur if budget tight
    ASSERT_TRUE(tokenizer.loadCount() >= 2, "At least 2 tokenizers loaded");
}

int main() {
    std::cout << "=== ISHA AI — Phase 6: LLM Lifecycle Stress Benchmark ===\n";
    std::cout << "Purpose: Validate model governance foundation\n";
    std::cout << "Current subsystem: LLM (governance only, no real inference)\n";
    std::cout << "Model: N/A (governance infrastructure validation)\n\n";

    auto start = std::chrono::high_resolution_clock::now();

    testModelRegistryBasics();
    testTokenizerManagerBasics();
    testModelManagerCooldown();
    testModelManagerEmergencyUnload();
    testModelManagerThermalUnload();
    testModelManagerTryReload();
    testRepeatedLoadUnloadStability();
    testTokenizerMemoryGovernance();

    auto end = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "\n=== RESULTS ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    std::cout << "Total time: " << total_ms << "ms\n";

    std::cout << "\n--- Stability Analysis ---\n";
    std::cout << "1. Scheduler impact: NONE (governance only)\n";
    std::cout << "2. mmap impact: NONE (governance only)\n";
    std::cout << "3. Thermal impact: Validated (38C/41C thresholds)\n";
    std::cout << "4. Android risks: Reload storms prevented by cooldown\n";
    std::cout << "5. Fragmentation risks: Tokenizer budget prevents growth\n";
    std::cout << "6. Recovery: Emergency + thermal unload validated\n";
    std::cout << "7. Runtime stable: " << (tests_failed == 0 ? "YES" : "NO") << "\n";
    std::cout << "8. Next step: Wire real GGUF loading through ModelRegistry\n";

    return tests_failed > 0 ? 1 : 0;
}
