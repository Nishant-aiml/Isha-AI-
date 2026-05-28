#include <iostream>
#include <string>
#include <cassert>
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

void testDownload15BModel() {
    std::cout << "\n=== Test: Model Acquisition via DownloadManager ===\n";

    DownloadManager manager;
    DownloadConfig config;
    // Bartowski's Qwen2.5-1.5B-Instruct-Q4_K_M.gguf URL
    config.url = "https://huggingface.co/bartowski/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/Qwen2.5-1.5B-Instruct-Q4_K_M.gguf";
    config.output_path = "models/llm/qwen2.5-1.5b-instruct-q4_k_m.gguf";
    config.expected_sha256 = ""; // Skip SHA verification for flexibility
    config.expected_size_bytes = 0; // Skip size validation for flexibility
    config.min_disk_space_bytes = 1024ULL * 1024ULL * 1024ULL * 2ULL; // Require 2GB free space
    config.safety_margin_bytes = 512ULL * 1024ULL * 1024ULL; // 512MB margin
    config.validate_post_download = true;

    // Validate storage headroom first
    StorageValidation storage = manager.validateStorageHeadroom(config);
    ASSERT_TRUE(storage.sufficient_space, "Storage has sufficient headroom (1.5x model size)");

    std::cout << "Starting model download via DownloadManager. This might take a few minutes...\n";
    
    DownloadResult result = manager.download(config, [](uint64_t downloaded, uint64_t total, float percent) {
        static int last_percent = -1;
        int p = static_cast<int>(percent);
        if (p % 10 == 0 && p != last_percent) {
            std::cout << "  Download progress: " << p << "% (" << downloaded / (1024 * 1024) << "/" << total / (1024 * 1024) << " MB)\n";
            last_percent = p;
        }
    });

    ASSERT_TRUE(result.success, "Download succeeded");
    if (result.success) {
        ASSERT_TRUE(result.bytes_downloaded > 0, "Downloaded non-zero bytes");
        std::cout << "Successfully downloaded to: " << result.filepath << " (" << result.bytes_downloaded << " bytes)\n";
    } else {
        std::cout << "Download failed: " << result.error << "\n";
    }
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "ISHA AI — MODEL ACQUISITION BENCHMARK\n";
    std::cout << "==================================================\n";

    testDownload15BModel();

    std::cout << "\n==================================================\n";
    std::cout << "MODEL ACQUISITION BENCHMARK: " << g_pass << " PASS, " << g_fail << " FAIL\n";
    std::cout << "==================================================\n";

    return g_fail > 0 ? 1 : 0;
}
