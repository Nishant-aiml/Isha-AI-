#ifndef ISHA_AI_RETRIEVAL_INTERFACE_HPP
#define ISHA_AI_RETRIEVAL_INTERFACE_HPP

#include <string>
#include <vector>

namespace isha {

struct DocumentChunk {
    std::string chunk_id;
    std::string text;
    std::string pack_id;
    double score;
};

class IRetrievalEngine {
public:
    virtual ~IRetrievalEngine() = default;

    virtual std::vector<DocumentChunk> retrieve(const std::string& query, const std::string& pack_id, unsigned int limit) = 0;
    virtual bool isIndexLoaded(const std::string& pack_id) const = 0;
};

class RetrievalStub : public IRetrievalEngine {
public:
    RetrievalStub();
    ~RetrievalStub() override = default;

    std::vector<DocumentChunk> retrieve(const std::string& query, const std::string& pack_id, unsigned int limit) override;
    bool isIndexLoaded(const std::string& pack_id) const override;
};

} // namespace isha

#endif // ISHA_AI_RETRIEVAL_INTERFACE_HPP
