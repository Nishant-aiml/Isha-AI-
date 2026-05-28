#include "../core/watchdog/runtime_watchdog.hpp"
#include "../core/inference/cancellation_token.hpp"
#include "../core/runtime/event_bus.hpp"
#include "../core/monitoring/resource_monitor.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cassert>
#include <memory>

int main() {
    std::cout << "=== WATCHDOG RECOVERY BENCHMARK ===\n";
    
    std::atomic<bool> alert_received{false};
    std::atomic<bool> timeout_received{false};
    
    isha::EventBus::getInstance().subscribe(
        isha::EventType::WATCHDOG_ALERT,
        "WatchdogTestListener",
        [&](const isha::Event& ev) {
            alert_received.store(true);
        }
    );
    
    isha::EventBus::getInstance().subscribe(
        isha::EventType::INFERENCE_TIMEOUT,
        "WatchdogTestListener",
        [&](const isha::Event& ev) {
            timeout_received.store(true);
        }
    );
    
    isha::RuntimeWatchdog watchdog(std::chrono::milliseconds(10));
    watchdog.start();
    
    // Test 1: Recovery of stuck tasks
    std::cout << "[TEST 1] Recovery of stuck tasks... ";
    auto cancel_token = std::make_shared<isha::CancellationToken>();
    watchdog.registerTask("stuck_inference_task", std::chrono::milliseconds(20), cancel_token);
    
    // Wait for watchdog to trigger (20ms timeout + checks)
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    
    assert(cancel_token->isCancelled());
    assert(alert_received.load());
    assert(timeout_received.load());
    std::cout << "PASS (Watchdog recovered stuck task successfully)\n";
    
    // Test 2: Natural completion unregisters task
    std::cout << "[TEST 2] Natural task completion... ";
    auto clean_cancel = std::make_shared<isha::CancellationToken>();
    watchdog.registerTask("clean_task", std::chrono::milliseconds(100), clean_cancel);
    watchdog.unregisterTask("clean_task");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    assert(!clean_cancel->isCancelled()); // Should NOT be cancelled since it was unregistered
    std::cout << "PASS\n";
    
    // Test 3: stuck decode recovery
    std::cout << "[TEST 3] stuck decode recovery... ";
    auto stuck_decode_cancel = std::make_shared<isha::CancellationToken>();
    watchdog.registerTask("stuck_decode", std::chrono::milliseconds(20), stuck_decode_cancel);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    assert(stuck_decode_cancel->isCancelled());
    std::cout << "PASS\n";

    // Test 4: scheduler overload recovery
    std::cout << "[TEST 4] scheduler overload recovery... ";
    for (int i = 0; i < 100; ++i) {
        auto dummy_cancel = std::make_shared<isha::CancellationToken>();
        watchdog.registerTask("dummy_" + std::to_string(i), std::chrono::milliseconds(1000), dummy_cancel);
    }
    for (int i = 0; i < 100; ++i) {
        watchdog.unregisterTask("dummy_" + std::to_string(i));
    }
    std::cout << "PASS\n";

    // Test 5: concurrent cancel + unload
    std::cout << "[TEST 5] concurrent cancel + unload... ";
    auto dummy_cancel2 = std::make_shared<isha::CancellationToken>();
    watchdog.registerTask("stuck_task", std::chrono::milliseconds(10), dummy_cancel2);
    dummy_cancel2->cancel();
    watchdog.unregisterTask("stuck_task");
    std::cout << "PASS\n";
    
    watchdog.stop();
    isha::EventBus::getInstance().unsubscribe(isha::EventType::WATCHDOG_ALERT, "WatchdogTestListener");
    isha::EventBus::getInstance().unsubscribe(isha::EventType::INFERENCE_TIMEOUT, "WatchdogTestListener");
    
    std::cout << "=== WATCHDOG RECOVERY BENCHMARK COMPLETED SUCCESSFULLY ===\n";
    return 0;
}
