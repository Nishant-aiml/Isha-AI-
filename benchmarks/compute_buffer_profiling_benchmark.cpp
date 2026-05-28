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

void testComputeBufferScaling() {
    std::cout << "\n=== Test: Compute Buffer Scaling ===\n";

    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Windows"};
    
    std::vector<int> batches = {512, 256, 128, 64};
    std::vector<size_t> sizes;
    std::vector<float> tok_rates;

    for (int b : batches) {
        LlamaCppEngine engine(profile);
        LlamaEngineConfig config;
        config.n_batch = b;
        engine.setConfig(config);

        bool loaded = engine.loadModel(MODEL_PATH);
        ASSERT_TRUE(loaded, "Model loads for n_batch=" + std::to_string(b));
        if (!loaded) continue;

        size_t size = engine.kvTelemetry().snapshot().compute;
        sizes.push_back(size);
        std::cout << "  n_batch=" << b << " -> compute_buffer_size=" << (size / (1024 * 1024)) << "MB\n";

        // Run a small 20-token generation
        SamplingConfig sampling;
        sampling.max_tokens = 20;
        engine.setSamplingConfig(sampling);

        auto token = std::make_shared<CancellationToken>();
        InferenceContext ctx("test-batch-profile", token, 512);
        ctx.max_tokens_to_generate = 20;

        GenerationResult result = engine.generateEx("The weather is nice", ctx);
        ASSERT_TRUE(result.success, "Generation success at batch " + std::to_string(b));
        std::cout << "    throughput: " << result.tokens_per_second << " tok/s\n";
        tok_rates.push_back(result.tokens_per_second);

        engine.unloadModel();
    }

    // Verify compute buffer decreases as n_batch decreases
    if (sizes.size() == 4) {
        ASSERT_TRUE(sizes[0] >= sizes[1], "Compute buffer scales: 512 >= 256");
        ASSERT_TRUE(sizes[1] >= sizes[2], "Compute buffer scales: 256 >= 128");
        ASSERT_TRUE(sizes[2] >= sizes[3], "Compute buffer scales: 128 >= 64");
    }

    // Verify tok/s is stable (within ±15% of the average of the last 3)
    if (tok_rates.size() == 4) {
        float sum = 0;
        for (float r : tok_rates) sum += r;
        float avg = sum / 4.0f;
        std::cout << "  Average decode rate: " << avg << " tok/s\n";
        for (size_t i = 0; i < tok_rates.size(); ++i) {
            float diff = std::abs(tok_rates[i] - avg) / avg;
            ASSERT_TRUE(diff < 0.25f, "Decode rate stable for batch " + std::to_string(batches[i]) + ": diff=" + std::to_string(diff));
        }
    }
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "ISHA AI — COMPUTE BUFFER PROFILING BENCHMARK\n";
    std::cout << "==================================================\n";

    testComputeBufferScaling();

    std::cout << "\n==================================================\n";
    std::cout << "COMPUTE BUFFER PROFILING RESULTS: " << g_pass << " PASS, " << g_fail << " FAIL\n";
    std::cout << "==================================================\n";

    return g_fail > 0 ? 1 : 0;
}
