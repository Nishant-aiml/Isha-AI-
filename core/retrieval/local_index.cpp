#include "local_index.hpp"
#include "../logging/logger.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <fstream>
#include <sstream>
#include <set>

namespace isha {

EdgeFlatVectorIndex::EdgeFlatVectorIndex() {
    ISHA_LOG_INFO("EdgeFlatVectorIndex", "Initialized local edge vector index.");
}

void EdgeFlatVectorIndex::addChunk(const IndexedChunk& chunk) {
    std::lock_guard<std::mutex> lock(mutex_);
    chunks_.push_back(chunk);
    ISHA_LOG_DEBUG("EdgeFlatVectorIndex", "Added chunk '" + chunk.chunk_id + "' to pack '" + chunk.pack_id + "'");
}

void EdgeFlatVectorIndex::clearPack(const std::string& pack_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(chunks_.begin(), chunks_.end(),
        [&pack_id](const IndexedChunk& c) { return c.pack_id == pack_id; });
    size_t removed = std::distance(it, chunks_.end());
    chunks_.erase(it, chunks_.end());
    ISHA_LOG_INFO("EdgeFlatVectorIndex", "Cleared " + std::to_string(removed) + " chunks from pack '" + pack_id + "'");
}

double EdgeFlatVectorIndex::cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) const {
    if (a.size() != b.size() || a.empty()) return 0.0;
    double dot = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
    }
    return dot;
}

std::vector<SearchResult> EdgeFlatVectorIndex::search(
    const std::vector<float>& query_embedding,
    const std::string& pack_id,
    unsigned int limit,
    double score_threshold,
    std::chrono::milliseconds timeout
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto start = std::chrono::steady_clock::now();
    
    std::vector<SearchResult> results;
    
    for (const auto& chunk : chunks_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        if (elapsed >= timeout) {
            ISHA_LOG_WARN("EdgeFlatVectorIndex", "Search timeout reached (" + std::to_string(elapsed.count()) + "ms). Returning partial results.");
            break;
        }
        
        if (!pack_id.empty() && chunk.pack_id != pack_id) {
            continue;
        }
        
        double score = cosineSimilarity(query_embedding, chunk.embedding);
        if (score >= score_threshold) {
            results.push_back({chunk.chunk_id, chunk.text, chunk.pack_id, score});
        }
    }
    
    std::sort(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b) { return a.score > b.score; });
    
    if (results.size() > limit) {
        results.resize(limit);
    }
    
    auto total_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    ISHA_LOG_DEBUG("EdgeFlatVectorIndex", "Search completed: " + std::to_string(results.size()) + " results in " + std::to_string(total_elapsed.count()) + "us");
    
    return results;
}

std::vector<std::string> EdgeFlatVectorIndex::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(text);
    while (ss >> token) {
        std::transform(token.begin(), token.end(), token.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        token.erase(std::remove_if(token.begin(), token.end(), 
            [](unsigned char c) { return std::ispunct(c); }), token.end());
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

std::vector<SearchResult> EdgeFlatVectorIndex::searchBM25(
    const std::string& query,
    const std::string& pack_id,
    unsigned int limit
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<SearchResult> results;
    auto query_tokens = tokenize(query);
    if (query_tokens.empty()) return results;
    
    std::vector<std::vector<std::string>> doc_tokens;
    std::vector<size_t> doc_lens;
    std::vector<size_t> matching_chunk_indices;
    double total_len = 0.0;
    
    // Gather statistics for active documents in target pack
    for (size_t i = 0; i < chunks_.size(); ++i) {
        if (!pack_id.empty() && chunks_[i].pack_id != pack_id) {
            continue;
        }
        auto tokens = tokenize(chunks_[i].text);
        doc_lens.push_back(tokens.size());
        total_len += tokens.size();
        doc_tokens.push_back(std::move(tokens));
        matching_chunk_indices.push_back(i);
    }
    
    size_t num_docs = doc_tokens.size();
    if (num_docs == 0) return results;
    
    double avgdl = total_len / num_docs;
    double k1 = 1.2;
    double b = 0.75;
    
    // Calculate document frequency (DF) for query tokens
    std::unordered_map<std::string, size_t> df;
    for (const auto& q_token : query_tokens) {
        size_t count = 0;
        for (const auto& doc : doc_tokens) {
            if (std::find(doc.begin(), doc.end(), q_token) != doc.end()) {
                count++;
            }
        }
        df[q_token] = count;
    }
    
    // Score documents
    for (size_t i = 0; i < num_docs; ++i) {
        double score = 0.0;
        const auto& doc = doc_tokens[i];
        double dl = static_cast<double>(doc_lens[i]);
        
        for (const auto& q_token : query_tokens) {
            size_t doc_freq = df[q_token];
            double idf = std::log((num_docs - doc_freq + 0.5) / (doc_freq + 0.5) + 1.0);
            
            double tf = std::count(doc.begin(), doc.end(), q_token);
            if (tf > 0.0) {
                double numerator = tf * (k1 + 1.0);
                double denominator = tf + k1 * (1.0 - b + b * (dl / avgdl));
                score += idf * (numerator / denominator);
            }
        }
        
        if (score > 0.0) {
            size_t original_index = matching_chunk_indices[i];
            // Normalize score to a 0.0 - 1.0 similarity proxy
            double normalized_score = 1.0 - (1.0 / (1.0 + score));
            results.push_back({
                chunks_[original_index].chunk_id,
                chunks_[original_index].text,
                chunks_[original_index].pack_id,
                normalized_score
            });
        }
    }
    
    std::sort(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b) { return a.score > b.score; });
        
    if (results.size() > limit) {
        results.resize(limit);
    }
    return results;
}

bool EdgeFlatVectorIndex::saveToFile(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream out(filepath, std::ios::binary);
    if (!out) {
        ISHA_LOG_ERROR("EdgeFlatVectorIndex", "Failed to open file for save: " + filepath);
        return false;
    }
    
    // File header signature: 'EFVI' + version (0x01)
    out.write("EFVI\x01", 5);
    
    uint64_t count = chunks_.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    for (const auto& chunk : chunks_) {
        // Write IDs
        uint32_t chunk_id_len = chunk.chunk_id.size();
        out.write(reinterpret_cast<const char*>(&chunk_id_len), sizeof(chunk_id_len));
        out.write(chunk.chunk_id.data(), chunk_id_len);
        
        uint32_t pack_id_len = chunk.pack_id.size();
        out.write(reinterpret_cast<const char*>(&pack_id_len), sizeof(pack_id_len));
        out.write(chunk.pack_id.data(), pack_id_len);
        
        // Write Text
        uint32_t text_len = chunk.text.size();
        out.write(reinterpret_cast<const char*>(&text_len), sizeof(text_len));
        out.write(chunk.text.data(), text_len);
        
        // Write Embedding
        uint32_t emb_size = chunk.embedding.size();
        out.write(reinterpret_cast<const char*>(&emb_size), sizeof(emb_size));
        out.write(reinterpret_cast<const char*>(chunk.embedding.data()), emb_size * sizeof(float));
    }
    
    ISHA_LOG_INFO("EdgeFlatVectorIndex", "Saved " + std::to_string(count) + " chunks to binary index: " + filepath);
    return true;
}

bool EdgeFlatVectorIndex::loadFromFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream in(filepath, std::ios::binary);
    if (!in) {
        ISHA_LOG_WARN("EdgeFlatVectorIndex", "Failed to open file for load: " + filepath);
        return false;
    }
    
    char header[5];
    in.read(header, 5);
    if (std::string(header, 5) != std::string("EFVI\x01", 5)) {
        ISHA_LOG_ERROR("EdgeFlatVectorIndex", "Invalid EFVI signature in file: " + filepath);
        return false;
    }
    
    uint64_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    chunks_.clear();
    chunks_.reserve(count);
    
    for (uint64_t i = 0; i < count; ++i) {
        IndexedChunk chunk;
        
        uint32_t chunk_id_len = 0;
        in.read(reinterpret_cast<char*>(&chunk_id_len), sizeof(chunk_id_len));
        chunk.chunk_id.resize(chunk_id_len);
        in.read(&chunk.chunk_id[0], chunk_id_len);
        
        uint32_t pack_id_len = 0;
        in.read(reinterpret_cast<char*>(&pack_id_len), sizeof(pack_id_len));
        chunk.pack_id.resize(pack_id_len);
        in.read(&chunk.pack_id[0], pack_id_len);
        
        uint32_t text_len = 0;
        in.read(reinterpret_cast<char*>(&text_len), sizeof(text_len));
        chunk.text.resize(text_len);
        in.read(&chunk.text[0], text_len);
        
        uint32_t emb_size = 0;
        in.read(reinterpret_cast<char*>(&emb_size), sizeof(emb_size));
        chunk.embedding.resize(emb_size);
        in.read(reinterpret_cast<char*>(chunk.embedding.data()), emb_size * sizeof(float));
        
        chunks_.push_back(std::move(chunk));
    }
    
    ISHA_LOG_INFO("EdgeFlatVectorIndex", "Loaded " + std::to_string(chunks_.size()) + " chunks from binary index: " + filepath);
    return true;
}

size_t EdgeFlatVectorIndex::chunkCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return chunks_.size();
}

size_t EdgeFlatVectorIndex::chunkCountForPack(const std::string& pack_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::count_if(chunks_.begin(), chunks_.end(),
        [&pack_id](const IndexedChunk& c) { return c.pack_id == pack_id; });
}

} // namespace isha
