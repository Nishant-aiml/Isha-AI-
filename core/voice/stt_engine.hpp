#ifndef ISHA_AI_STT_ENGINE_HPP
#define ISHA_AI_STT_ENGINE_HPP

#include "audio_types.hpp"
#include <memory>
#include <mutex>

namespace isha {

class EchoSuppressionPolicy {
public:
    EchoSuppressionPolicy() = default;
    
    // Minimal echo suppression placeholder
    void suppressEcho(std::vector<float>& microphone_samples, const std::vector<float>& playback_samples) {
        // Playback reference subtraction or gating placeholder
        size_t limit = std::min(microphone_samples.size(), playback_samples.size());
        for (size_t i = 0; i < limit; ++i) {
            // Subtract modeled echo or apply attenuation factor
            microphone_samples[i] -= 0.1f * playback_samples[i];
            if (microphone_samples[i] > 1.0f) microphone_samples[i] = 1.0f;
            if (microphone_samples[i] < -1.0f) microphone_samples[i] = -1.0f;
        }
    }
};

class IWhisperSTTEngine {
public:
    virtual ~IWhisperSTTEngine() = default;

    virtual std::string transcribeChunk(const AudioChunk& chunk) = 0;
    virtual bool detectVoiceActivity(const AudioChunk& chunk) = 0;
    
    virtual void setEnergyThreshold(double threshold) = 0;
    virtual double getEnergyThreshold() const = 0;
};

class WhisperSTTEngine : public IWhisperSTTEngine {
public:
    WhisperSTTEngine(double energy_threshold = 0.05);

    std::string transcribeChunk(const AudioChunk& chunk) override;
    bool detectVoiceActivity(const AudioChunk& chunk) override;

    void setEnergyThreshold(double threshold) override;
    double getEnergyThreshold() const override;

private:
    double energy_threshold_;
    EchoSuppressionPolicy echo_policy_;
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_STT_ENGINE_HPP
