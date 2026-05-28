#include "retrieval_interface.hpp"
#include "../logging/logger.hpp"

namespace isha {

RetrievalStub::RetrievalStub() {
    ISHA_LOG_INFO("Retrieval", "Initialized mock retrieval engine stub.");
}

std::vector<DocumentChunk> RetrievalStub::retrieve(const std::string& query, const std::string& pack_id, unsigned int limit) {
    ISHA_LOG_INFO("Retrieval", "Querying mock retrieval: '" + query + "' inside pack: '" + pack_id + "'");

    std::vector<DocumentChunk> mock_chunks = {
        { "chunk_01", "Mock chunk text grounder A for " + query, pack_id, 0.95 },
        { "chunk_02", "Mock chunk text grounder B containing facts.", pack_id, 0.84 },
        { "chunk_03", "Mock chunk text grounder C covering guidelines.", pack_id, 0.72 }
    };

    if (mock_chunks.size() > limit) {
        mock_chunks.resize(limit);
    }
    return mock_chunks;
}

bool RetrievalStub::isIndexLoaded(const std::string& pack_id) const {
    return pack_id == "bharat_core" || pack_id == "kisan_saathi" || pack_id == "swasthya";
}

} // namespace isha
