#include "image_decoder.hpp"
#include "../logging/logger.hpp"
#include <chrono>
#include <cstring>

namespace isha {

DecodeResult ImageDecoder::decode(const uint8_t* data, size_t size, const CancellationToken& token) {
    auto start = std::chrono::steady_clock::now();
    DecodeResult result;

    // Pre-validate: reject null input
    if (!data || size == 0) {
        reject_count_.fetch_add(1, std::memory_order_relaxed);
        result.error_msg = "Null or empty input data";
        ISHA_LOG_WARN("ImageDecoder", result.error_msg);
        return result;
    }

    // Pre-validate: reject oversized compressed data
    if (size > MAX_COMPRESSED_SIZE) {
        reject_count_.fetch_add(1, std::memory_order_relaxed);
        result.error_msg = "Compressed size " + std::to_string(size) + " exceeds limit " + std::to_string(MAX_COMPRESSED_SIZE);
        ISHA_LOG_WARN("ImageDecoder", result.error_msg);
        return result;
    }

    // Cancellation checkpoint 1
    if (token.isCancelled()) {
        result.error_msg = "Cancelled before decode";
        return result;
    }

    // Simulate header parse — extract dimensions from first 12 bytes as placeholder
    // In production this would call into a real codec (stb_image, libpng, etc.)
    // For bounded-edge implementation: assume 3-channel RGB, derive dimensions from size
    uint32_t width = 256;
    uint32_t height = 256;
    uint32_t channels = 3;

    // Validate decoded dimensions
    if (width > MAX_DIMENSION || height > MAX_DIMENSION) {
        reject_count_.fetch_add(1, std::memory_order_relaxed);
        result.error_msg = "Dimensions " + std::to_string(width) + "x" + std::to_string(height) + " exceed max " + std::to_string(MAX_DIMENSION);
        ISHA_LOG_WARN("ImageDecoder", result.error_msg);
        return result;
    }

    uint64_t pixel_count = static_cast<uint64_t>(width) * height;
    if (pixel_count > MAX_PIXEL_COUNT) {
        reject_count_.fetch_add(1, std::memory_order_relaxed);
        result.error_msg = "Pixel count exceeds maximum";
        ISHA_LOG_WARN("ImageDecoder", result.error_msg);
        return result;
    }

    size_t decode_size = static_cast<size_t>(pixel_count) * channels;
    if (decode_size > MAX_DECODE_HEAP) {
        reject_count_.fetch_add(1, std::memory_order_relaxed);
        result.error_msg = "Decoded size " + std::to_string(decode_size) + " exceeds heap limit " + std::to_string(MAX_DECODE_HEAP);
        ISHA_LOG_WARN("ImageDecoder", result.error_msg);
        return result;
    }

    // Cancellation checkpoint 2
    if (token.isCancelled()) {
        result.error_msg = "Cancelled during validation";
        return result;
    }

    // Allocate decoded buffer
    auto buffer = std::make_unique<uint8_t[]>(decode_size);

    // Simulate decode: fill with test pattern (checkerboard)
    for (uint32_t y = 0; y < height; ++y) {
        if (y % 64 == 0 && token.isCancelled()) {
            result.error_msg = "Cancelled mid-decode at row " + std::to_string(y);
            return result;
        }
        for (uint32_t x = 0; x < width; ++x) {
            size_t offset = (static_cast<size_t>(y) * width + x) * channels;
            uint8_t val = ((x / 32) + (y / 32)) % 2 == 0 ? 200 : 55;
            for (uint32_t c = 0; c < channels; ++c) {
                buffer[offset + c] = val;
            }
        }
    }

    // Build result
    result.success = true;
    result.image.data = std::move(buffer);
    result.image.width = width;
    result.image.height = height;
    result.image.channels = channels;
    result.image.size_bytes = decode_size;

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    decode_count_.fetch_add(1, std::memory_order_relaxed);
    total_decode_time_ms_.fetch_add(static_cast<uint64_t>(elapsed_ms), std::memory_order_relaxed);

    ISHA_LOG_INFO("ImageDecoder", "Decoded " + std::to_string(width) + "x" + std::to_string(height)
                  + "x" + std::to_string(channels) + " in " + std::to_string(elapsed_ms) + "ms");

    return result;
}

} // namespace isha
