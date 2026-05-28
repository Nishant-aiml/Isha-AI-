#include "../core/watchdog/runtime_watchdog.hpp"
#include "../core/runtime/event_bus.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace isha;

bool callback_triggered = false;

void testCooperativeRecovery() {
    RuntimeWatchdog watchdog(std::chrono::milliseconds(10));
    watchdog.registerRecoveryCallback([]() {
        callback_triggered = true;
    });

    auto token = std::make_shared<CancellationToken>();
    watchdog.registerTask("stalled_inference_task", std::chrono::milliseconds(50), token);
    watchdog.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(120));

    watchdog.stop();

    assert(callback_triggered);
    assert(token->isCancelled());
    assert(watchdog.totalTimeouts() == 1);

    std::cout << "testCooperativeRecovery passed." << std::endl;
}

int main() {
    testCooperativeRecovery();
    std::cout << "All Watchdog Cooperative Benchmarks passed." << std::endl;
    return 0;
}
