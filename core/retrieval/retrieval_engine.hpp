#ifndef ISHA_AI_RETRIEVAL_ENGINE_HPP
#define ISHA_AI_RETRIEVAL_ENGINE_HPP

#include "retrieval_interface.hpp"
#include "local_index.hpp"
#include "../embeddings/embedding_interface.hpp"
#include <memory>
#include <set>

namespace isha {

class RetrievalEngine : public IRetrievalEngine {
public:
    RetrievalEngine(std::shared_ptr<IEmbeddingGenerator> embedder);
    ~RetrievalEngine() override = default;

    std::vector<DocumentChunk> retrieve(const std::string& query, const std::string& pack_id, unsigned int limit) override;
    bool isIndexLoaded(const std::string& pack_id) const override;
    
    // Load mock grounded content for a pack
    void loadMockPack(const std::string& pack_id);
    
    // Direct access to index for benchmarking
    LocalIndex& getIndex() { return index_; }
    const LocalIndex& getIndex() const { return index_; }

private:
    std::shared_ptr<IEmbeddingGenerator> embedder_;
    LocalIndex index_;
    std::set<std::string> loaded_packs_;
    
    void ingestChunk(const std::string& chunk_id, const std::string& text, const std::string& pack_id);
};

} // namespace isha

#endif // ISHA_AI_RETRIEVAL_ENGINE_HPP
