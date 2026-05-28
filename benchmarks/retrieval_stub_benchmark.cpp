#include "retrieval/retrieval_interface.hpp"
#include "logging/logger.hpp"
#include <iostream>
#include <cassert>
#include <chrono>

int main() {
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::DEBUG);
    ISHA_LOG_INFO("Benchmark", "=== STARTING RETRIEVAL STUB BENCHMARK ===");

    isha::RetrievalStub engine;

    // Check index load check
    assert(engine.isIndexLoaded("bharat_core"));
    assert(!engine.isIndexLoaded("invalid_pack"));

    // Profile query retrieval
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<isha::DocumentChunk> results = engine.retrieve("wheat yellow leaves", "kisan_saathi", 2);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double latency = std::chrono::duration<double, std::micro>(end_time - start_time).count();

    assert(results.size() == 2);
    assert(results[0].chunk_id == "chunk_01");
    assert(results[0].score > 0.9);
    assert(results[0].pack_id == "kisan_saathi");

    std::cout << "Mock RAG retrieval latency: " << latency << " microseconds" << std::endl;

    ISHA_LOG_INFO("Benchmark", "=== RETRIEVAL STUB BENCHMARK COMPLETED SUCCESSFULLY ===");
    return 0;
}
