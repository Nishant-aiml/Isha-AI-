#include "image_context.hpp"

namespace isha {

bool isValidImageContext(const ImageContext& ctx) {
    // Max dimension: 2048x2048
    if (ctx.width == 0 || ctx.width > 2048) return false;
    if (ctx.height == 0 || ctx.height > 2048) return false;

    // Max compressed size: 2MB
    static constexpr size_t MAX_COMPRESSED = 2 * 1024 * 1024;
    if (ctx.compressed_size_bytes == 0 || ctx.compressed_size_bytes > MAX_COMPRESSED) return false;

    // Valid channel counts: 1 (grayscale), 3 (RGB), 4 (RGBA)
    if (ctx.channels != 1 && ctx.channels != 3 && ctx.channels != 4) return false;

    return true;
}

} // namespace isha
