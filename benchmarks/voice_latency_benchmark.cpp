#include "../core/logging/logger.hpp"
#include "../core/voice/audio_types.hpp"
#include "../core/voice/stt_engine.hpp"
#include "../core/voice/tts_engine.hpp"
#include "../core/voice/audio_buffer_manager.hpp"
#include <iostream>
#include <cassert>
#include <chrono>

int main() {
    std::cout << "=== VOICE LATENCY BENCHMARK ===\n";
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::WARN);

    // Test 1: Bounded memory and overflow rejection
    std::cout << "[TEST 1] Hard audio buffer memory caps... ";
    // Set 10KB memory limit (each float sample is 4 bytes, so 2500 samples limit)
    isha::AudioBufferManager buffer_manager(10 * 1024, 10);
    
    // Push small chunk (800 samples = 3.2KB)
    isha::AudioChunk c1;
    c1.samples.resize(800, 0.1f);
    bool ok1 = buffer_manager.pushChunk(c1);
    assert(ok1);
    
    // Push second chunk (total ~6.4KB)
    bool ok2 = buffer_manager.pushChunk(c1);
    assert(ok2);

    // Push third chunk (total ~9.6KB)
    bool ok3 = buffer_manager.pushChunk(c1);
    assert(ok3);
    
    // Push fourth chunk (will exceed 10KB, triggering eviction/rejection constraints)
    bool ok4 = buffer_manager.pushChunk(c1);
    // Since max capacity is 10KB, circular eviction popped oldest to make space
    assert(buffer_manager.getCurrentMemoryBytes() <= 10 * 1024);
    std::cout << "PASS (current memory: " << buffer_manager.getCurrentMemoryBytes() << " bytes)\n";

    // Test 2: Voice Activity Detection (RMS energy-threshold)
    std::cout << "[TEST 2] RMS Energy-Threshold VAD... ";
    isha::WhisperSTTEngine stt(0.05);
    
    // Silent chunk
    isha::AudioChunk silence;
    silence.samples.resize(1000, 0.0f);
    assert(!stt.detectVoiceActivity(silence));
    
    // Speaking chunk
    isha::AudioChunk speech;
    speech.samples.resize(1000, 0.2f); // steady high energy
    assert(stt.detectVoiceActivity(speech));
    std::cout << "PASS\n";

    // Test 3: Kokoro synthesis latency
    std::cout << "[TEST 3] Kokoro first-chunk latency... ";
    isha::KokoroTTSEngine tts;
    double high_lat = tts.getFirstChunkLatencyMs();
    assert(high_lat < 120.0); // High quality target < 120ms
    
    tts.setQualityMode(isha::TTSQuality::LOW_8KHZ);
    double low_lat = tts.getFirstChunkLatencyMs();
    assert(low_lat < high_lat); // Low quality should be faster
    std::cout << "PASS (high_lat=" << high_lat << "ms, low_lat=" << low_lat << "ms)\n";

    std::cout << "=== VOICE LATENCY BENCHMARK COMPLETED SUCCESSFULLY ===\n";
    return 0;
}
