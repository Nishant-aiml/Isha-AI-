#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cassert>
#include <memory>
#include <chrono>
#include "inference/llama_cpp_engine.hpp"
#include "inference/cancellation_token.hpp"
#include "config/device_profile.hpp"
#include "logging/logger.hpp"
#include "scheduler/inference_scheduler.hpp"

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

static const std::string MODEL_PATH = "models/llm/qwen2.5-1.5b-instruct-q4_k_m.gguf";

void testOverlapSpikes() {
    std::cout << "\n=== Test: Overlap Spike & Thread Safety ===\n";

    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    auto engine = std::make_shared<LlamaCppEngine>(profile);

    std::string path = MODEL_PATH;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        path = "models/llm/qwen2.5-0.5b-instruct-q4_k_m.gguf";
        std::cout << "  [WARNING] 1.5B model not found, falling back to 0.5B: " << path << "\n";
    } else {
        fclose(f);
    }

    // 1. Generation during unload
    {
        bool loaded = engine->loadModel(path);
        ASSERT_TRUE(loaded, "Model loaded successfully");
        if (loaded) {
            auto token = std::make_shared<CancellationToken>();
            InferenceContext ctx("unload-test", token, 512);
            ctx.max_tokens_to_generate = 100;

            std::thread gen_thread([&]() {
                engine->generateEx("Write a long essay about artificial intelligence.", ctx);
            });

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            engine->unloadModel();

            if (gen_thread.joinable()) {
                gen_thread.join();
            }
            ASSERT_TRUE(!engine->isModelLoaded(), "Engine successfully unloaded during active generation");
            auto snap = engine->kvTelemetry().snapshot();
            ASSERT_EQ(snap.current, 0u, "KV cells are completely cleaned up");
        }
    }

    // 2. Cancellation during reload
    {
        bool loaded = engine->loadModel(path);
        ASSERT_TRUE(loaded, "Model reloaded successfully");
        if (loaded) {
            auto token = std::make_shared<CancellationToken>();
            InferenceContext ctx("reload-test", token, 512);
            ctx.max_tokens_to_generate = 100;

            std::thread gen_thread([&]() {
                engine->generateEx("Write a long poem about gravity.", ctx);
            });

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            token->cancel();

            bool reloaded = engine->loadModel(path);
            ASSERT_TRUE(reloaded, "Model reloaded immediately after cancellation without deadlock");

            if (gen_thread.joinable()) {
                gen_thread.join();
            }
            engine->unloadModel();
        }
    }

    // 3. Rapid restart loops
    {
        std::cout << "  Starting 5 rapid load/generate/cancel/unload loops...\n";
        bool loop_ok = true;
        uint64_t initial_rss = 0;
        uint64_t final_rss = 0;
        
        for (int i = 0; i < 5; ++i) {
            if (!engine->loadModel(path)) { loop_ok = false; break; }
            auto token = std::make_shared<CancellationToken>();
            InferenceContext ctx("loop-test", token, 512);
            ctx.max_tokens_to_generate = 20;

            std::thread gen_thread([&]() {
                engine->generateEx("Tell me a quick joke.", ctx);
            });

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            token->cancel();

            if (gen_thread.joinable()) {
                gen_thread.join();
            }

            auto snap = engine->kvTelemetry().snapshot();
            if (i == 0) initial_rss = snap.peak_rss;
            final_rss = snap.peak_rss;

            engine->unloadModel();
        }
        ASSERT_TRUE(loop_ok, "Completed rapid restart loops without failures");
    }

    // 4. Tokenizer active instances check
    {
        bool loaded = engine->loadModel(path);
        if (loaded) {
            auto snap = engine->kvTelemetry().snapshot();
            ASSERT_TRUE(snap.tokenizer_active <= 1, "Tokenizer active instances never exceed 1");
            engine->unloadModel();
        }
    }
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "ISHA AI — OVERLAP SPIKE & CHAOS SURVIVABILITY\n";
    std::cout << "==================================================\n";

    testOverlapSpikes();

    std::cout << "\n==================================================\n";
    std::cout << "OVERLAP SPIKE BENCHMARK: " << g_pass << " PASS, " << g_fail << " FAIL\n";
    std::cout << "==================================================\n";

    return g_fail > 0 ? 1 : 0;
}
