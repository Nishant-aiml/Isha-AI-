#ifndef ISHA_AI_MEMORY_GUARD_HPP
#define ISHA_AI_MEMORY_GUARD_HPP

#include "../config/device_profile.hpp"
#include "../config/resource_limits.hpp"

namespace isha {

class IResourceMonitor; // Forward declaration

class MemoryGuard {
public:
    MemoryGuard(const DeviceProfile& profile, IResourceMonitor* monitor);
    ~MemoryGuard();

    bool checkMemoryConstraints(unsigned int requested_load_size_mb);
    void checkSystemMemoryPressure();
    void enforceUnload();

private:
    DeviceProfile profile_;
    ResourceLimits limits_;
    IResourceMonitor* monitor_;
};

} // namespace isha

#endif // ISHA_AI_MEMORY_GUARD_HPP
