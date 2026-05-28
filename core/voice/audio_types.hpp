#ifndef ISHA_AI_AUDIO_TYPES_HPP
#define ISHA_AI_AUDIO_TYPES_HPP

#include <vector>
#include <string>
#include <chrono>

namespace isha {

enum class VoiceState {
    IDLE,
    LISTENING,
    THINKING,
    SPEAKING,
    INTERRUPTED,
    ERROR
};

inline std::string voiceStateToString(VoiceState state) {
    switch (state) {
        case VoiceState::IDLE: return "IDLE";
        case VoiceState::LISTENING: return "LISTENING";
        case VoiceState::THINKING: return "THINKING";
        case VoiceState::SPEAKING: return "SPEAKING";
        case VoiceState::INTERRUPTED: return "INTERRUPTED";
        case VoiceState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

struct AudioChunk {
    uint64_t sequence_id = 0;
    uint64_t timestamp = 0; // Epoch milliseconds
    std::string stream_id;
    std::vector<float> samples;
    unsigned int sample_rate = 16000;
};

} // namespace isha

#endif // ISHA_AI_AUDIO_TYPES_HPP
