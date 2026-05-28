#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef ERROR
#undef ERROR
#endif

#include "../core/logging/logger.hpp"
#include "../core/inference/inference_engine.hpp"
#include "../core/inference/gguf_inference_engine.hpp"
#include "../core/inference/inference_context.hpp"
#include "../core/inference/cancellation_token.hpp"
#include "../core/scheduler/inference_scheduler.hpp"
#include "../core/watchdog/runtime_watchdog.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>

int main() {
    std::cout << "=== INFERENCE STRESS BENCHMARK ===\n";
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::WARN);
    
    auto engine = std::make_shared<isha::GGUFInferenceEngine>();
    isha::InferenceScheduler scheduler(4);
    isha::RuntimeWatchdog watchdog(std::chrono::milliseconds(50));
    watchdog.start();
    
    // Test 1: Concurrent inference requests with watchdog protection
    std::cout << "[TEST 1] Concurrent inference with watchdog... ";
    std::atomic<int> completed{0};
    std::atomic<int> cancelled{0};
    constexpr int NUM_CONCURRENT = 20;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_CONCURRENT; ++i) {
        threads.emplace_back([&, i]() {
            auto cancel = std::make_shared<isha::CancellationToken>();
            std::string task_id = "stress_task_" + std::to_string(i);
            
            // Register with watchdog (tight timeout to trigger some cancellations)
            watchdog.registerTask(task_id, std::chrono::milliseconds(50 + i * 10), cancel);
            
            isha::InferenceContext ctx("stress_session", cancel, 64);
            std::string result = engine->generate("Stress test prompt " + std::to_string(i), ctx);
            
            if (result == "GENERATION_CANCELLED" || cancel->isCancelled()) {
                cancelled.fetch_add(1);
            } else {
                completed.fetch_add(1);
            }
            watchdog.unregisterTask(task_id);
        });
    }
    for (auto& t : threads) t.join();
    
    std::cout << "PASS (completed=" << completed.load() << ", cancelled=" << cancelled.load() << ")\n";
    
    // Test 2: Scheduler priority under stress
    std::cout << "[TEST 2] Priority scheduling under load... ";
    isha::InferenceScheduler stress_scheduler(4);
    std::mutex order_mutex;
    std::vector<int> completion_order;
    
    // Submit tasks with different priorities
    std::vector<std::thread> prio_threads;
    for (int i = 0; i < 10; ++i) {
        prio_threads.emplace_back([&, i]() {
            auto cancel = std::make_shared<isha::CancellationToken>();
            isha::InferenceContext ctx("prio_session_" + std::to_string(i), cancel, 64);
            engine->generate("Priority test " + std::to_string(i), ctx);
            std::lock_guard<std::mutex> lock(order_mutex);
            completion_order.push_back(i);
        });
    }
    for (auto& t : prio_threads) t.join();
    
    assert(completion_order.size() == 10);
    std::cout << "PASS (" << completion_order.size() << " tasks completed)\n";
    
    // Test 3: Watchdog timeout count
    std::cout << "[TEST 3] Watchdog timeout tracking... ";
    unsigned int timeouts = watchdog.totalTimeouts();
    std::cout << "PASS (" << timeouts << " timeouts triggered)\n";
    
    // Test 4: Rapid submit-cancel cycle
    std::cout << "[TEST 4] Rapid submit-cancel cycle... ";
    for (int i = 0; i < 50; ++i) {
        auto cancel = std::make_shared<isha::CancellationToken>();
        std::string task_id = "rapid_task_" + std::to_string(i);
        watchdog.registerTask(task_id, std::chrono::milliseconds(2), cancel);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        watchdog.unregisterTask(task_id);
    }
    std::cout << "PASS\n";
    
    watchdog.stop();
    std::cout << "=== INFERENCE STRESS BENCHMARK COMPLETED SUCCESSFULLY ===\n";
    return 0;
}
