#ifndef ISHA_AI_SAMPLING_CONFIG_HPP
#define ISHA_AI_SAMPLING_CONFIG_HPP

#include <algorithm>
#include <string>
#include <vector>
#include "../config/device_profile.hpp"

namespace isha {

struct SamplingConfig {
    unsigned int max_tokens = 256;
    float temperature = 0.7f;
    float top_p = 0.9f;
    float top_k = 40.0f;
    float repetition_penalty = 1.1f;
    std::vector<std::string> stop_strings;

    // Clamp all values to safe ranges
    void clamp() {
        max_tokens = std::clamp(max_tokens, 1u, 1024u);
        temperature = std::clamp(temperature, 0.0f, 2.0f);
        top_p = std::clamp(top_p, 0.0f, 1.0f);
        top_k = std::clamp(top_k, 1.0f, 100.0f);
        repetition_penalty = std::clamp(repetition_penalty, 1.0f, 2.0f);
    }

    // Tier-appropriate defaults
    static SamplingConfig defaultForTier(DeviceTier tier) {
        SamplingConfig config;
        switch (tier) {
            case DeviceTier::LOW:
                config.max_tokens = 128;
                config.temperature = 0.6f;
                break;
            case DeviceTier::MID:
                config.max_tokens = 256;
                config.temperature = 0.7f;
                break;
            case DeviceTier::HIGH:
                config.max_tokens = 384;
                config.temperature = 0.7f;
                break;
            case DeviceTier::FLAGSHIP:
                config.max_tokens = 512;
                config.temperature = 0.8f;
                break;
            default:
                break;
        }
        config.clamp();
        return config;
    }
};

} // namespace isha

#endif // ISHA_AI_SAMPLING_CONFIG_HPP
