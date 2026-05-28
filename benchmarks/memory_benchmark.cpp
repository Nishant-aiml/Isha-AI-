#include "model/memory_guard.hpp"
#include "monitoring/resource_monitor.hpp"
#include "logging/logger.hpp"
#include <iostream>
#include <cassert>
#include <memory>

int main() {
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::DEBUG);
    ISHA_LOG_INFO("Benchmark", "=== STARTING MEMORY BENCHMARK ===");

    isha::DeviceProfile low_profile = { isha::DeviceTier::LOW, 2048, 2, "Windows" };
    isha::DeviceProfile high_profile = { isha::DeviceTier::HIGH, 8192, 8, "Windows" };

    std::unique_ptr<isha::IResourceMonitor> monitor(isha::createResourceMonitor());
    
    // Output current real system memory stats
    unsigned int system_free = monitor->getAvailableMemoryMB();
    unsigned int system_used = monitor->getSystemMemoryUsageMB();
    std::cout << "Real available system memory: " << system_free << " MB" << std::endl;
    std::cout << "Real used system memory: " << system_used << " MB" << std::endl;

    isha::MemoryGuard low_guard(low_profile, monitor.get());
    isha::MemoryGuard high_guard(high_profile, monitor.get());

    // Verify low-tier limits (model limit: 250MB)
    bool low_t1_fits = low_guard.checkMemoryConstraints(210); // T0 size
    std::cout << "Low tier fits 210MB T0 model: " << (low_t1_fits ? "YES" : "NO") << std::endl;
    assert(low_t1_fits);

    bool low_t2_fits = low_guard.checkMemoryConstraints(950); // T1 size
    std::cout << "Low tier fits 950MB T1 model: " << (low_t2_fits ? "YES" : "NO") << std::endl;
    assert(!low_t2_fits); // Should fail due to LOW limits

    // Verify high-tier limits (model limit: 1500MB)
    bool high_t2_fits = high_guard.checkMemoryConstraints(950); // T1 size
    std::cout << "High tier fits 950MB T1 model: " << (high_t2_fits ? "YES" : "NO") << std::endl;
    assert(high_t2_fits);

    // Simulate memory pressure check
    low_guard.checkSystemMemoryPressure();

    ISHA_LOG_INFO("Benchmark", "=== MEMORY BENCHMARK COMPLETED SUCCESSFULLY ===");
    return 0;
}
