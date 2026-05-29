#include "retrieval_engine.hpp"
#include "../logging/logger.hpp"
#include "../runtime/event_bus.hpp"
#include <chrono>

namespace isha {

RetrievalEngine::RetrievalEngine(std::shared_ptr<IEmbeddingGenerator> embedder)
    : embedder_(std::move(embedder)) {
    ISHA_LOG_INFO("RetrievalEngine", "Initialized with real embedding engine.");
}

void RetrievalEngine::ingestChunk(const std::string& chunk_id, const std::string& text, const std::string& pack_id) {
    auto embedding = embedder_->generateEmbedding(text);
    index_.addChunk({chunk_id, text, pack_id, std::move(embedding)});
}

void RetrievalEngine::loadMockPack(const std::string& pack_id) {
    if (loaded_packs_.count(pack_id)) {
        ISHA_LOG_WARN("RetrievalEngine", "Pack already loaded: " + pack_id);
        return;
    }
    
    ISHA_LOG_INFO("RetrievalEngine", "Loading mock grounded pack: " + pack_id);
    
    if (pack_id == "kisan_saathi") {
        ingestChunk("kisan_001", "Nitrogen deficiency in wheat causes yellowing of lower leaves, stunted growth, and reduced tillering. Apply 60kg/ha urea at basal dose.", pack_id);
        ingestChunk("kisan_002", "Phosphorus deficiency shows purplish discoloration on leaves. Apply DAP at sowing time for correction.", pack_id);
        ingestChunk("kisan_003", "Potassium deficiency causes scorching of leaf tips and margins. Apply MOP before flowering stage.", pack_id);
        ingestChunk("kisan_004", "Yellow mosaic virus in moong causes bright yellow patches. Use resistant varieties and control whitefly vectors.", pack_id);
        ingestChunk("kisan_005", "Wheat rust appears as orange-brown pustules on leaves and stems. Spray propiconazole fungicide immediately.", pack_id);
        ingestChunk("kisan_006", "Rice blast disease causes diamond-shaped lesions. Use tricyclazole fungicide preventively.", pack_id);
        ingestChunk("kisan_007", "Soil pH between 6.0-7.5 is optimal for most crops. Test soil before each season.", pack_id);
        ingestChunk("kisan_008", "Drip irrigation saves 40-60% water compared to flood irrigation and improves yield by 20-30%.", pack_id);
    } else if (pack_id == "swasthya") {
        ingestChunk("swasthya_001", "Paracetamol dosage for children: 10-15mg/kg every 4-6 hours. Maximum 4 doses in 24 hours. Do not exceed recommended dose.", pack_id);
        ingestChunk("swasthya_002", "Fever above 102F in children requires medical attention. Give ORS to prevent dehydration. Monitor for 24 hours.", pack_id);
        ingestChunk("swasthya_003", "Common cold symptoms include runny nose, sore throat, mild fever. Rest, warm fluids, and steam inhalation help recovery.", pack_id);
        ingestChunk("swasthya_004", "Diarrhea treatment: ORS solution (1 packet in 1 litre boiled cooled water). Feed zinc tablets for 14 days.", pack_id);
        ingestChunk("swasthya_005", "Dengue warning signs: persistent vomiting, abdominal pain, bleeding gums. Seek hospital immediately.", pack_id);
        ingestChunk("swasthya_006", "Diabetes management: monitor blood sugar regularly, take prescribed medication, exercise 30 min daily.", pack_id);
        ingestChunk("swasthya_007", "Hypertension: reduce salt intake below 5g/day, exercise regularly, take medication as prescribed.", pack_id);
        ingestChunk("swasthya_008", "First aid for burns: cool running water for 20 minutes, cover with clean cloth. Do not apply ice or butter.", pack_id);
    } else if (pack_id == "bharat_core") {
        ingestChunk("bharat_001", "India has 28 states and 8 union territories. New Delhi is the capital. Hindi and English are official languages.", pack_id);
        ingestChunk("bharat_002", "Indian Constitution was adopted on 26 January 1950. It guarantees fundamental rights to all citizens.", pack_id);
        ingestChunk("bharat_003", "Emergency helpline numbers: Police 100, Ambulance 108, Fire 101, Women helpline 1091.", pack_id);
        ingestChunk("bharat_004", "Aadhaar is a 12-digit unique identity number. Required for government subsidies and bank accounts.", pack_id);
    }
    
    loaded_packs_.insert(pack_id);
    ISHA_LOG_INFO("RetrievalEngine", "Loaded " + std::to_string(index_.chunkCountForPack(pack_id)) + " chunks for pack '" + pack_id + "'");
}

std::vector<DocumentChunk> RetrievalEngine::retrieve(const std::string& query, const std::string& pack_id, unsigned int limit) {
    ISHA_LOG_INFO("RetrievalEngine", "Retrieving for query: '" + query + "' in pack '" + pack_id + "'");
    
    auto query_embedding = embedder_->generateEmbedding(query);
    // Cosine similarity search with strict threshold of 0.35
    auto results = index_.search(query_embedding, pack_id, limit, 0.35);
    
    // If no semantic match above threshold is found, fallback to BM25-lite keyword search
    if (results.empty()) {
        ISHA_LOG_INFO("RetrievalEngine", "Semantic search returned no results above threshold. Falling back to BM25-lite.");
        results = index_.searchBM25(query, pack_id, limit);
    }
    
    std::vector<DocumentChunk> chunks;
    chunks.reserve(results.size());
    for (const auto& r : results) {
        chunks.push_back({r.chunk_id, r.text, r.pack_id, r.score});
    }
    
    // Publish retrieval complete event
    EventBus::getInstance().publish({EventType::RETRIEVAL_COMPLETE, "RetrievalEngine",
        "results=" + std::to_string(chunks.size()) + ",pack=" + pack_id});
    
    return chunks;
}

bool RetrievalEngine::isIndexLoaded(const std::string& pack_id) const {
    return loaded_packs_.count(pack_id) > 0;
}

} // namespace isha
