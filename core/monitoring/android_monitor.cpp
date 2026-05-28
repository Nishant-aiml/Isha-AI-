#ifndef _WIN32

#include "resource_monitor.hpp"
#include "../logging/logger.hpp"

namespace isha {

class AndroidMonitor : public IResourceMonitor {
public:
    AndroidMonitor() {
        ISHA_LOG_INFO("ResourceMonitor", "Initialized Android/Linux hardware performance monitor stub.");
    }

    ~AndroidMonitor() override = default;

    unsigned int getAvailableMemoryMB() override {
        return 1024; // Stub return value
    }

    unsigned int getSystemMemoryUsageMB() override {
        return 2048; // Stub return value
    }

    double getCpuUsagePercent() override {
        return 10.0;
    }

    double getBatteryLevel() override {
        return 85.0;
    }

    double getDeviceTemperatureC() override {
        return 34.0;
    }
};

IResourceMonitor* createResourceMonitor() {
    return new AndroidMonitor();
}

} // namespace isha

#endif // _WIN32
