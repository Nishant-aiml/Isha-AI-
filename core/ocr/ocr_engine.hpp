#ifndef ISHA_AI_OCR_ENGINE_HPP
#define ISHA_AI_OCR_ENGINE_HPP

#include <string>
#include <cstdint>
#include <atomic>
#include <mutex>
#include "../inference/cancellation_token.hpp"

namespace isha {

enum class OCRQuality {
    FULL,
    REDUCED_RESOLUTION,
    REDUCED_FREQUENCY,
    PAUSED
};

struct OCRResult {
    bool success = false;
    std::string extracted_text;
    float confidence = 0.0f;
    float latency_ms = 0.0f;
    bool was_degraded = false;
};

class OCREngine {
public:
    OCREngine();
    ~OCREngine() = default;

    // Core extraction - must complete within 50ms or return partial
    OCRResult extract(const uint8_t* image_data, uint32_t width, uint32_t height,
                      uint32_t channels, const CancellationToken& token);

    // Thermal degradation control
    void setThermalDegradation(double temperature_c);
    OCRQuality getCurrentQuality() const;

    // Statistics
    unsigned int extractionCount() const { return extraction_count_.load(); }
    unsigned int skipCount() const { return skip_count_.load(); }
    float totalLatencyMs() const;

private:
    std::atomic<OCRQuality> current_quality_{OCRQuality::FULL};
    std::atomic<unsigned int> extraction_count_{0};
    std::atomic<unsigned int> skip_count_{0};
    std::atomic<uint64_t> total_latency_us_{0}; // stored as microseconds for atomic precision

    mutable std::mutex mutex_;

    // Last cached result for PAUSED mode
    OCRResult last_result_;
};

} // namespace isha

#endif // ISHA_AI_OCR_ENGINE_HPP
