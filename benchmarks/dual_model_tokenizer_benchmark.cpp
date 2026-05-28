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

static const std::string MODEL_PATH_0_5B = "models/llm/qwen2.5-0.5b-instruct-q4_k_m.gguf";
static const std::string MODEL_PATH_1_5B = "models/llm/qwen2.5-1.5b-instruct-q4_k_m.gguf";

void testDualModelTokenizer() {
    std::cout << "\n=== Test: Dual-Model Tokenizer Swap ===\n";

    DeviceProfile profile{DeviceTier::MID, 6144, 8, "Android"};
    auto engine = std::make_shared<LlamaCppEngine>(profile);

    std::string path_1_5 = MODEL_PATH_1_5B;
    FILE* f = fopen(path_1_5.c_str(), "rb");
    if (!f) {
        path_1_5 = MODEL_PATH_0_5B;
        std::cout << "  [WARNING] 1.5B model not found, falling back to 0.5B for swap test\n";
    } else {
        fclose(f);
    }

    // 1. Swap 0.5B <-> 1.5B
    bool ok = true;
    for (int i = 0; i < 5; ++i) {
        if (!engine->loadModel(MODEL_PATH_0_5B)) { ok = false; break; }
        auto snap = engine->kvTelemetry().snapshot();
        ASSERT_TRUE(snap.tokenizer_active <= 1, "0.5B load: active tokenizer instances <= 1");
        
        if (!engine->loadModel(path_1_5)) { ok = false; break; }
        snap = engine->kvTelemetry().snapshot();
        ASSERT_TRUE(snap.tokenizer_active <= 1, "1.5B load: active tokenizer instances <= 1");
    }
    ASSERT_TRUE(ok, "Successfully completed dual-model tokenizer swap loops");
    engine->unloadModel();
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "ISHA AI — DUAL-MODEL TOKENIZER SWAP BENCHMARK\n";
    std::cout << "==================================================\n";

    testDualModelTokenizer();

    std::cout << "\n==================================================\n";
    std::cout << "DUAL-MODEL TOKENIZER SWAP BENCHMARK: " << g_pass << " PASS, " << g_fail << " FAIL\n";
    std::cout << "==================================================\n";

    return g_fail > 0 ? 1 : 0;
}
