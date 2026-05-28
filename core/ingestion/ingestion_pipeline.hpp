#ifndef ISHA_AI_INGESTION_PIPELINE_HPP
#define ISHA_AI_INGESTION_PIPELINE_HPP

#include <string>
#include <vector>
#include <memory>
#include "chunker.hpp"

namespace isha {

struct ProcessedDocument {
    std::string document_id;
    std::vector<IngestionChunk> chunks;
    std::string metadata_tag;
};

class IngestionPipeline {
public:
    IngestionPipeline(std::shared_ptr<IChunker> chunker);
    ~IngestionPipeline() = default;

    ProcessedDocument ingest(const std::string& document_id, const std::string& raw_text, const std::string& tag);

private:
    std::shared_ptr<IChunker> chunker_;
};

} // namespace isha

#endif // ISHA_AI_INGESTION_PIPELINE_HPP
