#ifndef ISHA_AI_EDGE_FLAT_VECTOR_INDEX_HPP
#define ISHA_AI_EDGE_FLAT_VECTOR_INDEX_HPP

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <unordered_map>

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

class EdgeFlatVectorIndex {
public:
    EdgeFlatVectorIndex();
    ~EdgeFlatVectorIndex() = default;

    // Add a chunk with its pre-computed embedding
    void addChunk(const IndexedChunk& chunk);
    
    // Clear all chunks for a given pack
    void clearPack(const std::string& pack_id);
    
    // Search with cosine similarity, score thresholds, and timeout protection
    std::vector<SearchResult> search(
        const std::vector<float>& query_embedding,
        const std::string& pack_id,
        unsigned int limit,
        double score_threshold = 0.35,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(20)
    ) const;
    
    // BM25-lite search fallback under Zone 4 / critical thermal situations
    std::vector<SearchResult> searchBM25(
        const std::string& query,
        const std::string& pack_id,
        unsigned int limit
    ) const;

    // Binary serialization / deserialization to file
    bool saveToFile(const std::string& filepath) const;
    bool loadFromFile(const std::string& filepath);
    
    size_t chunkCount() const;
    size_t chunkCountForPack(const std::string& pack_id) const;

private:
    double cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) const;
    
    // Helper to tokenize string for BM25-lite
    std::vector<std::string> tokenize(const std::string& text) const;

    std::vector<IndexedChunk> chunks_;
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_EDGE_FLAT_VECTOR_INDEX_HPP
