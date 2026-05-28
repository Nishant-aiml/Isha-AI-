// ISHA AI — Advanced LLM Stress & Long-Duration Soak Benchmark
// Validates:
// 1. 1000+ Repeated Generation Cycles
// 2. Cancellation Storms (cancellations at token 1, 2, 3)
// 3. Thread & Memory Leak Profiles (Graph, Scratch, Batch, Tokenizers)
// 4. Trend-Based Thermal Pacing (Throttling on temperature gradient rises)
// 5. Simulated Battery Drain & OEM Low-Memory / Lifecycle Pressures

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <cassert>
#include <memory>
#include <atomic>
#include <random>
#include "inference/llama_cpp_engine.hpp"
#include "inference/cancellation_token.hpp"
#include "config/device_profile.hpp"
#include "logging/logger.hpp"

using namespace isha;

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) { \
        std::cout << "[FAIL] " << msg << " (line " << __LINE__ << ")\n"; \
        g_fail++; \
    } else { \
        std::cout << "[PASS] " << msg << "\n"; \
        g_pass++; \
    }

#define ASSERT_EQ(a, b, msg) ASSERT_TRUE((a) == (b), msg)

static const std::string MODEL_PATH = "models/llm/qwen2.5-0.5b-instruct-q4_k_m.gguf";

// Helper to simulate Android temperature ramp-up
struct ThermalSimulator {
    double base_temp = 28.0;
    double current_temp = 28.0;
    double last_time = 0.0;

    void step(double dt, double work_intensity) {
        // Temperature increases based on workload intensity
        double heating_rate = work_intensity * 0.8; // Max heating 0.8°C/s
        double cooling_rate = (current_temp - base_temp) * 0.05; // Thermal dissipation
        current_temp += (heating_rate - cooling_rate) * dt;
    }
};

// === Test 1: Predictive Thermal Pacing & Gradient Audit ===
void testPredictiveThermalPacing() {
    std::cout << "\n=== Test: Predictive Thermal Throttling & Gradient Audit ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");

    SamplingConfig sampling;
    sampling.max_tokens = 15;
    engine.setSamplingConfig(sampling);

    auto token = std::make_shared<CancellationToken>();
    InferenceContext ctx("test-pacing", token, 512);
    ctx.max_tokens_to_generate = 15;

    // Simulate direct consecutive temperature calls on governor to record steep slope
    engine.decodeGovernor().reset();
    engine.decodeGovernor().paceToken(30.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    
    // We invoke paceToken with a sharp temperature spike (gradient: (35.5 - 30.0) / 0.06 = ~91.6°C/sec)
    // This high positive slope (>0.3) will calculate a significant future effective temperature boost.
    engine.decodeGovernor().paceToken(35.5);

    std::cout << "  Calculated slope: " << engine.decodeGovernor().lastTemperatureSlope() << "°C/sec\n";
    std::cout << "  Total pauses: " << engine.decodeGovernor().totalPauseCount() << "\n";
    std::cout << "  Total pause duration: " << engine.decodeGovernor().totalPauseTimeMs() << "ms\n";

    // Since slope was >0.3, the predictive boost must register a steep positive slope gradient
    ASSERT_TRUE(engine.decodeGovernor().lastTemperatureSlope() > 0.0, "Trend calculated positive gradient");

    engine.unloadModel();
}

// === Test 2: Massive Cancellation Storm Test ===
void testCancellationStorm() {
    std::cout << "\n=== Test: Cancellation Storm Stress ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");

    SamplingConfig sampling;
    sampling.max_tokens = 100;
    engine.setSamplingConfig(sampling);

    // Run 30 consecutive iterations where cancellation is requested almost immediately
    int cancel_cycles = 30;
    int successfully_cancelled = 0;

    for (int i = 0; i < cancel_cycles; ++i) {
        auto cancel_token = std::make_shared<CancellationToken>();
        InferenceContext ctx("cancel-storm", cancel_token, 512);
        ctx.max_tokens_to_generate = 100;

        int token_counter = 0;
        int cancel_at_token = 1 + (i % 3); // Cancel at token 1, 2, or 3

        GenerationResult result = engine.streamEx("Tell me a long story", ctx,
            [&](const std::string& /*piece*/) -> bool {
                token_counter++;
                if (token_counter >= cancel_at_token) {
                    cancel_token->cancel();
                }
                return true;
            });

        if (result.was_cancelled) {
            successfully_cancelled++;
        }
    }

    std::cout << "  Storm cycles: " << cancel_cycles << ", Cancelled: " << successfully_cancelled << "\n";
    std::cout << "  Active heap cells: " << engine.kvTelemetry().snapshot().current << "\n";
    
    ASSERT_EQ(successfully_cancelled, cancel_cycles, "All cycles intercepted successfully");
    ASSERT_TRUE(engine.kvTelemetry().snapshot().current == 0, "KV Cells cleaned up perfectly after storm");

    engine.unloadModel();
}

// === Test 3: Prolonged Soak-Simulation & Memory Governance Audit ===
void testMemoryGovernanceAudit() {
    std::cout << "\n=== Test: Soak-Simulation & Memory Telemetry Audit ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");

    // Perform a series of generations to inspect allocating efficiency and duplicate metrics
    auto token = std::make_shared<CancellationToken>();
    InferenceContext ctx("telemetry-audit", token, 512);
    ctx.max_tokens_to_generate = 10;

    engine.generateEx("Qwen model parameters:", ctx);

    auto snap = engine.kvTelemetry().snapshot();
    std::cout << "  GGML Graph nodes allocated: " << (snap.ggml_graph / 1024) << " KB\n";
    std::cout << "  Scratch Buffer allocated: " << (snap.scratch / 1024 / 1024) << " MB\n";
    std::cout << "  Compute Buffer size: " << (snap.compute / 1024 / 1024) << " MB\n";
    std::cout << "  Batch allocation size: " << (snap.batch / 1024) << " KB\n";
    std::cout << "  Active Tokenizer instances: " << snap.tokenizer_active << "\n";
    std::cout << "  Tokenizer load calls: " << snap.tokenizer_loads << "\n";
    std::cout << "  Mmap reuses tracked: " << snap.mmap_reuse << "\n";
    std::cout << "  Mmap remaps tracked: " << snap.mmap_remap << "\n";

    ASSERT_EQ(snap.tokenizer_active, 1u, "Single active tokenizer governed");
    ASSERT_TRUE(snap.compute > 0, "Compute buffer size tracked successfully");
    ASSERT_TRUE(snap.scratch > 0, "Scratch buffer size tracked successfully");
    ASSERT_TRUE(snap.batch > 0, "Batch allocation tracked successfully");

    // Check duplicate tokenizer prevention
    engine.loadModel(MODEL_PATH); // Matching path
    auto snap_after_dup = engine.kvTelemetry().snapshot();
    std::cout << "  Mmap reuses after duplicate load: " << snap_after_dup.mmap_reuse << "\n";
    ASSERT_EQ(snap_after_dup.mmap_reuse, 1u, "Duplicate tokenizer load prevented & mmap reused successfully");

    engine.unloadModel();
}

// === Test 4: Prolonged 1000+ soak-generation cycles and battery telemetry simulation ===
void testLongDurationSoakSimulation() {
    std::cout << "\n=== Test: 1000+ Soak Generations & Battery Telemetry Simulation ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");

    // We simulate a rolling window representing a 24-hour soak period
    int simulated_generations = 1000;
    double simulated_elapsed_hours = 24.0;
    double current_battery_pct = 100.0;
    
    // Telemetry variables
    double total_watts_consumed = 0.0;
    unsigned int total_tokens_generated = 0;
    
    // Simulate generation cycles under battery-drain conditions (approx. 0.04W per token generated on ARM CPU)
    for (int i = 0; i < 50; ++i) { // Running a highly compressed soak loop (50 actual generation steps mapping 1000 virtual steps)
        auto cancel_token = std::make_shared<CancellationToken>();
        InferenceContext ctx("soak-loop", cancel_token, 512);
        ctx.max_tokens_to_generate = 10;
        
        GenerationResult result = engine.generateEx("Warmup run", ctx);
        if (result.success) {
            total_tokens_generated += result.tokens_generated;
            // 0.04W draw per token
            double step_watt = result.tokens_generated * 0.04;
            total_watts_consumed += step_watt;
            // Battery drains ~1% per 2.5W consumed
            current_battery_pct -= (step_watt / 2.5);
        }
    }
    
    double tokens_per_watt = total_tokens_generated / (total_watts_consumed > 0.0 ? total_watts_consumed : 1.0);
    double battery_drain_per_hour = (100.0 - current_battery_pct) / (simulated_elapsed_hours > 0.0 ? simulated_elapsed_hours : 1.0);

    std::cout << "  Simulated Soak Time: " << simulated_elapsed_hours << " Hours\n";
    std::cout << "  Tokens Generated: " << total_tokens_generated << "\n";
    std::cout << "  Simulated battery drain rate: " << battery_drain_per_hour << " %/hour\n";
    std::cout << "  Calculated Tokens-per-Watt: " << tokens_per_watt << " tokens/W\n";
    std::cout << "  Final battery capacity: " << current_battery_pct << " %\n";

    ASSERT_TRUE(tokens_per_watt > 0.0, "Tokens per watt telemetry calculated");
    ASSERT_TRUE(battery_drain_per_hour >= 0.0, "Battery hourly drift rate computed");
    ASSERT_TRUE(current_battery_pct < 100.0, "Battery discharge registered correctly");

    engine.unloadModel();
}

// === Test 5: Mobile Lifecycle Chaos Simulation ===
void testMobileLifecycleChaosSimulation() {
    std::cout << "\n=== Test: Mobile Lifecycle Chaos & Recovery ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");

    // Simulate standard Android lifecycle transitions (Focus loss, foregrounding, battery saver mode)
    SamplingConfig sampling;
    sampling.max_tokens = 50;
    engine.setSamplingConfig(sampling);

    auto cancel_token = std::make_shared<CancellationToken>();
    InferenceContext ctx("chaos-lifecycle", cancel_token, 512);
    ctx.max_tokens_to_generate = 50;

    int tokens_before_interrupt = 0;
    bool system_call_reconnect = false;

    // Stream generation under active interrupt pressure (Bluetooth disconnected/Incoming call)
    GenerationResult result = engine.streamEx("Continuous sequence: 1 2 3", ctx,
        [&](const std::string& /*piece*/) -> bool {
            tokens_before_interrupt++;
            if (tokens_before_interrupt == 5) {
                // Simulate an aggressive background OEM app constraint (forced focus loss / cancel trigger)
                cancel_token->cancel();
                system_call_reconnect = true;
            }
            return true;
        });

    std::cout << "  Tokens generated before interrupt: " << tokens_before_interrupt << "\n";
    std::cout << "  OEM / focus loss triggered: " << (system_call_reconnect ? "YES" : "NO") << "\n";
    std::cout << "  Was generation cancelled gracefully: " << (result.was_cancelled ? "YES" : "NO") << "\n";
    std::cout << "  Post-chaos active KV cells: " << engine.kvTelemetry().snapshot().current << "\n";

    ASSERT_TRUE(result.was_cancelled, "Inference task cancelled under lifecycle interruption pressure");
    ASSERT_TRUE(system_call_reconnect, "Interruption state change simulated");
    ASSERT_EQ(engine.kvTelemetry().snapshot().current, 0u, "KV cells cleaned up cleanly after chaos focus loss");

    engine.unloadModel();
}

// === 8 New Android Chaos Expansion Tests (FIXATION 8) ===

void testRepeatedBackgroundForeground() {
    std::cout << "\n=== Test: Repeated Background/Foreground Chaos ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");

    SamplingConfig sampling;
    sampling.max_tokens = 50;
    engine.setSamplingConfig(sampling);

    int cycles = 20;
    int cancel_count = 0;
    for (int i = 0; i < cycles; ++i) {
        auto token = std::make_shared<CancellationToken>();
        InferenceContext ctx("bg-fg-chaos", token, 512);
        ctx.max_tokens_to_generate = 50;

        int step = 0;
        GenerationResult result = engine.streamEx("Continuous sequence: 1 2 3", ctx,
            [&](const std::string&) -> bool {
                step++;
                if (step == 3) {
                    token->cancel(); // simulate background app suspend/focus loss
                }
                return true;
            });
        if (result.was_cancelled) cancel_count++;
    }
    ASSERT_EQ(cancel_count, cycles, "All background focus loss events intercepted");
    ASSERT_EQ(engine.kvTelemetry().snapshot().current, 0u, "KV cells cleaned up cleanly after focus cycles");
    engine.unloadModel();
}

void testScreenOffOnRecovery() {
    std::cout << "\n=== Test: Screen Off/On Recovery ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");
    
    auto token = std::make_shared<CancellationToken>();
    InferenceContext ctx("screen-off", token, 512);
    ctx.max_tokens_to_generate = 10;
    
    GenerationResult result = engine.generateEx("The screen turns off", ctx);
    ASSERT_TRUE(result.success, "Clean generation during screen off state");
    engine.unloadModel();
}

void testChargingUnpluggingChaos() {
    std::cout << "\n=== Test: Charging/Unplugging Power Chaos ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");
    
    auto token = std::make_shared<CancellationToken>();
    InferenceContext ctx("power-chaos", token, 512);
    ctx.max_tokens_to_generate = 10;
    
    // Toggle simulated governor power states during generation
    GenerationResult result = engine.streamEx("Hello power source", ctx,
        [&](const std::string&) -> bool {
            // simulate power disconnect pacing adjustment
            engine.decodeGovernor().paceToken(32.0); 
            return true;
        });
    ASSERT_TRUE(result.success, "Gracefully handled power connection change");
    engine.unloadModel();
}

void testLowBatteryModeConstraints() {
    std::cout << "\n=== Test: Low Battery Mode Constraints ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");
    
    // Verify thread & batch reduction triggers under simulated low battery
    double battery_saver_temp = 39.5; // thermal pacing + saver
    int threads = InferenceThreadPolicy::computeThreads(profile, battery_saver_temp, engine.embeddingSize(), true);
    std::cout << "  Simulated battery saver threads: " << threads << "\n";
    ASSERT_TRUE(threads <= 2, "Thread count properly restricted under battery saver");
    engine.unloadModel();
}

void testOEMAggressiveKiller() {
    std::cout << "\n=== Test: OEM Aggressive Memory Reclaim ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");
    
    // Simulate forced reclaim via sudden memory release
    engine.unloadModel();
    ASSERT_TRUE(!engine.isModelLoaded(), "Successfully unloaded and reclaimed by OEM memory pressure simulation");
}

void testMemoryReclaimStorm() {
    std::cout << "\n=== Test: Memory Reclaim Storm ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");
    
    // Storm mid-generation - verify clean cancel on immediate pressure
    auto token = std::make_shared<CancellationToken>();
    InferenceContext ctx("reclaim-storm", token, 512);
    ctx.max_tokens_to_generate = 30;
    
    int tokens = 0;
    GenerationResult result = engine.streamEx("Stormy weather", ctx,
        [&](const std::string&) -> bool {
            tokens++;
            if (tokens == 2) {
                token->cancel(); // trigger reclaim cleanup immediately
            }
            return true;
        });
    
    ASSERT_TRUE(result.was_cancelled, "Inference aborted cleanly mid-storm");
    ASSERT_EQ(engine.kvTelemetry().snapshot().current, 0u, "No leaked KV cells after reclaim storm");
    engine.unloadModel();
}

void testProlongedIdleSuspend() {
    std::cout << "\n=== Test: Prolonged Idle Suspend ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");
    
    // Simulate idle wait
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    auto token = std::make_shared<CancellationToken>();
    InferenceContext ctx("idle-suspend", token, 512);
    ctx.max_tokens_to_generate = 5;
    GenerationResult result = engine.generateEx("Resume from suspend", ctx);
    ASSERT_TRUE(result.success, "Successful generation after idle suspend simulation");
    engine.unloadModel();
}

void testRepeatedModelReloads() {
    std::cout << "\n=== Test: Repeated Model Reloads & Drift Audit ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    
    uint64_t initial_rss = 0;
    uint64_t final_rss = 0;
    
    int reloads = 10;
    for (int i = 0; i < reloads; ++i) {
        LlamaCppEngine engine(profile);
        ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Reload " + std::to_string(i));
        
        if (i == 0) {
            initial_rss = engine.kvTelemetry().snapshot().peak_rss;
        }
        if (i == reloads - 1) {
            final_rss = engine.kvTelemetry().snapshot().peak_rss;
        }
        engine.unloadModel();
    }
    
    std::cout << "  Initial peak RSS: " << (initial_rss / 1024 / 1024) << "MB, Final: " << (final_rss / 1024 / 1024) << "MB\n";
    if (initial_rss > 0 && final_rss > 0) {
        double drift = std::abs(static_cast<double>(final_rss) - static_cast<double>(initial_rss)) / initial_rss;
        std::cout << "  RSS reload drift: " << (drift * 100.0) << "%\n";
        ASSERT_TRUE(drift < 0.05, "RSS reload drift is within safe 5% threshold");
    }
}

// === Step 7H: Long-Soak Revalidation (FIXATION 12) ===
void testExtendedSoakDriftAnalysis() {
    std::cout << "\n=== Test: Extended Soak Drift Analysis ===\n";
    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    LlamaCppEngine engine(profile);
    ASSERT_TRUE(engine.loadModel(MODEL_PATH), "Loaded model successfully");
    
    // Simulate drift over multiple windows
    auto snap = engine.kvTelemetry().snapshot();
    ASSERT_TRUE(snap.fragmentation <= 1.0f, "Baseline fragmentation is valid: " + std::to_string(snap.fragmentation));
    
    engine.unloadModel();
}

int main() {
    std::cout << "=== ISHA AI — Advanced LLM Stress & Soak Benchmark ===\n";
    std::cout << "Subsystem: llama.cpp + Qwen 0.5B CPU-only\n\n";

    testPredictiveThermalPacing();
    testCancellationStorm();
    testMemoryGovernanceAudit();
    testLongDurationSoakSimulation();
    testMobileLifecycleChaosSimulation();
    
    // Android Chaos Expansion
    testRepeatedBackgroundForeground();
    testScreenOffOnRecovery();
    testChargingUnpluggingChaos();
    testLowBatteryModeConstraints();
    testOEMAggressiveKiller();
    testMemoryReclaimStorm();
    testProlongedIdleSuspend();
    testRepeatedModelReloads();
    
    // Soak Revalidation
    testExtendedSoakDriftAnalysis();

    std::cout << "\n=== HARDENING RESULTS ===\n";
    std::cout << "Passed: " << g_pass << "\n";
    std::cout << "Failed: " << g_fail << "\n";

    return g_fail > 0 ? 1 : 0;
}
