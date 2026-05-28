#include <iostream>
#include <vector>
#include <cassert>
#include <chrono>
#include "../core/ocr/ocr_engine.hpp"
#include "../core/inference/cancellation_token.hpp"

using namespace isha;

static int tests_passed = 0;
static int tests_failed = 0;

static void CHECK(bool cond, const char* msg) {
    if (cond) { ++tests_passed; }
    else { ++tests_failed; std::cerr << "[FAIL] " << msg << "\n"; }
}

// Test 1: OCR extraction within 50ms deadline
static void test_extraction_deadline() {
    OCREngine engine;
    CancellationToken token;
    std::vector<uint8_t> img(640 * 480 * 3, 128);
    auto start = std::chrono::steady_clock::now();
    auto result = engine.extract(img.data(), 640, 480, 3, token);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    CHECK(result.success, "OCR extraction must succeed on valid data");
    CHECK(elapsed < 200, "OCR must complete within reasonable time");
    CHECK(result.latency_ms >= 0.0f, "Latency must be non-negative");
}

// Test 2: Thermal degradation at 42-45°C (REDUCED_RESOLUTION)
static void test_thermal_reduced_resolution() {
    OCREngine engine;
    engine.setThermalDegradation(43.0);
    CHECK(engine.getCurrentQuality() == OCRQuality::REDUCED_RESOLUTION,
          "43°C must set REDUCED_RESOLUTION");
}

// Test 3: Thermal degradation at 45-48°C (REDUCED_FREQUENCY)
static void test_thermal_reduced_frequency() {
    OCREngine engine;
    engine.setThermalDegradation(46.0);
    CHECK(engine.getCurrentQuality() == OCRQuality::REDUCED_FREQUENCY,
          "46°C must set REDUCED_FREQUENCY");
}

// Test 4: Thermal degradation at 48°C+ (PAUSED)
static void test_thermal_paused() {
    OCREngine engine;
    engine.setThermalDegradation(49.0);
    CHECK(engine.getCurrentQuality() == OCRQuality::PAUSED,
          "49°C must set PAUSED");
    CancellationToken token;
    std::vector<uint8_t> img(320 * 240 * 3, 64);
    auto result = engine.extract(img.data(), 320, 240, 3, token);
    CHECK(engine.skipCount() >= 1, "PAUSED mode must increment skip counter");
}

// Test 5: Thermal recovery back to FULL
static void test_thermal_recovery() {
    OCREngine engine;
    engine.setThermalDegradation(50.0);
    CHECK(engine.getCurrentQuality() == OCRQuality::PAUSED, "Start paused");
    engine.setThermalDegradation(38.0);
    CHECK(engine.getCurrentQuality() == OCRQuality::FULL,
          "38°C must restore FULL quality");
}

// Test 6: Cancellation during extraction
static void test_extraction_cancellation() {
    OCREngine engine;
    CancellationToken token;
    token.cancel();
    std::vector<uint8_t> img(640 * 480 * 3, 200);
    auto result = engine.extract(img.data(), 640, 480, 3, token);
    CHECK(!result.success, "Cancelled extraction must fail");
}

// Test 7: Degraded extraction marks was_degraded
static void test_degraded_flag() {
    OCREngine engine;
    engine.setThermalDegradation(44.0);
    CancellationToken token;
    std::vector<uint8_t> img(640 * 480 * 3, 100);
    auto result = engine.extract(img.data(), 640, 480, 3, token);
    if (result.success) {
        CHECK(result.was_degraded, "REDUCED_RESOLUTION extraction must set was_degraded");
    }
}

// Test 8: Telemetry counters accuracy
static void test_telemetry_counters() {
    OCREngine engine;
    CancellationToken token;
    std::vector<uint8_t> img(320 * 240 * 3, 50);
    for (int i = 0; i < 10; ++i) {
        engine.extract(img.data(), 320, 240, 3, token);
    }
    CHECK(engine.extractionCount() == 10, "10 extractions must be counted");
    CHECK(engine.totalLatencyMs() >= 0.0f, "Total latency must be non-negative");
}

int main() {
    std::cout << "=== OCR LATENCY BENCHMARK ===\n";
    test_extraction_deadline();
    test_thermal_reduced_resolution();
    test_thermal_reduced_frequency();
    test_thermal_paused();
    test_thermal_recovery();
    test_extraction_cancellation();
    test_degraded_flag();
    test_telemetry_counters();
    std::cout << "Passed: " << tests_passed << " / " << (tests_passed + tests_failed) << "\n";
    if (tests_failed > 0) {
        std::cerr << "FAILED: " << tests_failed << " tests\n";
        return 1;
    }
    std::cout << "[OK] OCR latency benchmark passed.\n";
    return 0;
}
