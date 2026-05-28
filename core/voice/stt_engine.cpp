#include "stt_engine.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace isha {

WhisperSTTEngine::WhisperSTTEngine(double energy_threshold)
    : energy_threshold_(energy_threshold) {}

std::string WhisperSTTEngine::transcribeChunk(const AudioChunk& chunk) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (chunk.samples.empty()) {
        return "";
    }

    // VAD check first
    double sum_sq = 0.0;
    for (float s : chunk.samples) {
        sum_sq += s * s;
    }
    double rms = std::sqrt(sum_sq / chunk.samples.size());
    
    if (rms < energy_threshold_) {
        return ""; // No speech detected
    }

    // Mock speech-to-text transcription mapping based on amplitude characteristics
    // In production, this invokes whisper.cpp model inference.
    if (rms > 0.3) {
        return "grounded query Kisan Saathi wheat yellow leaves";
    } else if (rms > 0.15) {
        return "grounded query Swasthya paracetamol dose";
    } else {
        return "hello isha";
    }
}

bool WhisperSTTEngine::detectVoiceActivity(const AudioChunk& chunk) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (chunk.samples.empty()) {
        return false;
    }

    double sum_sq = 0.0;
    for (float s : chunk.samples) {
        sum_sq += s * s;
    }
    double rms = std::sqrt(sum_sq / chunk.samples.size());
    
    return rms >= energy_threshold_;
}

void WhisperSTTEngine::setEnergyThreshold(double threshold) {
    std::lock_guard<std::mutex> lock(mutex_);
    energy_threshold_ = threshold;
}

double WhisperSTTEngine::getEnergyThreshold() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return energy_threshold_;
}

} // namespace isha
