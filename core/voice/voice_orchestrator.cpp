#include "voice_orchestrator.hpp"
#include "../logging/logger.hpp"
#include <chrono>

namespace isha {

VoiceOrchestrator::VoiceOrchestrator(
    std::shared_ptr<IWhisperSTTEngine> stt_engine,
    std::shared_ptr<IKokoroTTSEngine> tts_engine
) : stt_engine_(stt_engine), tts_engine_(tts_engine) {
    mic_buffer_ = std::make_shared<AudioBufferManager>(1024 * 1024, 100);
    playback_buffer_ = std::make_shared<AudioBufferManager>(2 * 1024 * 1024, 200);
    
    // Subscribe to Focus loss to pause processing
    EventBus::getInstance().subscribe(
        EventType::AUDIO_FOCUS_LOST,
        "VoiceOrchestrator",
        [this](const Event& ev) {
            std::lock_guard<std::mutex> lock(mutex_);
            Logger::getInstance().log(LogLevel::WARN, "VoiceOrchestrator", "Audio focus lost. Pausing pipelines.");
            setState(VoiceState::IDLE);
        }
    );
}

VoiceOrchestrator::~VoiceOrchestrator() {
    stop();
    EventBus::getInstance().unsubscribe(EventType::AUDIO_FOCUS_LOST, "VoiceOrchestrator");
}

void VoiceOrchestrator::start() {
    is_running_.store(true);
    setState(VoiceState::IDLE);
}

void VoiceOrchestrator::stop() {
    is_running_.store(false);
    resetPipeline();
}

void VoiceOrchestrator::handleMicrophoneInput(const AudioChunk& chunk) {
    if (!is_running_.load()) return;

    // Evaluate thermal state conditions
    double temp = thermal_temp_.load();
    bool battery_saver = battery_saver_active_.load();
    
    // Low tier devices scale down threshold limits by 2C
    double offset = (device_tier_.load() == 1) ? 2.0 : 0.0;
    
    // Subsystem degradation parameters
    int min_chunks_for_stt = 2;
    unsigned int max_tokens_to_gen = 64;
    
    if (temp >= (48.0 - offset)) {
        // 48C+ sustained: emergency unload allowed
        Logger::getInstance().log(LogLevel::WARN, "VoiceOrchestrator", "Extreme thermal threshold hit! Triggering emergency unload.");
        EventBus::getInstance().publish({EventType::MODEL_UNLOADING, "VoiceOrchestrator", "thermal_critical"});
        stop();
        return;
    } else if (temp >= (45.0 - offset)) {
        // 45-48C: degrade TTS fidelity, lower sample rate, reduce max token budget
        tts_engine_->setQualityMode(TTSQuality::LOW_8KHZ);
        max_tokens_to_gen = 16;
        min_chunks_for_stt = 4; // reduce chunk frequency
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Aggressive sleep throttle
    } else if (temp >= (42.0 - offset)) {
        // 42-45C: aggressive scheduler throttling, reduce token budget, reduce STT chunk frequency
        max_tokens_to_gen = 32;
        min_chunks_for_stt = 3;
        std::this_thread::sleep_for(std::chrono::milliseconds(2)); // Throttling delay
    } else if (temp >= (38.0 - offset)) {
        // 38-42C: mild scheduler throttling
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
        tts_engine_->setQualityMode(TTSQuality::HIGH_16KHZ);
    }

    if (battery_saver) {
        // Battery saver further limits generation length
        max_tokens_to_gen = std::min(max_tokens_to_gen, 24u);
    }

    // Check VAD for user speech
    bool has_voice = stt_engine_->detectVoiceActivity(chunk);
    
    VoiceState current_state = state_.load();
    
    // 1. Interruption Check: if speaking and user starts talking, abort speaking instantly
    if (current_state == VoiceState::SPEAKING && has_voice) {
        triggerInterruption();
        return;
    }

    // Capture microphone chunk with audio overload governance
    if (mic_buffer_->getQueuedFramesCount() >= mic_buffer_->getMaxQueuedFrames()) {
        // Drop oldest chunk
        AudioChunk drop_chunk;
        mic_buffer_->popChunk(drop_chunk);
        Logger::getInstance().log(LogLevel::WARN, "VoiceOrchestrator", "Audio queue overload! Dropping oldest mic chunk.");
    }
    mic_buffer_->pushChunk(chunk);

    if (current_state == VoiceState::IDLE && has_voice) {
        setState(VoiceState::LISTENING);
    }

    if (state_.load() == VoiceState::LISTENING) {
        if (mic_buffer_->getQueuedFramesCount() >= static_cast<size_t>(min_chunks_for_stt)) {
            setState(VoiceState::THINKING);
            
            AudioChunk raw_accum;
            if (mic_buffer_->popChunk(raw_accum)) {
                // If playback reference exists, apply echo suppression
                if (!last_playback_reference_.empty()) {
                    EchoSuppressionPolicy suppressor;
                    suppressor.suppressEcho(raw_accum.samples, last_playback_reference_);
                }

                std::string text = stt_engine_->transcribeChunk(raw_accum);
                Logger::getInstance().log(LogLevel::INFO, "VoiceOrchestrator", "STT transcribed text: " + text);
                
                EventBus::getInstance().publish({
                    EventType::RETRIEVAL_COMPLETE,
                    "VoiceOrchestrator",
                    "text=" + text + ",max_tokens=" + std::to_string(max_tokens_to_gen)
                });

                // Trigger TTS generation
                setState(VoiceState::SPEAKING);
                std::string tts_stream_id = "voice_stream_" + std::to_string(raw_accum.sequence_id);
                std::vector<AudioChunk> tts_chunks = tts_engine_->synthesize("Mock response details.", tts_stream_id);
                
                for (const auto& tc : tts_chunks) {
                    if (playback_buffer_->getQueuedFramesCount() >= playback_buffer_->getMaxQueuedFrames()) {
                        AudioChunk drop;
                        playback_buffer_->popChunk(drop); // Audio overload drop oldest
                    }
                    playback_buffer_->pushChunk(tc);
                }
            } else {
                setState(VoiceState::IDLE);
            }
        }
    }
}

void VoiceOrchestrator::handlePlaybackReference(const AudioChunk& chunk) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_playback_reference_ = chunk.samples;
}

VoiceState VoiceOrchestrator::getState() const {
    return state_.load();
}

void VoiceOrchestrator::setState(VoiceState state) {
    if (state_.load() != state) {
        state_.store(state);
        Logger::getInstance().log(LogLevel::INFO, "VoiceOrchestrator", "Voice state transitioned to: " + voiceStateToString(state));
    }
}

void VoiceOrchestrator::triggerInterruption() {
    setState(VoiceState::INTERRUPTED);
    resetPipeline();
    
    EventBus::getInstance().publish({
        EventType::AUDIO_INTERRUPT_TRIGGERED,
        "VoiceOrchestrator",
        "Interrupted by user voice energy VAD"
    });
    
    // Auto restore to listening for next turn
    setState(VoiceState::LISTENING);
}

void VoiceOrchestrator::resetPipeline() {
    mic_buffer_->clear();
    playback_buffer_->clear();
    last_playback_reference_.clear();
}

} // namespace isha
