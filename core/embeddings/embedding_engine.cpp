#include "embedding_engine.hpp"
#include "../logging/logger.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace isha {

EmbeddingEngine::EmbeddingEngine(unsigned int dimensions) : dimensions_(dimensions) {
    ISHA_LOG_INFO("EmbeddingEngine", "Initialized char 3-gram TF-IDF engine (" + std::to_string(dimensions) + " dims)");
}

std::vector<std::string> EmbeddingEngine::extractNgrams(const std::string& text, int n) const {
    std::vector<std::string> ngrams;
    if (static_cast<int>(text.size()) < n) {
        ngrams.push_back(text);
        return ngrams;
    }
    ngrams.reserve(text.size() - n + 1);
    for (size_t i = 0; i <= text.size() - n; ++i) {
        ngrams.push_back(text.substr(i, n));
    }
    return ngrams;
}

unsigned int EmbeddingEngine::hashToDimension(const std::string& ngram) const {
    // FNV-1a hash mapped to dimension space
    unsigned int hash = 2166136261u;
    for (char c : ngram) {
        hash ^= static_cast<unsigned int>(c);
        hash *= 16777619u;
    }
    return hash % dimensions_;
}

void EmbeddingEngine::normalize(std::vector<float>& vec) const {
    float norm = 0.0f;
    for (float v : vec) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 1e-8f) {
        for (float& v : vec) {
            v /= norm;
        }
    }
}

std::vector<float> EmbeddingEngine::generateEmbedding(const std::string& text) {
    std::vector<float> vec(dimensions_, 0.0f);
    
    if (text.empty()) {
        return vec;
    }
    
    // Convert to lowercase for normalization
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // Extract 3-grams and accumulate TF weights
    auto ngrams = extractNgrams(lower_text, 3);
    for (const auto& ng : ngrams) {
        unsigned int dim = hashToDimension(ng);
        vec[dim] += 1.0f; // Term frequency
    }
    
    // Apply log-TF scaling: log(1 + tf)
    for (float& v : vec) {
        if (v > 0.0f) {
            v = std::log(1.0f + v);
        }
    }
    
    // L2 normalize
    normalize(vec);
    
    ISHA_LOG_DEBUG("EmbeddingEngine", "Generated embedding for text (" + std::to_string(text.size()) + " chars)");
    return vec;
}

} // namespace isha
