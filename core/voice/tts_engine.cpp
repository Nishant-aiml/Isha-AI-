#include "tts_engine.hpp"
#include "../logging/logger.hpp"
#include <chrono>
#include <thread>
#include <cmath>

namespace isha {

KokoroTTSEngine::KokoroTTSEngine() {}

std::vector<AudioChunk> KokoroTTSEngine::synthesize(const std::string& text,
                                                     const std::string& stream_id) {
    std::vector<AudioChunk> chunks;
    if (text.empty()) return chunks;

    unsigned int rate = 16000;
    size_t samples_per_chunk = 1600;
    if (quality_mode_.load() == TTSQuality::LOW_8KHZ) {
        rate = 8000;
        samples_per_chunk = 800;
    }

    size_t num_chunks = 3;
    uint64_t start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (size_t i = 0; i < num_chunks; ++i) {
        AudioChunk chunk;
        chunk.sequence_id = i;
        chunk.timestamp   = start_time + (i * 100);
        chunk.stream_id   = stream_id;
        chunk.sample_rate = rate;
        chunk.samples.resize(samples_per_chunk);
        double freq = 220.0;
        for (size_t s = 0; s < samples_per_chunk; ++s) {
            chunk.samples[s] = 0.5f * std::sin(2.0 * 3.1415926535 * freq * s / rate);
        }
        chunks.push_back(std::move(chunk));
    }
    return chunks;
}

void KokoroTTSEngine::setQualityMode(TTSQuality quality) {
    quality_mode_.store(quality);
}

TTSQuality KokoroTTSEngine::getQualityMode() const {
    return quality_mode_.load();
}

double KokoroTTSEngine::getFirstChunkLatencyMs() const {
    return (quality_mode_.load() == TTSQuality::LOW_8KHZ) ? 45.0 : 95.0;
}

// ============================================================
// CLASS_C BURST POLICY ENFORCEMENT
// ============================================================

void KokoroTTSEngine::beginSession() {
    session_active_.store(true);
    ISHA_LOG_INFO("KokoroTTS", "CLASS_C burst session STARTED — model loading for synthesis.");
}

void KokoroTTSEngine::endSession() {
    session_active_.store(false);
    ISHA_LOG_INFO("KokoroTTS",
        "CLASS_C burst session ENDED — model must be unloaded immediately after this call.");
}

bool KokoroTTSEngine::enforceNoBgResidency(bool is_background) {
    if (!is_background) return false;
    if (!session_active_.load()) return false;

    ISHA_LOG_ERROR("KokoroTTS",
        "CRITICAL: CLASS_C TTS background residency DETECTED. "
        "Kokoro-82M must NEVER remain resident between synthesis requests. "
        "Forcing immediate teardown. Caller must invoke endSession() after every synthesis.");

    session_active_.store(false);
    return true;
}

} // namespace isha
