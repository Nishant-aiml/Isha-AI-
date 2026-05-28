#ifndef ISHA_AI_RESOURCE_LIMITS_HPP
#define ISHA_AI_RESOURCE_LIMITS_HPP

#include "device_profile.hpp"

namespace isha {

struct ResourceLimits {
    unsigned int max_allowed_ram_usage_mb;
    unsigned int model_memory_limit_mb;
    unsigned int inactive_timeout_seconds;
    bool allow_t2_models;
    bool allow_t3_models;

    static ResourceLimits getLimitsForTier(DeviceTier tier) {
        switch (tier) {
            case DeviceTier::LOW:
                return { 512, 250, 60, false, false };
            case DeviceTier::MID:
                return { 1024, 750, 90, true, false };
            case DeviceTier::HIGH:
                return { 2048, 1500, 120, true, false };
            case DeviceTier::FLAGSHIP:
                return { 4096, 4500, 180, true, true };
            default:
                return { 256, 100, 30, false, false };
        }
    }
};

} // namespace isha

#endif // ISHA_AI_RESOURCE_LIMITS_HPP
