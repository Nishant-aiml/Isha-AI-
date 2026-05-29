#ifndef _WIN32

#include "resource_monitor.hpp"
#include "../logging/logger.hpp"
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace isha {

class AndroidMonitor : public IResourceMonitor {
public:
    AndroidMonitor() {
        ISHA_LOG_INFO("ResourceMonitor", "Initialized Android/Linux hardware performance monitor.");
    }

    ~AndroidMonitor() override = default;

    uint64_t getProcessRSSBytes() override {
        FILE* file = fopen("/proc/self/statm", "r");
        if (!file) return 0;
        char buf[128];
        if (fgets(buf, sizeof(buf), file)) {
            fclose(file);
            char* p = buf;
            while (*p && *p != ' ') p++;
            if (*p == ' ') {
                p++;
                unsigned long pages = std::strtoul(p, nullptr, 10);
                return static_cast<uint64_t>(pages) * sysconf(_SC_PAGESIZE);
            }
        } else {
            fclose(file);
        }
        return 0;
    }

    uint64_t getPageFaultCount() override {
        FILE* file = fopen("/proc/self/stat", "r");
        if (!file) return 0;
        char buf[1024];
        if (fgets(buf, sizeof(buf), file)) {
            fclose(file);
            char* p = strrchr(buf, ')');
            if (p) {
                p++;
                unsigned long minflt = 0;
                unsigned long majflt = 0;
                int token_idx = 0;
                char* token = strtok(p, " ");
                while (token != nullptr) {
                    token_idx++;
                    if (token_idx == 8) {
                        minflt = std::strtoul(token, nullptr, 10);
                    } else if (token_idx == 10) {
                        majflt = std::strtoul(token, nullptr, 10);
                        break;
                    }
                    token = strtok(nullptr, " ");
                }
                return static_cast<uint64_t>(minflt + majflt);
            }
        } else {
            fclose(file);
        }
        return 0;
    }

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
