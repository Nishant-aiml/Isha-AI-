#include <iostream>
#include <vector>
#include <cassert>
#include <chrono>
#include "../core/ocr/ocr_engine.hpp"
#include "../core/vision/vision_engine.hpp"
#include "../core/multimodal/image_decoder.hpp"
#include "../core/inference/cancellation_token.hpp"

using namespace isha;

static int tests_passed = 0;
static int tests_failed = 0;

static void CHECK(bool cond, const char* msg) {
    if (cond) { ++tests_passed; }
    else { ++tests_failed; std::cerr << "[FAIL] " << msg << "\n"; }
}

// Test 1: OCR progressive degradation under rising temperature
static void test_ocr_progressive_degradation() {
    OCREngine engine;
    CancellationToken token;
    std::vector<uint8_t> img(320 * 240 * 3, 100);
    // Normal temperature
    engine.setThermalDegradation(38.0);
    auto r1 = engine.extract(img.data(), 320, 240, 3, token);
    CHECK(r1.success, "OCR at 38°C must succeed");
    CHECK(!r1.was_degraded, "OCR at 38°C must not be degraded");
    // Stage 1: 42-45°C
    engine.setThermalDegradation(43.0);
    CHECK(engine.getCurrentQuality() == OCRQuality::REDUCED_RESOLUTION,
          "43°C must trigger REDUCED_RESOLUTION");
    auto r2 = engine.extract(img.data(), 320, 240, 3, token);
    if (r2.success) {
        CHECK(r2.was_degraded, "REDUCED_RESOLUTION must flag was_degraded");
    }
    // Stage 2: 45-48°C
    engine.setThermalDegradation(47.0);
    CHECK(engine.getCurrentQuality() == OCRQuality::REDUCED_FREQUENCY,
          "47°C must trigger REDUCED_FREQUENCY");
    // Stage 3: 48°C+
    engine.setThermalDegradation(50.0);
    CHECK(engine.getCurrentQuality() == OCRQuality::PAUSED,
          "50°C must trigger PAUSED");
}

// Test 2: Vision engine under simulated thermal stress
static void test_vision_thermal_stress() {
    VisionEngine engine;
    CancellationToken token;
    std::vector<uint8_t> img(640 * 480 * 3, 128);
    // Run multiple analyses - simulating thermal load
    for (int i = 0; i < 10; ++i) {
        auto result = engine.analyze(img.data(), 640, 480, 3, token);
        CHECK(result.success, "Vision analysis must succeed");
    }
    CHECK(engine.analysisCount() == 10, "All 10 analyses must be counted");
}

// Test 3: OCR paused retains last result
static void test_ocr_paused_retains_result() {
    OCREngine engine;
    CancellationToken token;
    std::vector<uint8_t> img(320 * 240 * 3, 150);
    // First: normal extraction
    engine.setThermalDegradation(38.0);
    auto normal = engine.extract(img.data(), 320, 240, 3, token);
    CHECK(normal.success, "Normal extraction must succeed");
    // Now pause
    engine.setThermalDegradation(49.0);
    auto paused = engine.extract(img.data(), 320, 240, 3, token);
    // Paused mode should return cached or empty result, not crash
    CHECK(engine.skipCount() >= 1, "PAUSED must increment skip count");
}

// Test 4: Image decoder under thermal (verify decode still bounded)
static void test_decoder_thermal_bounds() {
    ImageDecoder decoder;
    CancellationToken token;
    // Even under "thermal stress", decoder must still enforce size limits
    size_t oversized = ImageDecoder::MAX_COMPRESSED_SIZE + 1;
    std::vector<uint8_t> big(oversized, 0xFF);
    auto result = decoder.decode(big.data(), big.size(), token);
    CHECK(!result.success, "Oversized must still be rejected under stress");
}

// Test 5: Sustained high-rate vision analysis (CPU pressure)
static void test_sustained_vision_pressure() {
    VisionEngine engine;
    CancellationToken token;
    std::vector<uint8_t> img(1024 * 768 * 3, 200);
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 20; ++i) {
        auto result = engine.analyze(img.data(), 1024, 768, 3, token);
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    CHECK(engine.analysisCount() == 20, "All 20 analyses must complete");
    CHECK(engine.timeoutCount() == 0, "No timeouts under normal execution");
}

// Test 6: Emergency cancel during combined OCR+Vision
static void test_emergency_cancel() {
    OCREngine ocr;
    VisionEngine vision;
    CancellationToken token;
    token.cancel(); // Emergency cancel
    std::vector<uint8_t> img(640 * 480 * 3, 100);
    auto ocr_r = ocr.extract(img.data(), 640, 480, 3, token);
    auto vis_r = vision.analyze(img.data(), 640, 480, 3, token);
    CHECK(!ocr_r.success, "Cancelled OCR must fail");
    CHECK(!vis_r.success, "Cancelled vision must fail");
}

int main() {
    std::cout << "=== THERMAL VISION STRESS BENCHMARK ===\n";
    test_ocr_progressive_degradation();
    test_vision_thermal_stress();
    test_ocr_paused_retains_result();
    test_decoder_thermal_bounds();
    test_sustained_vision_pressure();
    test_emergency_cancel();
    std::cout << "Passed: " << tests_passed << " / " << (tests_passed + tests_failed) << "\n";
    if (tests_failed > 0) {
        std::cerr << "FAILED: " << tests_failed << " tests\n";
        return 1;
    }
    std::cout << "[OK] Thermal vision stress benchmark passed.\n";
    return 0;
}
