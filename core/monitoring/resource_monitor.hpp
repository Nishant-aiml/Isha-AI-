#ifndef ISHA_AI_RESOURCE_MONITOR_HPP
#define ISHA_AI_RESOURCE_MONITOR_HPP

#include <cstdint>

namespace isha {

class IResourceMonitor {
public:
    virtual ~IResourceMonitor() = default;

    virtual unsigned int getAvailableMemoryMB() = 0;
    virtual unsigned int getSystemMemoryUsageMB() = 0;
    virtual uint64_t getProcessRSSBytes() = 0;
    virtual uint64_t getPageFaultCount() = 0;
    virtual double getCpuUsagePercent() = 0;
    virtual double getBatteryLevel() = 0;
    virtual double getDeviceTemperatureC() = 0;
};

IResourceMonitor* createResourceMonitor();

} // namespace isha

#endif // ISHA_AI_RESOURCE_MONITOR_HPP
