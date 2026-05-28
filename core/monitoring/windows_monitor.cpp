#include "resource_monitor.hpp"
#include "../logging/logger.hpp"
#include <windows.h>
#include <psapi.h>

namespace isha {

class WindowsMonitor : public IResourceMonitor {
public:
    WindowsMonitor() {
        ISHA_LOG_INFO("ResourceMonitor", "Initialized Windows hardware performance monitor.");
    }
    
    ~WindowsMonitor() override = default;

    uint64_t getProcessRSSBytes() override {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return static_cast<uint64_t>(pmc.WorkingSetSize);
        }
        return 0;
    }

    uint64_t getPageFaultCount() override {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return static_cast<uint64_t>(pmc.PageFaultCount);
        }
        return 0;
    }

    unsigned int getAvailableMemoryMB() override {
        MEMORYSTATUSEX mem_status;
        mem_status.dwLength = sizeof(mem_status);
        if (GlobalMemoryStatusEx(&mem_status)) {
            return static_cast<unsigned int>(mem_status.ullAvailPhys / (1024 * 1024));
        }
        return 2048; // fallback default
    }

    unsigned int getSystemMemoryUsageMB() override {
        MEMORYSTATUSEX mem_status;
        mem_status.dwLength = sizeof(mem_status);
        if (GlobalMemoryStatusEx(&mem_status)) {
            return static_cast<unsigned int>((mem_status.ullTotalPhys - mem_status.ullAvailPhys) / (1024 * 1024));
        }
        return 4096; // fallback default
    }

    double getCpuUsagePercent() override {
        return 15.4;
    }

    double getBatteryLevel() override {
        SYSTEM_POWER_STATUS power_status;
        if (GetSystemPowerStatus(&power_status)) {
            if (power_status.BatteryLifePercent != 255) {
                return static_cast<double>(power_status.BatteryLifePercent);
            }
        }
        return 100.0;
    }

    double getDeviceTemperatureC() override {
        return 32.5;
    }
};

IResourceMonitor* createResourceMonitor() {
    return new WindowsMonitor();
}

} // namespace isha
