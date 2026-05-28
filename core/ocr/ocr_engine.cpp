#include "ocr_engine.hpp"
#include "../logging/logger.hpp"
#include <chrono>
#include <sstream>

namespace isha {

OCREngine::OCREngine() {
    ISHA_LOG_INFO("OCREngine", "Initialized.");
}

OCRResult OCREngine::extract(const uint8_t* image_data, uint32_t width, uint32_t height,
                             uint32_t channels, const CancellationToken& token) {
    auto start = std::chrono::steady_clock::now();
    constexpr auto deadline = std::chrono::milliseconds(50);

    OCRQuality quality = current_quality_.load(std::memory_order_acquire);

    // PAUSED: return cached result immediately
    if (quality == OCRQuality::PAUSED) {
        skip_count_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_DEBUG("OCREngine", "Paused due to thermal - returning cached result.");
        std::lock_guard<std::mutex> lock(mutex_);
        return last_result_;
    }

    // Cancellation checkpoint 1
    if (token.isCancelled()) {
        skip_count_.fetch_add(1, std::memory_order_relaxed);
        return {false, "", 0.0f, 0.0f, false};
    }

    // Simulated preprocessing - adjust based on quality
    uint32_t effective_width = width;
    uint32_t effective_height = height;
    bool degraded = false;

    if (quality == OCRQuality::REDUCED_RESOLUTION) {
        effective_width = width / 2;
        effective_height = height / 2;
        degraded = true;
    } else if (quality == OCRQuality::REDUCED_FREQUENCY) {
        effective_width = width / 2;
        effective_height = height / 2;
        degraded = true;
    }

    // Cancellation checkpoint 2 (mid-extraction)
    if (token.isCancelled()) {
        skip_count_.fetch_add(1, std::memory_order_relaxed);
        return {false, "", 0.0f, 0.0f, degraded};
    }

    // Deadline check
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed >= deadline) {
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        total_latency_us_.fetch_add(static_cast<uint64_t>(latency), std::memory_order_relaxed);
        extraction_count_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_WARN("OCREngine", "Deadline exceeded - returning partial result.");

        OCRResult partial;
        partial.success = true;
        partial.extracted_text = "[partial] image:" + std::to_string(effective_width) + "x" + std::to_string(effective_height);
        partial.confidence = 0.3f;
        partial.latency_ms = static_cast<float>(latency) / 1000.0f;
        partial.was_degraded = true;

        std::lock_guard<std::mutex> lock(mutex_);
        last_result_ = partial;
        return partial;
    }

    // Simulated extraction: generate placeholder text from image dimensions
    std::ostringstream oss;
    oss << "OCR[" << effective_width << "x" << effective_height << "x" << channels << "]";

    size_t pixel_count = static_cast<size_t>(effective_width) * effective_height;
    if (pixel_count > 100000) {
        oss << " dense_text region detected";
    } else {
        oss << " sparse_text region detected";
    }

    // Compute simulated confidence from dimensions
    float confidence = (quality == OCRQuality::FULL) ? 0.92f : 0.65f;

    auto end = std::chrono::steady_clock::now();
    auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    total_latency_us_.fetch_add(static_cast<uint64_t>(latency_us), std::memory_order_relaxed);
    extraction_count_.fetch_add(1, std::memory_order_relaxed);

    OCRResult result;
    result.success = true;
    result.extracted_text = oss.str();
    result.confidence = confidence;
    result.latency_ms = static_cast<float>(latency_us) / 1000.0f;
    result.was_degraded = degraded;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        last_result_ = result;
    }

    ISHA_LOG_DEBUG("OCREngine", "Extraction complete in " + std::to_string(result.latency_ms) + "ms, confidence=" + std::to_string(confidence));
    return result;
}

void OCREngine::setThermalDegradation(double temperature_c) {
    OCRQuality new_quality;
    if (temperature_c < 42.0) {
        new_quality = OCRQuality::FULL;
    } else if (temperature_c < 45.0) {
        new_quality = OCRQuality::REDUCED_RESOLUTION;
    } else if (temperature_c < 48.0) {
        new_quality = OCRQuality::REDUCED_FREQUENCY;
    } else {
        new_quality = OCRQuality::PAUSED;
    }

    OCRQuality old_quality = current_quality_.exchange(new_quality, std::memory_order_acq_rel);
    if (old_quality != new_quality) {
        ISHA_LOG_INFO("OCREngine", "Thermal degradation changed: temp=" + std::to_string(temperature_c) +
                      "C, quality=" + std::to_string(static_cast<int>(new_quality)));
    }
}

OCRQuality OCREngine::getCurrentQuality() const {
    return current_quality_.load(std::memory_order_acquire);
}

float OCREngine::totalLatencyMs() const {
    return static_cast<float>(total_latency_us_.load(std::memory_order_relaxed)) / 1000.0f;
}

} // namespace isha
