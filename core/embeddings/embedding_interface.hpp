#ifndef ISHA_AI_EMBEDDING_INTERFACE_HPP
#define ISHA_AI_EMBEDDING_INTERFACE_HPP

#include <string>
#include <vector>

namespace isha {

class IEmbeddingGenerator {
public:
    virtual ~IEmbeddingGenerator() = default;

    virtual std::vector<float> generateEmbedding(const std::string& text) = 0;
    virtual unsigned int getDimensions() const = 0;
};

class EmbeddingStub : public IEmbeddingGenerator {
public:
    EmbeddingStub(unsigned int dimensions = 384);
    ~EmbeddingStub() override = default;

    std::vector<float> generateEmbedding(const std::string& text) override;
    unsigned int getDimensions() const override { return dimensions_; }

private:
    unsigned int dimensions_;
};

} // namespace isha

#endif // ISHA_AI_EMBEDDING_INTERFACE_HPP
