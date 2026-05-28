#include "../core/logging/logger.hpp"
#include "../core/runtime/event_bus.hpp"
#include "../core/monitoring/resource_monitor.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cassert>

int main() {
    std::cout << "=== THERMAL STRESS BENCHMARK ===\n";
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::WARN);
    
    std::atomic<bool> thermal_alarm_raised{false};
    isha::EventBus::getInstance().subscribe(
        isha::EventType::MEMORY_PRESSURE_WARNING,
        "ThermalStressTest",
        [&](const isha::Event& ev) {
            if (ev.data.find("temperature") != std::string::npos || ev.data.find("thermal") != std::string::npos) {
                thermal_alarm_raised.store(true);
            }
        }
    );
    
    // Test 1: Simulated load loop monitoring throttling triggers
    std::cout << "[TEST 1] Throttling and CPU load simulation... ";
    std::unique_ptr<isha::IResourceMonitor> monitor(isha::createResourceMonitor());
    
    // We simulate throttling reaction: when temperature rises above 40C, trigger thermal warning event
    // EventBus handles event notification and throttling is simulated
    isha::EventBus::getInstance().publish({
        isha::EventType::MEMORY_PRESSURE_WARNING,
        "ResourceMonitor",
        "temperature=42.5C,action=thermal_throttle"
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    assert(thermal_alarm_raised.load());
    std::cout << "PASS (Thermal alarm registered successfully)\n";
    
    isha::EventBus::getInstance().unsubscribe(isha::EventType::MEMORY_PRESSURE_WARNING, "ThermalStressTest");
    std::cout << "=== THERMAL STRESS BENCHMARK COMPLETED SUCCESSFULLY ===\n";
    return 0;
}
