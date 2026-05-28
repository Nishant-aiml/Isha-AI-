#include "vision_engine.hpp"
#include "../logging/logger.hpp"
#include <chrono>
#include <sstream>

namespace isha {

VisionEngine::VisionEngine() {
    ISHA_LOG_INFO("VisionEngine", "Initialized.");
}

VisionResult VisionEngine::analyze(const uint8_t* image_data, uint32_t width, uint32_t height,
                                   uint32_t channels, const CancellationToken& token) {
    auto start = std::chrono::steady_clock::now();
    constexpr auto deadline = std::chrono::milliseconds(150);

    // Cancellation checkpoint 1
    if (token.isCancelled()) {
        return {false, "", {}, 0.0f, 0.0f};
    }

    // Phase 1: Generate scene description from dimensions/channels
    std::ostringstream desc;
    size_t pixel_count = static_cast<size_t>(width) * height;

    if (channels == 1) {
        desc << "Grayscale image";
    } else if (channels == 3) {
        desc << "Color image (RGB)";
    } else if (channels == 4) {
        desc << "Color image (RGBA)";
    } else {
        desc << "Multi-channel image (" << channels << "ch)";
    }

    if (pixel_count > 2000000) {
        desc << ", high-resolution (" << width << "x" << height << ")";
    } else if (pixel_count > 500000) {
        desc << ", medium-resolution (" << width << "x" << height << ")";
    } else {
        desc << ", low-resolution (" << width << "x" << height << ")";
    }

    // Deadline check after phase 1
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed >= deadline) {
        auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        total_latency_us_.fetch_add(static_cast<uint64_t>(latency_us), std::memory_order_relaxed);
        analysis_count_.fetch_add(1, std::memory_order_relaxed);
        timeout_count_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_WARN("VisionEngine", "Deadline exceeded after scene description - returning partial.");

        return {true, desc.str(), {}, static_cast<float>(latency_us) / 1000.0f, 0.4f};
    }

    // Cancellation checkpoint 2 (mid-analysis)
    if (token.isCancelled()) {
        return {false, desc.str(), {}, 0.0f, 0.0f};
    }

    // Phase 2: Synthetic object detection based on image properties
    std::vector<std::string> objects;

    if (pixel_count > 1000000) {
        objects.push_back("background_region");
        objects.push_back("foreground_subject");
    }
    if (channels >= 3) {
        objects.push_back("color_region");
    }
    if (width > height * 2) {
        objects.push_back("panoramic_element");
    } else if (height > width * 2) {
        objects.push_back("vertical_element");
    }
    if (pixel_count > 500000) {
        objects.push_back("detail_region");
    }

    // Deadline check after phase 2
    elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed >= deadline) {
        auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        total_latency_us_.fetch_add(static_cast<uint64_t>(latency_us), std::memory_order_relaxed);
        analysis_count_.fetch_add(1, std::memory_order_relaxed);
        timeout_count_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_WARN("VisionEngine", "Deadline exceeded after object detection - returning partial.");

        return {true, desc.str(), objects, static_cast<float>(latency_us) / 1000.0f, 0.6f};
    }

    // Compute confidence
    float confidence = 0.85f;
    if (pixel_count > 1000000 && channels >= 3) {
        confidence = 0.92f;
    }

    auto end = std::chrono::steady_clock::now();
    auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    total_latency_us_.fetch_add(static_cast<uint64_t>(latency_us), std::memory_order_relaxed);
    analysis_count_.fetch_add(1, std::memory_order_relaxed);

    float latency_ms = static_cast<float>(latency_us) / 1000.0f;

    ISHA_LOG_DEBUG("VisionEngine", "Analysis complete in " + std::to_string(latency_ms) +
                   "ms, objects=" + std::to_string(objects.size()) +
                   ", confidence=" + std::to_string(confidence));

    return {true, desc.str(), std::move(objects), latency_ms, confidence};
}

float VisionEngine::totalLatencyMs() const {
    return static_cast<float>(total_latency_us_.load(std::memory_order_relaxed)) / 1000.0f;
}

} // namespace isha
