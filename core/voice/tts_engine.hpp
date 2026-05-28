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

    virtual std::vector<AudioChunk> synthesize(const std::string& text, const std::string& stream_id) = 0;
    virtual void setQualityMode(TTSQuality quality) = 0;
    virtual TTSQuality getQualityMode() const = 0;
    virtual double getFirstChunkLatencyMs() const = 0;
};

class KokoroTTSEngine : public IKokoroTTSEngine {
public:
    KokoroTTSEngine();

    std::vector<AudioChunk> synthesize(const std::string& text, const std::string& stream_id) override;
    
    void setQualityMode(TTSQuality quality) override;
    TTSQuality getQualityMode() const override;
    double getFirstChunkLatencyMs() const override;

private:
    std::atomic<TTSQuality> quality_mode_{TTSQuality::HIGH_16KHZ};
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_TTS_ENGINE_HPP
