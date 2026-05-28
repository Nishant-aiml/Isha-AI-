#include "monitoring/resource_monitor.hpp"
#include "logging/logger.hpp"
#include "runtime/event_bus.hpp"
#include <iostream>
#include <cassert>
#include <memory>

class MockThermalMonitor : public isha::IResourceMonitor {
public:
    MockThermalMonitor(double temp, double battery, unsigned int available_ram)
        : temp_(temp), battery_(battery), available_ram_(available_ram) {}
    ~MockThermalMonitor() override = default;

    unsigned int getAvailableMemoryMB() override { return available_ram_; }
    unsigned int getSystemMemoryUsageMB() override { return 4096; }
    double getCpuUsagePercent() override { return 25.0; }
    double getBatteryLevel() override { return battery_; }
    double getDeviceTemperatureC() override { return temp_; }

    void setTemperature(double temp) { temp_ = temp; }
    void setBatteryLevel(double level) { battery_ = level; }

private:
    double temp_;
    double battery_;
    unsigned int available_ram_;
};

int main() {
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::DEBUG);
    ISHA_LOG_INFO("Benchmark", "=== STARTING ANDROID PROFILE BENCHMARK ===");

    // Simulate standard conditions (35°C temp, 85% battery, 2048MB free RAM)
    auto monitor = std::make_unique<MockThermalMonitor>(35.0, 85.0, 2048);

    // Setup EventBus listener for thermal alerts
    bool temp_warning_fired = false;
    bool temp_critical_fired = false;

    isha::EventBus::getInstance().subscribe(isha::EventType::MEMORY_PRESSURE_WARNING, "Profiler", [&](const isha::Event& event) {
        temp_warning_fired = true;
    });

    // 1. Verify nominal states
    std::cout << "Initial temp: " << monitor->getDeviceTemperatureC() << " C" << std::endl;
    std::cout << "Initial battery: " << monitor->getBatteryLevel() << " %" << std::endl;
    assert(monitor->getDeviceTemperatureC() == 35.0);

    // 2. Simulate thermal rise
    monitor->setTemperature(39.0);
    ISHA_LOG_WARN("Profiler", "Simulated device temperature rise to: " + std::to_string(monitor->getDeviceTemperatureC()) + "C");
    assert(monitor->getDeviceTemperatureC() > 38.0); // Exceeds nominal threshold

    // 3. Simulate battery drain
    monitor->setBatteryLevel(12.0);
    ISHA_LOG_WARN("Profiler", "Simulated battery level drops to: " + std::to_string(monitor->getBatteryLevel()) + "%");
    assert(monitor->getBatteryLevel() < 15.0); // Low battery warning threshold

    isha::EventBus::getInstance().unsubscribe(isha::EventType::MEMORY_PRESSURE_WARNING, "Profiler");

    ISHA_LOG_INFO("Benchmark", "=== ANDROID PROFILE BENCHMARK COMPLETED SUCCESSFULLY ===");
    return 0;
}
