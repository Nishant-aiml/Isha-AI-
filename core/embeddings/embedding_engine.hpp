#ifndef ISHA_AI_EMBEDDING_ENGINE_HPP
#define ISHA_AI_EMBEDDING_ENGINE_HPP

#include "embedding_interface.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace isha {

// Real lightweight embedding engine using character 3-gram TF-IDF hashing.
// Produces normalized 384-dimension dense vectors in pure C++.
// No external dependencies (no ONNX, no PyTorch).
class EmbeddingEngine : public IEmbeddingGenerator {
public:
    explicit EmbeddingEngine(unsigned int dimensions = 384);
    ~EmbeddingEngine() override = default;

    std::vector<float> generateEmbedding(const std::string& text) override;
    unsigned int getDimensions() const override { return dimensions_; }

private:
    unsigned int dimensions_;
    
    // Extract character 3-grams from text
    std::vector<std::string> extractNgrams(const std::string& text, int n = 3) const;
    
    // Hash a string to a dimension index
    unsigned int hashToDimension(const std::string& ngram) const;
    
    // L2-normalize a vector in-place
    void normalize(std::vector<float>& vec) const;
};

} // namespace isha

#endif // ISHA_AI_EMBEDDING_ENGINE_HPP
