#include "../core/logging/logger.hpp"
#include "../core/runtime/event_bus.hpp"
#include "../core/inference/gguf_inference_engine.hpp"
#include "../core/scheduler/inference_scheduler.hpp"
#include "../core/voice/voice_orchestrator.hpp"
#include "../core/voice/stt_engine.hpp"
#include "../core/voice/tts_engine.hpp"
#include "../core/watchdog/runtime_watchdog.hpp"
#include "../core/observability/telemetry.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>

int main() {
    std::cout << "=== REALTIME HARDENING BENCHMARK ===\n";
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::WARN);

    auto engine = std::make_shared<isha::GGUFInferenceEngine>();
    isha::InferenceContext context("session_harden", nullptr, 64);
    engine->load("valid_model.gguf", context);

    // Test 1: Interruption storms under stream load
    std::cout << "[TEST 1] Interruption Storms and Preemption... ";
    auto stt = std::make_shared<isha::WhisperSTTEngine>(0.05);
    auto tts = std::make_shared<isha::KokoroTTSEngine>();
    isha::VoiceOrchestrator orchestrator(stt, tts);
    orchestrator.start();
    
    // Simulating parallel rapid interruption events
    std::vector<std::thread> storm_threads;
    for (int i = 0; i < 20; ++i) {
        storm_threads.emplace_back([&]() {
            isha::AudioChunk speaking;
            speaking.samples.resize(800, 0.3f);
            orchestrator.handleMicrophoneInput(speaking);
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        });
    }
    for (auto& t : storm_threads) t.join();
    assert(orchestrator.getState() != isha::VoiceState::ERROR);
    std::cout << "PASS\n";

    // Test 2: Thermal Escalation Windows & Throttling
    std::cout << "[TEST 2] Thermal Escalation Policies... ";
    
    // Standard baseline temp
    orchestrator.setTemperature(35.0);
    isha::AudioChunk speech_chunk;
    speech_chunk.samples.resize(800, 0.25f);
    orchestrator.handleMicrophoneInput(speech_chunk);
    assert(tts->getQualityMode() == isha::TTSQuality::HIGH_16KHZ);

    // Stage 1: 40C (Mild Throttling)
    orchestrator.setTemperature(40.0);
    orchestrator.handleMicrophoneInput(speech_chunk);
    assert(tts->getQualityMode() == isha::TTSQuality::HIGH_16KHZ);

    // Stage 2: 44C (Aggressive Throttling)
    orchestrator.setTemperature(44.0);
    orchestrator.handleMicrophoneInput(speech_chunk);
    assert(tts->getQualityMode() == isha::TTSQuality::HIGH_16KHZ);

    // Stage 3: 46C (TTS Sample degradation & lower limits)
    orchestrator.setTemperature(46.0);
    orchestrator.handleMicrophoneInput(speech_chunk);
    assert(tts->getQualityMode() == isha::TTSQuality::LOW_8KHZ);
    
    // Stage 4: 49C (Emergency unload)
    std::atomic<bool> emergency_unloaded{false};
    isha::EventBus::getInstance().subscribe(
        isha::EventType::MODEL_UNLOADING,
        "ThermalHardenTest",
        [&](const isha::Event& ev) {
            if (ev.data == "thermal_critical") {
                emergency_unloaded.store(true);
            }
        }
    );

    orchestrator.setTemperature(49.0);
    orchestrator.handleMicrophoneInput(speech_chunk);
    assert(emergency_unloaded.load());
    std::cout << "PASS\n";

    // Test 3: Mmap residency & cooldowns
    std::cout << "[TEST 3] Mmap Residency Cooldown protection... ";
    engine->unload();
    bool remap_ok = engine->load("valid_model.gguf", context);
    assert(remap_ok);
    std::cout << "PASS\n";

    // Test 4: Android Lifecycle Chaos Simulation
    std::cout << "[TEST 4] Android Lifecycle Chaos Recovery... ";
    orchestrator.start();
    
    // Focus loss gate
    isha::EventBus::getInstance().publish({
        isha::EventType::AUDIO_FOCUS_LOST,
        "SystemAndroidMock",
        "focus_loss_transient"
    });
    // State goes to IDLE on focus loss
    assert(orchestrator.getState() == isha::VoiceState::IDLE);
    
    // battery saver limits activation
    orchestrator.setBatterySaver(true);
    assert(orchestrator.isBatterySaverActive());
    std::cout << "PASS\n";

    // Test 5: Latency and Fragmentation telemetry
    std::cout << "[TEST 5] Telemetry distribution tracking... ";
    isha::Telemetry telemetry(10);
    isha::ProfilingSnapshot snap1;
    snap1.timestamp = std::chrono::system_clock::now();
    snap1.inference_latency_ms = 45.0;
    snap1.scheduler_latency_ms = 2.0;
    snap1.ram_used_mb = 120;
    snap1.model_mapped_mb = 950;
    snap1.model_loaded = true;
    
    telemetry.recordSnapshot(snap1);
    
    assert(telemetry.getP50InferenceLatency() == 45.0);
    assert(telemetry.getFragmentationRatio() > 0.0);
    std::cout << "PASS\n";

    // Cleanup
    isha::EventBus::getInstance().unsubscribe(isha::EventType::MODEL_UNLOADING, "ThermalHardenTest");
    orchestrator.stop();
    engine->unload();

    std::cout << "=== REALTIME HARDENING BENCHMARK COMPLETED SUCCESSFULLY ===\n";
    return 0;
}
