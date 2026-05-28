#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cassert>
#include <memory>
#include <atomic>
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

void testQwen15BIntegration() {
    std::cout << "\n=== Test: Qwen 1.5B Integration & Scaling ===\n";

    std::string path = "models/llm/qwen2.5-1.5b-instruct-q4_k_m.gguf";
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        path = "models/llm/qwen2.5-0.5b-instruct-q4_k_m.gguf";
        std::cout << "  [WARNING] 1.5B model not found, falling back to 0.5B for integration test\n";
    } else {
        fclose(f);
    }

    // 1. Tier Gating: LOW tier must refuse Qwen 1.5B (MEDIUM model)
    {
        DeviceProfile low_profile{DeviceTier::LOW, 2048, 4, "Android"};
        LlamaCppEngine low_engine(low_profile);
        low_engine.setEmbeddingSizeOverride(1536); // Mock 1.5B model size (n_embd=1536)
        
        bool loaded = low_engine.loadModel(path);
        ASSERT_TRUE(!loaded, "LOW tier device correctly refuses to load 1.5B model");
    }

    // 2. Tier Gating: MID tier must accept Qwen 1.5B
    {
        DeviceProfile mid_profile{DeviceTier::MID, 6144, 8, "Android"};
        LlamaCppEngine engine(mid_profile);
        engine.setEmbeddingSizeOverride(1536); // Mock 1.5B model size
        
        bool loaded = engine.loadModel(path);
        ASSERT_TRUE(loaded, "MID tier device successfully loads 1.5B model");
        
        if (loaded) {
            // Verify dynamic n_batch scaling: 1.5B (MEDIUM size > 1024) on MID tier gets batch_size = 128
            auto snap = engine.kvTelemetry().snapshot();
            std::cout << "  Active batch size: " << snap.batch / 1024 << " KB\n";
            ASSERT_EQ(snap.batch, 128u * 1024u, "MID tier correctly restricts 1.5B batch size to 128");

            // Verify thread policy: large model (>1024) on MID tier gets 2 threads
            int expected_threads = InferenceThreadPolicy::computeThreads(mid_profile, 25.0, 1536);
            std::cout << "  Expected threads: " << expected_threads << "\n";
            ASSERT_TRUE(expected_threads <= 3, "Thread count is restricted within MID tier limits (2-3)");

            // Verify clean token generation
            auto token = std::make_shared<CancellationToken>();
            InferenceContext ctx("1.5b-test", token, 512);
            ctx.max_tokens_to_generate = 10;
            
            GenerationResult result = engine.generateEx("Explain the importance of safe runtime scaling.", ctx);
            ASSERT_TRUE(result.success, "Generation on 1.5B mock succeeds");
            ASSERT_TRUE(result.tokens_generated > 0, "Generated non-empty tokens");
            
            // Verify prefill chunking telemetry (n_batch=128, prompt length ~15 tokens, chunk count should be tracked)
            std::cout << "  Prefill chunks tracked: " << result.prefill_chunks << "\n";
            ASSERT_TRUE(result.prefill_chunks > 0, "Prefill chunks tracked");

            // Verify new telemetry fields
            snap = engine.kvTelemetry().snapshot();
            ASSERT_TRUE(snap.peak_cold_load > 0, "Phase-separated peak cold load memory is tracked");
            ASSERT_TRUE(snap.peak_context_init > 0, "Phase-separated peak context init memory is tracked");
            ASSERT_TRUE(snap.phase_mmap_init > 0.0f, "Phase mmap timing is tracked");
            ASSERT_TRUE(snap.phase_context_init > 0.0f, "Phase context init timing is tracked");
            ASSERT_TRUE(snap.prefill_lat > 0.0f, "Prefill latency is tracked separately");
            ASSERT_TRUE(snap.decode_tps > 0.0f, "Decode tokens per second is tracked separately");
            
            engine.unloadModel();
        }
    }
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "ISHA AI — QWEN 1.5B INTEGRATION & SCALING BENCHMARK\n";
    std::cout << "==================================================\n";

    testQwen15BIntegration();

    std::cout << "\n==================================================\n";
    std::cout << "QWEN 1.5B INTEGRATION BENCHMARK: " << g_pass << " PASS, " << g_fail << " FAIL\n";
    std::cout << "==================================================\n";

    return g_fail > 0 ? 1 : 0;
}
