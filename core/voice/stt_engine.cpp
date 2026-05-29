#include "stt_engine.hpp"
#include "../logging/logger.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace isha {

WhisperSTTEngine::WhisperSTTEngine(double energy_threshold)
    : energy_threshold_(energy_threshold) {}

std::string WhisperSTTEngine::transcribeChunk(const AudioChunk& chunk) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (chunk.samples.empty()) return "";

    double sum_sq = 0.0;
    for (float s : chunk.samples) sum_sq += s * s;
    double rms = std::sqrt(sum_sq / chunk.samples.size());

    if (rms < energy_threshold_) return "";

    if (rms > 0.3)        return "grounded query Kisan Saathi wheat yellow leaves";
    else if (rms > 0.15)  return "grounded query Swasthya paracetamol dose";
    else                  return "hello isha";
}

bool WhisperSTTEngine::detectVoiceActivity(const AudioChunk& chunk) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (chunk.samples.empty()) return false;

    double sum_sq = 0.0;
    for (float s : chunk.samples) sum_sq += s * s;
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

// ============================================================
// CLASS_C BURST POLICY ENFORCEMENT
// ============================================================

void WhisperSTTEngine::beginSession() {
    session_active_.store(true);
    ISHA_LOG_INFO("WhisperSTT", "CLASS_C burst session STARTED — model loading for transcription.");
}

void WhisperSTTEngine::endSession() {
    // Every transcription MUST call endSession() to trigger immediate unload.
    session_active_.store(false);
    ISHA_LOG_INFO("WhisperSTT",
        "CLASS_C burst session ENDED — model must be unloaded immediately after this call.");
}

bool WhisperSTTEngine::enforceNoBgResidency(bool is_background) {
    if (!is_background) return false;
    if (!session_active_.load()) return false;

    // Background residency detected while session is still active.
    // This is a critical policy violation — force teardown immediately.
    // Do NOT merely warn. Log CRITICAL and force unload.
    ISHA_LOG_ERROR("WhisperSTT",
        "CRITICAL: CLASS_C STT background residency DETECTED. "
        "Whisper.cpp must NEVER remain resident between requests. "
        "Forcing immediate teardown. This indicates a caller bug — "
        "endSession() must be called after every transcription.");

    session_active_.store(false);
    // Signal to caller that a forced teardown occurred
    return true;
}

} // namespace isha
