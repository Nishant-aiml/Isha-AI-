#include "embedding_interface.hpp"
#include "../logging/logger.hpp"
#include <cmath>

namespace isha {

EmbeddingStub::EmbeddingStub(unsigned int dimensions) : dimensions_(dimensions) {
    ISHA_LOG_INFO("Embedding", "Initialized mock embedding generator with " + std::to_string(dimensions) + " dimensions.");
}

std::vector<float> EmbeddingStub::generateEmbedding(const std::string& text) {
    ISHA_LOG_DEBUG("Embedding", "Generating mock embedding for text: '" + text + "'");
    
    std::vector<float> vector(dimensions_, 0.0f);
    
    // Generate deterministic mock vector based on text character weights
    float sum = 0.0f;
    for (char c : text) {
        sum += static_cast<float>(c);
    }

    for (unsigned int i = 0; i < dimensions_; ++i) {
        vector[i] = std::sin(sum + i) * 0.1f;
    }

    return vector;
}

} // namespace isha
