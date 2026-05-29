#ifndef ISHA_AI_TTS_ENGINE_HPP
#define ISHA_AI_TTS_ENGINE_HPP

#include "audio_types.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

namespace isha {

enum class TTSQuality {
    HIGH_16KHZ,
    LOW_8KHZ
};

class IKokoroTTSEngine {
public:
    virtual ~IKokoroTTSEngine() = default;

    virtual std::vector<AudioChunk> synthesize(const std::string& text,
                                               const std::string& stream_id) = 0;
    virtual void setQualityMode(TTSQuality quality) = 0;
    virtual TTSQuality getQualityMode() const = 0;
    virtual double getFirstChunkLatencyMs() const = 0;
};

// ============================================================
// KokoroTTSEngine — CLASS_C: BURST LOAD ONLY
//
// RESIDENCY POLICY:
//   - Never resident between user requests.
//   - Load → synthesize → stream audio → forceUnload() immediately.
//   - NO background TTS residency. NO idle TTS daemon loops.
//   - If background residency detected: forceUnload() + CRITICAL log.
//
// REASON: Continuous residency causes battery drain, standby wakeups,
//         thermal buildup, LMK pressure, and OEM kill aggression.
//
// CANNOT co-exist with CLASS_D (T2/T3) due to HeavyComputeSemaphore.
// ============================================================
class KokoroTTSEngine : public IKokoroTTSEngine {
public:
    KokoroTTSEngine();

    std::vector<AudioChunk> synthesize(const std::string& text,
                                       const std::string& stream_id) override;
    void setQualityMode(TTSQuality quality) override;
    TTSQuality getQualityMode() const override;
    double getFirstChunkLatencyMs() const override;

    // CLASS_C burst policy enforcement
    static bool isResidentAllowed() { return false; }

    // Force teardown on background residency detection.
    // Returns true if forced teardown was necessary (CRITICAL logged).
    bool enforceNoBgResidency(bool is_background);

    // Session tracking for burst boundary enforcement
    bool isSessionActive() const { return session_active_.load(); }
    void beginSession();
    void endSession(); // MUST be called after every synthesis. Triggers unload.

private:
    std::atomic<TTSQuality> quality_mode_{TTSQuality::HIGH_16KHZ};
    mutable std::mutex mutex_;
    std::atomic<bool> session_active_{false};
};

} // namespace isha

#endif // ISHA_AI_TTS_ENGINE_HPP
