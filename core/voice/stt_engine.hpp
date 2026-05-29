#ifndef ISHA_AI_STT_ENGINE_HPP
#define ISHA_AI_STT_ENGINE_HPP

#include "audio_types.hpp"
#include <memory>
#include <mutex>
#include <atomic>

namespace isha {

class EchoSuppressionPolicy {
public:
    EchoSuppressionPolicy() = default;
    void suppressEcho(std::vector<float>& microphone_samples,
                      const std::vector<float>& playback_samples) {
        size_t limit = std::min(microphone_samples.size(), playback_samples.size());
        for (size_t i = 0; i < limit; ++i) {
            microphone_samples[i] -= 0.1f * playback_samples[i];
            if (microphone_samples[i] >  1.0f) microphone_samples[i] =  1.0f;
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

// ============================================================
// WhisperSTTEngine — CLASS_C: BURST LOAD ONLY
//
// RESIDENCY POLICY:
//   - Never resident between user requests.
//   - Load → execute transcription → stream result → forceUnload() immediately.
//   - Background residency is FORBIDDEN.
//   - If background residency detected: forceUnload() + CRITICAL log (not just WARN).
//
// CANNOT co-exist with CLASS_D (T2/T3) due to HeavyComputeSemaphore.
// Max burst duration: 30 seconds per transcription session.
// ============================================================
class WhisperSTTEngine : public IWhisperSTTEngine {
public:
    WhisperSTTEngine(double energy_threshold = 0.05);

    std::string transcribeChunk(const AudioChunk& chunk) override;
    bool detectVoiceActivity(const AudioChunk& chunk) override;

    void setEnergyThreshold(double threshold) override;
    double getEnergyThreshold() const override;

    // CLASS_C burst policy enforcement
    // Returns false always — STT must NEVER be kept resident between requests.
    static bool isResidentAllowed() { return false; }

    // Called if background residency is detected.
    // Does NOT log WARN — triggers forceUnload() + CRITICAL log.
    // Returns true if a forced teardown was necessary.
    bool enforceNoBgResidency(bool is_background);

    // Session tracking — used to enforce 30s burst limit
    bool isSessionActive() const { return session_active_.load(); }
    void beginSession();
    void endSession(); // MUST be called after every transcription. Triggers unload.

private:
    double energy_threshold_;
    EchoSuppressionPolicy echo_policy_;
    mutable std::mutex mutex_;
    std::atomic<bool> session_active_{false};
};

} // namespace isha

#endif // ISHA_AI_STT_ENGINE_HPP
