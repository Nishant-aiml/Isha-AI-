#ifndef ISHA_AI_VISION_ENGINE_HPP
#define ISHA_AI_VISION_ENGINE_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include "../inference/cancellation_token.hpp"

namespace isha {

struct VisionResult {
    bool success = false;
    std::string scene_description;
    std::vector<std::string> detected_objects;
    float latency_ms = 0.0f;
    float confidence = 0.0f;
};

class VisionEngine {
public:
    VisionEngine();
    ~VisionEngine() = default;

    // Core analysis - must complete within 150ms or return partial
    VisionResult analyze(const uint8_t* image_data, uint32_t width, uint32_t height,
                         uint32_t channels, const CancellationToken& token);

    // Statistics
    unsigned int analysisCount() const { return analysis_count_.load(); }
    unsigned int timeoutCount() const { return timeout_count_.load(); }
    float totalLatencyMs() const;

private:
    std::atomic<unsigned int> analysis_count_{0};
    std::atomic<unsigned int> timeout_count_{0};
    std::atomic<uint64_t> total_latency_us_{0};
};

} // namespace isha

#endif // ISHA_AI_VISION_ENGINE_HPP
