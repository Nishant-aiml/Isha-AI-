#ifndef ISHA_AI_IMAGE_DECODER_HPP
#define ISHA_AI_IMAGE_DECODER_HPP

#include <atomic>
#include <cstdint>
#include <string>
#include "image_context.hpp"
#include "../inference/cancellation_token.hpp"

namespace isha {

struct DecodeResult {
    bool success = false;
    DecodedImage image;
    std::string error_msg;

    DecodeResult() = default;
    DecodeResult(DecodeResult&&) = default;
    DecodeResult& operator=(DecodeResult&&) = default;
};

class ImageDecoder {
public:
    static constexpr size_t MAX_COMPRESSED_SIZE = 2 * 1024 * 1024;       // 2MB
    static constexpr uint32_t MAX_DIMENSION     = 2048;
    static constexpr uint64_t MAX_PIXEL_COUNT   = 2048ULL * 2048ULL;
    static constexpr size_t MAX_DECODE_HEAP     = 4 * 1024 * 1024;       // 4MB

    ImageDecoder() = default;
    ~ImageDecoder() = default;

    ImageDecoder(const ImageDecoder&) = delete;
    ImageDecoder& operator=(const ImageDecoder&) = delete;

    DecodeResult decode(const uint8_t* data, size_t size, const CancellationToken& token);

    // Telemetry accessors
    uint64_t getDecodeCount() const { return decode_count_.load(std::memory_order_relaxed); }
    uint64_t getRejectCount() const { return reject_count_.load(std::memory_order_relaxed); }
    uint64_t getTotalDecodeTimeMs() const { return total_decode_time_ms_.load(std::memory_order_relaxed); }

private:
    std::atomic<uint64_t> decode_count_{0};
    std::atomic<uint64_t> reject_count_{0};
    std::atomic<uint64_t> total_decode_time_ms_{0};
};

} // namespace isha

#endif // ISHA_AI_IMAGE_DECODER_HPP
