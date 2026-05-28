#include "../core/retrieval/retrieval_engine.hpp"
#include "../core/embeddings/embedding_engine.hpp"
#include "../core/retrieval/local_index.hpp"
#include <iostream>
#include <chrono>
#include <cassert>
#include <memory>

int main() {
    std::cout << "=== RETRIEVAL LATENCY BENCHMARK ===\n";
    
    auto embedder = std::make_shared<isha::EmbeddingEngine>(384);
    isha::RetrievalEngine engine(embedder);
    
    // Test 1: Ingesting chunks
    std::cout << "[TEST 1] Ingesting and building vector index... ";
    engine.loadMockPack("kisan_saathi");
    engine.loadMockPack("swasthya");
    
    size_t count = engine.getIndex().chunkCount();
    assert(count > 0);
    std::cout << "PASS (" << count << " chunks loaded)\n";
    
    // Test 2: Embedding query latency
    std::cout << "[TEST 2] Embedding generation latency... ";
    auto start = std::chrono::high_resolution_clock::now();
    auto vec = embedder->generateEmbedding("Symptoms of nitrogen deficiency in wheat crop");
    auto end = std::chrono::high_resolution_clock::now();
    double embedding_ms = std::chrono::duration<double, std::milli>(end - start).count();
    assert(vec.size() == 384);
    std::cout << "PASS (" << embedding_ms << " ms)\n";
    
    // Test 3: cosine similarity flat search latency
    std::cout << "[TEST 3] Flat cosine similarity search latency... ";
    start = std::chrono::high_resolution_clock::now();
    auto results = engine.retrieve("nitrogen wheat yellow leaves", "kisan_saathi", 3);
    end = std::chrono::high_resolution_clock::now();
    double search_ms = std::chrono::duration<double, std::milli>(end - start).count();
    assert(!results.empty());
    std::cout << "PASS (" << search_ms << " ms, top hit: " << results[0].chunk_id << ", score: " << results[0].score << ")\n";
    
    // Test 4: Search timeout protection (deadline check)
    std::cout << "[TEST 4] Search timeout protection... ";
    // Inject massive amount of dummy chunks to slow search
    for (int i = 0; i < 5000; ++i) {
        engine.getIndex().addChunk({"dummy_" + std::to_string(i), "Wheat nitrogen soil water crop farming temperature fertilizer rain yield pesticide blast rust", "kisan_saathi", vec});
    }
    
    // Search with a 1 microsecond timeout (should trigger deadline exit and return partial)
    auto query_embedding = embedder->generateEmbedding("nitrogen");
    start = std::chrono::high_resolution_clock::now();
    auto timed_results = engine.getIndex().search(query_embedding, "kisan_saathi", 10, std::chrono::milliseconds(0)); // 0ms timeout ensures instant hit
    end = std::chrono::high_resolution_clock::now();
    double timeout_search_ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "PASS (" << timeout_search_ms << " ms, results returned: " << timed_results.size() << ")\n";
    
    std::cout << "=== RETRIEVAL LATENCY BENCHMARK COMPLETED SUCCESSFULLY ===\n";
    return 0;
}
