#include "tts_engine.hpp"
#include <chrono>
#include <thread>
#include <cmath>

namespace isha {

KokoroTTSEngine::KokoroTTSEngine() {}

std::vector<AudioChunk> KokoroTTSEngine::synthesize(const std::string& text, const std::string& stream_id) {
    std::vector<AudioChunk> chunks;
    if (text.empty()) {
        return chunks;
    }

    // Determine config based on quality mode
    unsigned int rate = 16000;
    size_t samples_per_chunk = 1600; // 100ms chunk
    if (quality_mode_.load() == TTSQuality::LOW_8KHZ) {
        rate = 8000;
        samples_per_chunk = 800; // 100ms chunk at lower rate
    }

    // Split text into simulated words/sentences
    size_t num_chunks = 3; // Synthesize in 3 chunks
    uint64_t start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    for (size_t i = 0; i < num_chunks; ++i) {
        AudioChunk chunk;
        chunk.sequence_id = i;
        chunk.timestamp = start_time + (i * 100);
        chunk.stream_id = stream_id;
        chunk.sample_rate = rate;
        
        // Generate a mock sine wave speech signal
        chunk.samples.resize(samples_per_chunk);
        double freq = 220.0; // pitch
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
    // Model latency simulation (<120ms target, returning constant mock benchmark baseline)
    if (quality_mode_.load() == TTSQuality::LOW_8KHZ) {
        return 45.0; // Lower resolution is faster
    }
    return 95.0; // High resolution is standard
}

} // namespace isha
