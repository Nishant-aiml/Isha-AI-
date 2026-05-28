#ifndef ISHA_AI_DEVICE_PROFILE_HPP
#define ISHA_AI_DEVICE_PROFILE_HPP

#include <string>

namespace isha {

enum class DeviceTier {
    LOW,
    MID,
    HIGH,
    FLAGSHIP
};

inline std::string deviceTierToString(DeviceTier tier) {
    switch (tier) {
        case DeviceTier::LOW: return "LOW";
        case DeviceTier::MID: return "MID";
        case DeviceTier::HIGH: return "HIGH";
        case DeviceTier::FLAGSHIP: return "FLAGSHIP";
        default: return "UNKNOWN";
    }
}

struct DeviceProfile {
    DeviceTier tier = DeviceTier::LOW;
    unsigned int total_ram_mb = 0;
    unsigned int cpu_cores = 0;
    std::string os_name = "Unknown";
};

} // namespace isha

#endif // ISHA_AI_DEVICE_PROFILE_HPP
