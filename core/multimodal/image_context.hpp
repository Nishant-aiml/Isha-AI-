#ifndef ISHA_AI_IMAGE_CONTEXT_HPP
#define ISHA_AI_IMAGE_CONTEXT_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <chrono>

namespace isha {

enum class PixelFormat {
    RGB,
    RGBA,
    GRAYSCALE
};

enum class ImageSource {
    CAMERA,
    FILE,
    CLIPBOARD,
    SCREEN_CAPTURE
};

struct ImageContext {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;
    std::string source_id;
    uint64_t timestamp = 0;    // Epoch milliseconds
    size_t compressed_size_bytes = 0;
    PixelFormat pixel_format = PixelFormat::RGB;
    ImageSource source = ImageSource::FILE;
};

struct DecodedImage {
    std::unique_ptr<uint8_t[]> data;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;
    size_t size_bytes = 0;

    DecodedImage() = default;
    DecodedImage(DecodedImage&&) = default;
    DecodedImage& operator=(DecodedImage&&) = default;
    DecodedImage(const DecodedImage&) = delete;
    DecodedImage& operator=(const DecodedImage&) = delete;
};

// Validate image context against bounded constraints
bool isValidImageContext(const ImageContext& ctx);

} // namespace isha

#endif // ISHA_AI_IMAGE_CONTEXT_HPP
