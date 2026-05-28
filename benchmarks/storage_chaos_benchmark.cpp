#include <iostream>
#include <string>
#include <fstream>
#include <cassert>
#include <memory>
#include "model/download_manager.hpp"
#include "logging/logger.hpp"

using namespace isha;

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) { \
        std::cout << "[FAIL] " << msg << " (line " << __LINE__ << ")\n"; \
        g_fail++; \
    } else { \
        std::cout << "[PASS] " << msg << "\n"; \
        g_pass++; \
    }

void testStorageChaos() {
    std::cout << "\n=== Test: Storage Chaos & Headroom Validation ===\n";

    DownloadManager manager;

    // 1. Insufficient space rejection
    {
        DownloadConfig config;
        config.url = "https://example.com/test-model.gguf";
        config.output_path = "models/llm/test-model.gguf";
        config.min_disk_space_bytes = 10ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
        config.safety_margin_bytes = 1024ULL * 1024ULL * 1024ULL;

        StorageValidation storage = manager.validateStorageHeadroom(config);
        ASSERT_TRUE(!storage.sufficient_space, "Correctly rejects download when requested space exceeds storage limits");
        std::cout << "  Rejection reason: " << storage.rejection_reason << "\n";
    }

    // 2. Partial download cleanup
    {
        std::string partial_path = "models/llm/partial_test.gguf.tmp";
        std::ofstream out(partial_path);
        out << "partial file content";
        out.close();

        manager.cleanPartialDownload(partial_path);
        
        std::ifstream in(partial_path);
        ASSERT_TRUE(!in.good(), "Partial download cleanup removes temp/partial files");
    }

    // 3. Rollback cleanup validation
    {
        std::string corrupted_path = "models/llm/corrupted_test.gguf";
        std::ofstream out(corrupted_path);
        out << "corrupted file content";
        out.close();

        manager.cleanupCorruptedDownload(corrupted_path);
        
        std::ifstream in(corrupted_path);
        ASSERT_TRUE(!in.good(), "Corrupted download cleanup removes corrupted GGUF files");
    }
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "ISHA AI — STORAGE CHAOS VALIDATION\n";
    std::cout << "==================================================\n";

    testStorageChaos();

    std::cout << "\n==================================================\n";
    std::cout << "STORAGE CHAOS BENCHMARK: " << g_pass << " PASS, " << g_fail << " FAIL\n";
    std::cout << "==================================================\n";

    return g_fail > 0 ? 1 : 0;
}
