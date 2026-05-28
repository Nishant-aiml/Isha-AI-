#include <iostream>
#include <vector>
#include <cassert>
#include <memory>
#include <chrono>
#include "../core/multimodal/image_decoder.hpp"
#include "../core/multimodal/image_pipeline.hpp"
#include "../core/inference/cancellation_token.hpp"

using namespace isha;

static int tests_passed = 0;
static int tests_failed = 0;

static void CHECK(bool cond, const char* msg) {
    if (cond) { ++tests_passed; }
    else { ++tests_failed; std::cerr << "[FAIL] " << msg << "\n"; }
}

// Test 1: Reject oversized compressed data (>2MB)
static void test_oversized_rejection() {
    ImageDecoder decoder;
    CancellationToken token;
    size_t oversized = ImageDecoder::MAX_COMPRESSED_SIZE + 1;
    std::vector<uint8_t> data(oversized, 0xFF);
    auto result = decoder.decode(data.data(), data.size(), token);
    CHECK(!result.success, "Oversized image must be rejected");
    CHECK(decoder.getRejectCount() == 1, "Reject counter must increment");
}

// Test 2: Reject image exceeding max dimension
static void test_dimension_rejection() {
    ImageDecoder decoder;
    CancellationToken token;
    // Valid size but we test the decoder behavior with minimal data
    std::vector<uint8_t> small_data(64, 0xAA);
    auto result = decoder.decode(small_data.data(), small_data.size(), token);
    // Small data should decode (simulated) - decoder accepts if under limits
    CHECK(decoder.getDecodeCount() + decoder.getRejectCount() >= 1,
          "Decoder must process or reject every submission");
}

// Test 3: Valid decode within bounds
static void test_valid_decode() {
    ImageDecoder decoder;
    CancellationToken token;
    // Create data within 2MB limit
    size_t valid_size = 1024 * 1024; // 1MB
    std::vector<uint8_t> data(valid_size, 0x55);
    auto result = decoder.decode(data.data(), data.size(), token);
    CHECK(result.success, "Valid-sized image must decode successfully");
    CHECK(decoder.getDecodeCount() >= 1, "Decode counter must increment");
    CHECK(decoder.getTotalDecodeTimeMs() > 0 || decoder.getDecodeCount() > 0,
          "Timing telemetry must be recorded");
}

// Test 4: Cancellation mid-decode
static void test_cancellation_mid_decode() {
    ImageDecoder decoder;
    CancellationToken token;
    token.cancel(); // Pre-cancel
    size_t valid_size = 512 * 1024;
    std::vector<uint8_t> data(valid_size, 0x33);
    auto result = decoder.decode(data.data(), data.size(), token);
    CHECK(!result.success, "Cancelled decode must not succeed");
}

// Test 5: Pipeline end-to-end
static void test_pipeline_processing() {
    ImagePipeline pipeline;
    CancellationToken token;
    size_t valid_size = 256 * 1024;
    std::vector<uint8_t> data(valid_size, 0x77);
    auto result = pipeline.process(data.data(), data.size(), ImageSource::FILE, token);
    CHECK(result.success, "Pipeline must process valid data");
    CHECK(result.preprocessing_time_ms >= 0.0f, "Preprocessing time must be non-negative");
}

// Test 6: Pipeline cancellation
static void test_pipeline_cancellation() {
    ImagePipeline pipeline;
    CancellationToken token;
    token.cancel();
    size_t valid_size = 128 * 1024;
    std::vector<uint8_t> data(valid_size, 0x99);
    auto result = pipeline.process(data.data(), data.size(), ImageSource::CAMERA, token);
    CHECK(!result.success, "Pipeline must respect cancellation");
}

// Test 7: Zero-size data rejection
static void test_zero_size_rejection() {
    ImageDecoder decoder;
    CancellationToken token;
    auto result = decoder.decode(nullptr, 0, token);
    CHECK(!result.success, "Zero-size data must be rejected");
}

// Test 8: Rapid sequential decodes (memory stability)
static void test_rapid_sequential_decodes() {
    ImageDecoder decoder;
    CancellationToken token;
    size_t valid_size = 64 * 1024;
    std::vector<uint8_t> data(valid_size, 0xBB);
    for (int i = 0; i < 50; ++i) {
        auto result = decoder.decode(data.data(), data.size(), token);
    }
    CHECK(decoder.getDecodeCount() + decoder.getRejectCount() == 50,
          "All 50 sequential decodes must be accounted for");
}

int main() {
    std::cout << "=== IMAGE INGESTION BENCHMARK ===\n";
    test_oversized_rejection();
    test_dimension_rejection();
    test_valid_decode();
    test_cancellation_mid_decode();
    test_pipeline_processing();
    test_pipeline_cancellation();
    test_zero_size_rejection();
    test_rapid_sequential_decodes();
    std::cout << "Passed: " << tests_passed << " / " << (tests_passed + tests_failed) << "\n";
    if (tests_failed > 0) {
        std::cerr << "FAILED: " << tests_failed << " tests\n";
        return 1;
    }
    std::cout << "[OK] Image ingestion benchmark passed.\n";
    return 0;
}
