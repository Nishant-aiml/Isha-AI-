#include "image_pipeline.hpp"
#include "../logging/logger.hpp"
#include <chrono>

namespace isha {

ImagePipeline::ImagePipeline() {
    ISHA_LOG_INFO("ImagePipeline", "Initialized");
}

NormalizedImage ImagePipeline::normalize(const DecodedImage& decoded, const CancellationToken& token) {
    NormalizedImage norm;
    size_t pixel_count = static_cast<size_t>(decoded.width) * decoded.height * decoded.channels;
    norm.data = std::make_unique<float[]>(pixel_count);
    norm.width = decoded.width;
    norm.height = decoded.height;
    norm.channels = decoded.channels;

    static constexpr float INV_255 = 1.0f / 255.0f;
    for (size_t i = 0; i < pixel_count; ++i) {
        // Cancellation checkpoint every 64K pixels
        if ((i & 0xFFFF) == 0 && token.isCancelled()) {
            ISHA_LOG_INFO("ImagePipeline", "Cancelled during normalization");
            return NormalizedImage{};
        }
        norm.data[i] = static_cast<float>(decoded.data[i]) * INV_255;
    }

    return norm;
}

PipelineResult ImagePipeline::process(const uint8_t* raw_data, size_t size,
                                       ImageSource source, const CancellationToken& token) {
    auto start = std::chrono::steady_clock::now();
    PipelineResult result;

    // Step 1: Validate input
    if (!raw_data || size == 0) {
        result.error = "Null or empty input data";
        ISHA_LOG_WARN("ImagePipeline", result.error);
        return result;
    }

    ImageContext ctx;
    ctx.compressed_size_bytes = size;
    ctx.source = source;
    // Placeholder dimensions — real codec would parse header
    ctx.width = 256;
    ctx.height = 256;
    ctx.channels = 3;
    ctx.pixel_format = PixelFormat::RGB;

    if (!isValidImageContext(ctx)) {
        result.error = "Invalid image context";
        ISHA_LOG_WARN("ImagePipeline", result.error);
        return result;
    }

    // Cancellation checkpoint after validation
    if (token.isCancelled()) {
        result.error = "Cancelled after validation";
        return result;
    }

    // Step 2: Decode
    DecodeResult decode_res = decoder_.decode(raw_data, size, token);
    if (!decode_res.success) {
        result.error = "Decode failed: " + decode_res.error_msg;
        ISHA_LOG_WARN("ImagePipeline", result.error);
        return result;
    }

    // Cancellation checkpoint after decode
    if (token.isCancelled()) {
        result.error = "Cancelled after decode";
        return result;
    }

    // Step 3: Normalize (uint8 -> float)
    NormalizedImage norm = normalize(decode_res.image, token);
    if (!norm.data) {
        result.error = "Normalization failed or cancelled";
        return result;
    }

    // Cancellation checkpoint after normalize
    if (token.isCancelled()) {
        result.error = "Cancelled after normalization";
        return result;
    }

    // Step 4: Resize if needed (placeholder — no-op for bounded edge)
    // In production, would resize to model input dimensions

    auto end = std::chrono::steady_clock::now();
    result.preprocessing_time_ms = std::chrono::duration<float, std::milli>(end - start).count();
    result.success = true;
    result.image = std::move(decode_res.image);
    result.normalized = std::move(norm);

    ISHA_LOG_INFO("ImagePipeline", "Pipeline complete in " + std::to_string(result.preprocessing_time_ms) + "ms");

    return result;
}

} // namespace isha
