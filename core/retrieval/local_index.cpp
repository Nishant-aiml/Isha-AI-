#include "local_index.hpp"
#include "../logging/logger.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace isha {

LocalIndex::LocalIndex() {
    ISHA_LOG_INFO("LocalIndex", "Initialized local vector index.");
}

void LocalIndex::addChunk(const IndexedChunk& chunk) {
    std::lock_guard<std::mutex> lock(mutex_);
    chunks_.push_back(chunk);
    ISHA_LOG_DEBUG("LocalIndex", "Added chunk '" + chunk.chunk_id + "' to pack '" + chunk.pack_id + "'");
}

void LocalIndex::clearPack(const std::string& pack_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(chunks_.begin(), chunks_.end(),
        [&pack_id](const IndexedChunk& c) { return c.pack_id == pack_id; });
    size_t removed = std::distance(it, chunks_.end());
    chunks_.erase(it, chunks_.end());
    ISHA_LOG_INFO("LocalIndex", "Cleared " + std::to_string(removed) + " chunks from pack '" + pack_id + "'");
}

double LocalIndex::cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) const {
    if (a.size() != b.size() || a.empty()) return 0.0;
    double dot = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
    }
    // Vectors are already L2-normalized, so dot product = cosine similarity
    return dot;
}

std::vector<SearchResult> LocalIndex::search(
    const std::vector<float>& query_embedding,
    const std::string& pack_id,
    unsigned int limit,
    std::chrono::milliseconds timeout
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto start = std::chrono::steady_clock::now();
    
    std::vector<SearchResult> results;
    
    for (const auto& chunk : chunks_) {
        // Check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        if (elapsed >= timeout) {
            ISHA_LOG_WARN("LocalIndex", "Search timeout reached (" + std::to_string(elapsed.count()) + "ms). Returning partial results.");
            break;
        }
        
        // Filter by pack
        if (!pack_id.empty() && chunk.pack_id != pack_id) {
            continue;
        }
        
        double score = cosineSimilarity(query_embedding, chunk.embedding);
        results.push_back({chunk.chunk_id, chunk.text, chunk.pack_id, score});
    }
    
    // Sort by score descending
    std::sort(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b) { return a.score > b.score; });
    
    // Limit results
    if (results.size() > limit) {
        results.resize(limit);
    }
    
    auto total_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    ISHA_LOG_DEBUG("LocalIndex", "Search completed: " + std::to_string(results.size()) + " results in " + std::to_string(total_elapsed.count()) + "us");
    
    return results;
}

size_t LocalIndex::chunkCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return chunks_.size();
}

size_t LocalIndex::chunkCountForPack(const std::string& pack_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::count_if(chunks_.begin(), chunks_.end(),
        [&pack_id](const IndexedChunk& c) { return c.pack_id == pack_id; });
}

} // namespace isha
