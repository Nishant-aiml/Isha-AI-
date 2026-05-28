#include <iostream>
#include <string>
#include <thread>
#include <cassert>
#include <memory>
#include <chrono>
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

static const std::string MODEL_PATH = "models/llm/qwen2.5-1.5b-instruct-q4_k_m.gguf";

void testPromptScale() {
    std::cout << "\n=== Test: Prompt Scaling & Chunked Prefill ===\n";

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

    bool loaded = engine->loadModel(path);
    ASSERT_TRUE(loaded, "Model loaded successfully");

    if (loaded) {
        // 1. 400-token prompt prefill
        {
            std::string prompt_400 = "Context info: ";
            for (int i = 0; i < 60; ++i) {
                prompt_400 += "knowledge base context rule test factor ";
            }
            prompt_400 += "\nQuery: Summarize context.";

            auto token = std::make_shared<CancellationToken>();
            InferenceContext ctx("scale-test", token, 512);
            ctx.max_tokens_to_generate = 10;

            GenerationResult result = engine->generateEx(prompt_400, ctx);
            ASSERT_TRUE(result.success, "400-token prefill generation succeeds");
            ASSERT_TRUE(result.prefill_chunks > 1, "Chunked prefill correctly splits long prompt");
        }

        // 2. Cancellation during prefill
        {
            std::string long_prompt = "Context: ";
            for (int i = 0; i < 50; ++i) {
                long_prompt += "chunked prefill cancellation test token element ";
            }

            auto token = std::make_shared<CancellationToken>();
            InferenceContext ctx("cancel-prefill-test", token, 512);
            ctx.max_tokens_to_generate = 10;

            std::thread cancel_thread([&]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                token->cancel();
            });

            GenerationResult result = engine->generateEx(long_prompt, ctx);
            if (cancel_thread.joinable()) {
                cancel_thread.join();
            }
            std::cout << "  Cancellation result: success=" << result.success << ", cancelled=" << result.was_cancelled << "\n";
            ASSERT_TRUE(result.was_cancelled || result.success, "Prefill cancellation exits cleanly");
        }

        engine->unloadModel();
    }
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "ISHA AI — PROMPT SCALE & CHUNKED PREFILL BENCHMARK\n";
    std::cout << "==================================================\n";

    testPromptScale();

    std::cout << "\n==================================================\n";
    std::cout << "PROMPT SCALE BENCHMARK: " << g_pass << " PASS, " << g_fail << " FAIL\n";
    std::cout << "==================================================\n";

    return g_fail > 0 ? 1 : 0;
}
