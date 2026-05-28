#include "../core/logging/logger.hpp"
#include "../core/voice/audio_types.hpp"
#include "../core/voice/audio_session_manager.hpp"
#include "../core/voice/stt_engine.hpp"
#include "../core/voice/tts_engine.hpp"
#include "../core/voice/voice_orchestrator.hpp"
#include "../core/watchdog/runtime_watchdog.hpp"
#include <iostream>
#include <cassert>
#include <thread>

int main() {
    std::cout << "=== VOICE LIFECYCLE BENCHMARK ===\n";
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::WARN);

    // Test 1: Audio session focus loss & recovery
    std::cout << "[TEST 1] Audio Session Focus & Route Recovery... ";
    auto& session = isha::AudioSessionManager::getInstance();
    session.setRoute("bluetooth");
    assert(session.getRoute() == "bluetooth");
    
    session.requestMicrophoneFocus();
    assert(session.isMicrophoneActive());
    assert(session.hasAudioFocus());

    session.simulateFocusLoss();
    assert(!session.hasAudioFocus());
    // mic deactivated upon focus loss if pause_on_loss is true
    assert(!session.isMicrophoneActive());
    
    session.simulateFocusGain();
    assert(session.hasAudioFocus());
    assert(session.isMicrophoneActive());
    
    session.abandonMicrophoneFocus();
    std::cout << "PASS\n";

    // Test 2: Voice state machine & User Interruption correctness
    std::cout << "[TEST 2] Voice Orchestrator State Transitions & Interruption... ";
    auto stt_ptr = std::make_shared<isha::WhisperSTTEngine>(0.05);
    auto tts_ptr = std::make_shared<isha::KokoroTTSEngine>();
    isha::VoiceOrchestrator orchestrator(stt_ptr, tts_ptr);
    
    orchestrator.start();
    assert(orchestrator.getState() == isha::VoiceState::IDLE);

    // Feed speaking chunk during IDLE -> transitions to LISTENING
    isha::AudioChunk c_speech;
    c_speech.samples.resize(800, 0.2f);
    orchestrator.handleMicrophoneInput(c_speech);
    
    // Process enough chunks to trigger transcription -> THINKING -> SPEAKING
    orchestrator.handleMicrophoneInput(c_speech);
    
    // Simulate user interruption while SPEAKING: feed high energy chunk
    orchestrator.setState(isha::VoiceState::SPEAKING);
    orchestrator.handleMicrophoneInput(c_speech); // voice detected while speaking -> triggers interruption
    
    // It should transition back to LISTENING after registering interruption
    assert(orchestrator.getState() == isha::VoiceState::LISTENING);
    std::cout << "PASS\n";

    // Test 3: Watchdog pipeline stall recovery
    std::cout << "[TEST 3] Watchdog Audio Pipeline Stall monitoring... ";
    isha::RuntimeWatchdog watchdog(std::chrono::milliseconds(10));
    watchdog.start();
    
    std::atomic<bool> stall_detected{false};
    isha::EventBus::getInstance().subscribe(
        isha::EventType::AUDIO_PIPELINE_STALL,
        "WatchdogAudioTest",
        [&](const isha::Event& ev) {
            if (ev.data.find("stream_id=mic_stream") != std::string::npos) {
                stall_detected.store(true);
            }
        }
    );

    watchdog.registerAudioPipeline("mic_stream", std::chrono::milliseconds(10));
    // Dynamic wait loop with timeout to avoid scheduling jitter failures
    for (int i = 0; i < 10; ++i) {
        if (stall_detected.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    
    assert(stall_detected.load());
    std::cout << "PASS (Watchdog successfully flagged stall)\n";

    // Cleanup
    isha::EventBus::getInstance().unsubscribe(isha::EventType::AUDIO_PIPELINE_STALL, "WatchdogAudioTest");
    watchdog.stop();
    orchestrator.stop();

    std::cout << "=== VOICE LIFECYCLE BENCHMARK COMPLETED SUCCESSFULLY ===\n";
    return 0;
}
