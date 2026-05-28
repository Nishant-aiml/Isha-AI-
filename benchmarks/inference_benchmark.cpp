#include "inference/gguf_inference_engine.hpp"
#include "logging/logger.hpp"
#include <iostream>
#include <fstream>
#include <cassert>

static void createDummyGguf(const std::string& path, size_t size_bytes, bool valid_magic = true) {
    std::ofstream file(path, std::ios::binary);
    assert(file.is_open());

    char magic[4] = {'G', 'G', 'U', 'F'};
    if (!valid_magic) {
        magic[0] = 'X';
    }
    uint32_t version = 3;
    uint64_t tensor_count = 8;
    uint64_t metadata_kv_count = 2;

    file.write(magic, sizeof(magic));
    file.write(reinterpret_cast<char*>(&version), sizeof(version));
    file.write(reinterpret_cast<char*>(&tensor_count), sizeof(tensor_count));
    file.write(reinterpret_cast<char*>(&metadata_kv_count), sizeof(metadata_kv_count));

    size_t written = 24;
    std::vector<char> padding(1024, 0);
    while (written < size_bytes) {
        size_t to_write = std::min(padding.size(), size_bytes - written);
        file.write(padding.data(), to_write);
        written += to_write;
    }
}

int main() {
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::DEBUG);
    ISHA_LOG_INFO("Benchmark", "=== STARTING INFERENCE BENCHMARK ===");

    std::string valid_path = "valid_model.gguf";
    std::string corrupt_path = "corrupt_model.gguf";

    createDummyGguf(valid_path, 2 * 1024 * 1024); // 2MB model
    createDummyGguf(corrupt_path, 1 * 1024 * 1024, false); // invalid magic

    auto cancel_token = std::make_shared<isha::CancellationToken>();
    isha::InferenceContext context("session_123", cancel_token, 5); // 5MB RAM budget limit

    isha::GGUFInferenceEngine engine;
    assert(!engine.isLoaded());

    // 1. Test valid load
    bool load_ok = engine.load(valid_path, context);
    assert(load_ok);
    assert(engine.isLoaded());
    assert(engine.memoryUsageMB() == 2);

    // Test generation output
    std::string out = engine.generate("test prompt", context);
    assert(out == "Mock GGUF response to prompt: 'test prompt'");

    // Test unload
    engine.unload();
    assert(!engine.isLoaded());
    assert(engine.memoryUsageMB() == 0);

    // 2. Test budget limit violation (load 2MB model with 1MB budget limit)
    isha::InferenceContext tight_context("session_123", cancel_token, 1);
    bool over_budget_ok = engine.load(valid_path, tight_context);
    assert(!over_budget_ok);

    // 3. Test corrupted GGUF magic bytes
    bool corrupt_ok = engine.load(corrupt_path, context);
    assert(!corrupt_ok);

    // Cleanup
    std::remove(valid_path.c_str());
    std::remove(corrupt_path.c_str());

    ISHA_LOG_INFO("Benchmark", "=== INFERENCE BENCHMARK COMPLETED SUCCESSFULLY ===");
    return 0;
}
