#ifndef ISHA_AI_CHUNKER_HPP
#define ISHA_AI_CHUNKER_HPP

#include <string>
#include <vector>

namespace isha {

enum class ChunkingStrategy {
    CHARACTER,
    WORD,
    TOKEN_AWARE,
    SEMANTIC
};

struct IngestionChunk {
    std::string text;
    size_t char_offset;
    size_t index;
};

class IChunker {
public:
    virtual ~IChunker() = default;

    virtual std::vector<IngestionChunk> chunkText(const std::string& text, size_t chunk_size, size_t overlap) = 0;
    virtual ChunkingStrategy getStrategy() const = 0;
};

class BasicWordChunker : public IChunker {
public:
    BasicWordChunker() = default;
    ~BasicWordChunker() override = default;

    std::vector<IngestionChunk> chunkText(const std::string& text, size_t chunk_size, size_t overlap) override;
    ChunkingStrategy getStrategy() const override { return ChunkingStrategy::WORD; }
};

} // namespace isha

#endif // ISHA_AI_CHUNKER_HPP
