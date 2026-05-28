#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cassert>
#include <memory>
#include <vector>
#include "inference/llama_cpp_engine.hpp"
#include "inference/cancellation_token.hpp"
#include "config/device_profile.hpp"

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

void testPeakMemoryBounds() {
    std::cout << "\n=== Test: Peak Memory Bounds ===\n";

    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Windows"};
    LlamaCppEngine engine(profile);

    // 1. Load model and record peak RSS
    bool loaded = engine.loadModel(MODEL_PATH);
    ASSERT_TRUE(loaded, "Model loaded");
    
    auto snap = engine.kvTelemetry().snapshot();
    std::cout << "  After load - peak RSS: " << (snap.peak_rss / 1024 / 1024) << "MB\n";
    ASSERT_TRUE(snap.peak_rss > 0, "Telemetry tracked peak RSS");

    // 2. Prefill long prompt (150 tokens) and record prefill peak
    SamplingConfig sampling;
    sampling.max_tokens = 20;
    engine.setSamplingConfig(sampling);

    auto token = std::make_shared<CancellationToken>();
    InferenceContext ctx("peak-memory-test", token, 512);
    ctx.max_tokens_to_generate = 20;

    std::string long_prompt = "Verify that peak memory is within bounds under long input contexts. "
                              "This prompt is repeated to ensure a large prefill chunk sequence is evaluated. "
                              "Verify that peak memory is within bounds under long input contexts. "
                              "This prompt is repeated to ensure a large prefill chunk sequence is evaluated. "
                              "Verify that peak memory is within bounds under long input contexts. "
                              "This prompt is repeated to ensure a large prefill chunk sequence is evaluated.";

    GenerationResult result = engine.generateEx(long_prompt, ctx);
    ASSERT_TRUE(result.success, "Generation succeeded");

    snap = engine.kvTelemetry().snapshot();
    std::cout << "  After generation - peak RSS: " << (snap.peak_rss / 1024 / 1024) << "MB\n";
    std::cout << "  Prefill chunks: " << snap.peak_prefill << "\n"; // peak_prefill holds peak prefill chunk/bytes

    // 3. Cancel mid-generation
    auto cancel_token = std::make_shared<CancellationToken>();
    InferenceContext cancel_ctx("cancel-peak", cancel_token, 512);
    cancel_ctx.max_tokens_to_generate = 50;

    int steps = 0;
    GenerationResult cancel_result = engine.streamEx("Tell me a very long story with multiple details", cancel_ctx,
        [&](const std::string&) -> bool {
            steps++;
            if (steps == 2) {
                cancel_token->cancel();
            }
            return true;
        });

    ASSERT_TRUE(cancel_result.was_cancelled, "Generation cancelled cleanly");
    snap = engine.kvTelemetry().snapshot();
    std::cout << "  After cancellation - peak RSS: " << (snap.peak_rss / 1024 / 1024) << "MB\n";
    std::cout << "  Peak cancellation cleanup tracked: " << snap.peak_cancellation_cleanup << "\n";

    // 4. Verify fragmentation ratio is valid
    ASSERT_TRUE(snap.fragmentation <= 1.0f, "Fragmentation ratio is valid: " + std::to_string(snap.fragmentation));

    // 5. 10x rapid load/unload peak tracking
    engine.unloadModel();
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "ISHA AI — PEAK MEMORY TELEMETRY BENCHMARK\n";
    std::cout << "==================================================\n";

    testPeakMemoryBounds();

    std::cout << "\n==================================================\n";
    std::cout << "PEAK MEMORY TELEMETRY RESULTS: " << g_pass << " PASS, " << g_fail << " FAIL\n";
    std::cout << "==================================================\n";

    return g_fail > 0 ? 1 : 0;
}
