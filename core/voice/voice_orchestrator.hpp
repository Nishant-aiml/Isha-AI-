#ifndef ISHA_AI_VOICE_ORCHESTRATOR_HPP
#define ISHA_AI_VOICE_ORCHESTRATOR_HPP

#include "audio_types.hpp"
#include "audio_buffer_manager.hpp"
#include "stt_engine.hpp"
#include "tts_engine.hpp"
#include "../runtime/event_bus.hpp"
#include <memory>
#include <mutex>
#include <atomic>

namespace isha {

class VoiceOrchestrator {
public:
    VoiceOrchestrator(
        std::shared_ptr<IWhisperSTTEngine> stt_engine,
        std::shared_ptr<IKokoroTTSEngine> tts_engine
    );
    ~VoiceOrchestrator();

    void start();
    void stop();

    // Push mic audio chunk to orchestrator processing pipeline
    void handleMicrophoneInput(const AudioChunk& chunk);

    // Simulated playback feed to support EchoSuppression subtraction
    void handlePlaybackReference(const AudioChunk& chunk);

    VoiceState getState() const;
    void setState(VoiceState state);

    // Trigger user interruption explicitly
    void triggerInterruption();

    std::shared_ptr<AudioBufferManager> getPlaybackBuffer() { return playback_buffer_; }

    void setTemperature(double temp) { thermal_temp_.store(temp); }
    double getTemperature() const { return thermal_temp_.load(); }
    
    void setBatterySaver(bool active) { battery_saver_active_.store(active); }
    bool isBatterySaverActive() const { return battery_saver_active_.load(); }
    
    void setDeviceTier(int tier) { device_tier_.store(tier); }
    int getDeviceTier() const { return device_tier_.load(); }

private:
    void processAudioLoop();
    void resetPipeline();

    std::shared_ptr<IWhisperSTTEngine> stt_engine_;
    std::shared_ptr<IKokoroTTSEngine> tts_engine_;

    std::shared_ptr<AudioBufferManager> mic_buffer_;
    std::shared_ptr<AudioBufferManager> playback_buffer_;

    std::atomic<double> thermal_temp_{35.0};
    std::atomic<bool> battery_saver_active_{false};
    std::atomic<int> device_tier_{2}; // 1 = low, 2 = mid, 3 = high

    std::atomic<VoiceState> state_{VoiceState::IDLE};
    std::atomic<bool> is_running_{false};
    
    std::vector<float> last_playback_reference_;
    
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_VOICE_ORCHESTRATOR_HPP
