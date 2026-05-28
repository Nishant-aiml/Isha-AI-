#ifndef ISHA_AI_IMAGE_PIPELINE_HPP
#define ISHA_AI_IMAGE_PIPELINE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include "image_context.hpp"
#include "image_decoder.hpp"
#include "../inference/cancellation_token.hpp"

namespace isha {

struct NormalizedImage {
    std::unique_ptr<float[]> data;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;

    NormalizedImage() = default;
    NormalizedImage(NormalizedImage&&) = default;
    NormalizedImage& operator=(NormalizedImage&&) = default;
    NormalizedImage(const NormalizedImage&) = delete;
    NormalizedImage& operator=(const NormalizedImage&) = delete;
};

struct PipelineResult {
    bool success = false;
    DecodedImage image;
    NormalizedImage normalized;
    float preprocessing_time_ms = 0.0f;
    std::string error;

    PipelineResult() = default;
    PipelineResult(PipelineResult&&) = default;
    PipelineResult& operator=(PipelineResult&&) = default;
};

class ImagePipeline {
public:
    ImagePipeline();
    ~ImagePipeline() = default;

    ImagePipeline(const ImagePipeline&) = delete;
    ImagePipeline& operator=(const ImagePipeline&) = delete;

    PipelineResult process(const uint8_t* raw_data, size_t size,
                           ImageSource source, const CancellationToken& token);

private:
    ImageDecoder decoder_;

    // Normalize uint8 [0,255] to float [0.0, 1.0]
    NormalizedImage normalize(const DecodedImage& decoded, const CancellationToken& token);
};

} // namespace isha

#endif // ISHA_AI_IMAGE_PIPELINE_HPP
