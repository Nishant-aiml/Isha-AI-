#ifndef ISHA_AI_LOCAL_INDEX_HPP
#define ISHA_AI_LOCAL_INDEX_HPP

#include <string>
#include <vector>
#include <mutex>
#include <chrono>

namespace isha {

struct IndexedChunk {
    std::string chunk_id;
    std::string text;
    std::string pack_id;
    std::vector<float> embedding;
};

struct SearchResult {
    std::string chunk_id;
    std::string text;
    std::string pack_id;
    double score;
};

class LocalIndex {
public:
    LocalIndex();
    ~LocalIndex() = default;

    // Add a chunk with its pre-computed embedding
    void addChunk(const IndexedChunk& chunk);
    
    // Clear all chunks for a given pack
    void clearPack(const std::string& pack_id);
    
    // Search by cosine similarity with timeout protection
    std::vector<SearchResult> search(
        const std::vector<float>& query_embedding,
        const std::string& pack_id,
        unsigned int limit,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(20)
    ) const;
    
    size_t chunkCount() const;
    size_t chunkCountForPack(const std::string& pack_id) const;

private:
    double cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) const;
    
    std::vector<IndexedChunk> chunks_;
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_LOCAL_INDEX_HPP
