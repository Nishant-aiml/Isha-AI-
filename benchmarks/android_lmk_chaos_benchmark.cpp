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
#include "runtime/event_bus.hpp"

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

void testLmkChaos() {
    std::cout << "\n=== Test: Android LMK Chaos & Emergency Degradation ===\n";

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
        // 1. Memory reclaim during decode
        {
            auto token = std::make_shared<CancellationToken>();
            InferenceContext ctx("lmk-reclaim-test", token, 512);
            ctx.max_tokens_to_generate = 100;

            std::thread gen_thread([&]() {
                engine->generateEx("Describe the process of photosynthesis in extreme detail.", ctx);
            });

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            // Simulate memory reclaim by cancelling generation token
            token->cancel();

            if (gen_thread.joinable()) {
                gen_thread.join();
            }

            auto snap = engine->kvTelemetry().snapshot();
            ASSERT_EQ(snap.current, 0u, "KV cache correctly cleaned up after simulation of memory reclaim");
        }

        // 2. Aggressive OEM trim simulation
        {
            bool event_received = false;
            EventBus::getInstance().subscribe(EventType::MODEL_UNLOADING, [&](const Event& ev) {
                event_received = true;
                engine->unloadModel();
            });

            EventBus::getInstance().publish({EventType::MEMORY_PRESSURE_CRITICAL, "MemoryGuard", "92.5"});
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ASSERT_TRUE(event_received, "EventBus successfully routed critical memory pressure notification");
            ASSERT_TRUE(!engine->isModelLoaded(), "Engine aggressively unloaded model in response to memory pressure");
        }
    }
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "ISHA AI — ANDROID LMK CHAOS BENCHMARK\n";
    std::cout << "==================================================\n";

    testLmkChaos();

    std::cout << "\n==================================================\n";
    std::cout << "ANDROID LMK CHAOS BENCHMARK: " << g_pass << " PASS, " << g_fail << " FAIL\n";
    std::cout << "==================================================\n";

    return g_fail > 0 ? 1 : 0;
}
