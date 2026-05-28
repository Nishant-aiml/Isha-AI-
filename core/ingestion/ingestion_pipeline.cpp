#include "ingestion_pipeline.hpp"
#include "../logging/logger.hpp"

namespace isha {

IngestionPipeline::IngestionPipeline(std::shared_ptr<IChunker> chunker)
    : chunker_(chunker) {
    ISHA_LOG_INFO("Ingestion", "Initialized document ingestion pipeline.");
}

ProcessedDocument IngestionPipeline::ingest(const std::string& document_id, const std::string& raw_text, const std::string& tag) {
    ISHA_LOG_INFO("Ingestion", "Ingesting document: " + document_id + " (Tag: " + tag + ")");
    
    std::vector<IngestionChunk> chunks;
    if (chunker_) {
        // Defaults to 256 size and 32 word overlap
        chunks = chunker_->chunkText(raw_text, 256, 32);
    }

    ISHA_LOG_INFO("Ingestion", "Document " + document_id + " processed into " + std::to_string(chunks.size()) + " chunks.");
    return { document_id, chunks, tag };
}

} // namespace isha
