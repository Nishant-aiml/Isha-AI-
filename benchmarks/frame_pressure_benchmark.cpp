#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include "../core/vision/frame_governor.hpp"

using namespace isha;

static int tests_passed = 0;
static int tests_failed = 0;

static void CHECK(bool cond, const char* msg) {
    if (cond) { ++tests_passed; }
    else { ++tests_failed; std::cerr << "[FAIL] " << msg << "\n"; }
}

static uint64_t now_ms() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

// Test 1: Submit up to max frames
static void test_max_frame_submission() {
    FrameGovernor gov(5, 0); // No cooldown for this test
    for (uint64_t i = 0; i < 5; ++i) {
        FrameEntry e;
        e.frame_id = i;
        e.timestamp = now_ms();
        e.width = 640; e.height = 480; e.data_size = 640*480*3;
        bool accepted = gov.submitFrame(e);
        CHECK(accepted, "Frame within limit must be accepted");
    }
    CHECK(gov.queueSize() == 5, "Queue must hold exactly 5 frames");
}

// Test 2: Auto-eviction when exceeding max
static void test_auto_eviction() {
    FrameGovernor gov(3, 0);
    for (uint64_t i = 0; i < 5; ++i) {
        FrameEntry e;
        e.frame_id = i;
        e.timestamp = now_ms();
        e.width = 320; e.height = 240; e.data_size = 320*240*3;
        gov.submitFrame(e);
    }
    CHECK(gov.queueSize() <= 3, "Queue must not exceed max after eviction");
    CHECK(gov.totalEvictions() >= 2, "At least 2 evictions must occur");
}

// Test 3: Cooldown enforcement
static void test_cooldown_enforcement() {
    FrameGovernor gov(5, 200); // 200ms cooldown
    FrameEntry e1;
    e1.frame_id = 1; e1.timestamp = now_ms();
    e1.width = 320; e1.height = 240; e1.data_size = 320*240*3;
    bool first = gov.submitFrame(e1);
    CHECK(first, "First frame must be accepted");
    // Immediate second frame should be rejected (cooldown)
    FrameEntry e2;
    e2.frame_id = 2; e2.timestamp = now_ms();
    e2.width = 320; e2.height = 240; e2.data_size = 320*240*3;
    bool second = gov.submitFrame(e2);
    CHECK(!second, "Frame within cooldown must be rejected");
    CHECK(gov.totalRejections() >= 1, "Rejection counter must increment");
}

// Test 4: Cooldown recovery
static void test_cooldown_recovery() {
    FrameGovernor gov(5, 50); // 50ms cooldown
    FrameEntry e1;
    e1.frame_id = 1; e1.timestamp = now_ms();
    e1.width = 320; e1.height = 240; e1.data_size = 320*240*3;
    gov.submitFrame(e1);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    FrameEntry e2;
    e2.frame_id = 2; e2.timestamp = now_ms();
    e2.width = 320; e2.height = 240; e2.data_size = 320*240*3;
    bool accepted = gov.submitFrame(e2);
    CHECK(accepted, "Frame after cooldown must be accepted");
}

// Test 5: Manual eviction
static void test_manual_eviction() {
    FrameGovernor gov(5, 0);
    for (uint64_t i = 0; i < 3; ++i) {
        FrameEntry e;
        e.frame_id = i; e.timestamp = now_ms();
        e.width = 640; e.height = 480; e.data_size = 640*480*3;
        gov.submitFrame(e);
    }
    CHECK(gov.queueSize() == 3, "Queue must have 3 frames");
    gov.evictOldest();
    CHECK(gov.queueSize() == 2, "Queue must have 2 after eviction");
}

// Test 6: Reset clears everything
static void test_reset() {
    FrameGovernor gov(5, 0);
    for (uint64_t i = 0; i < 4; ++i) {
        FrameEntry e;
        e.frame_id = i; e.timestamp = now_ms();
        e.width = 320; e.height = 240; e.data_size = 320*240*3;
        gov.submitFrame(e);
    }
    gov.reset();
    CHECK(gov.queueSize() == 0, "Queue must be empty after reset");
}

// Test 7: Pressure test - rapid 100 frame submissions
static void test_pressure() {
    FrameGovernor gov(5, 0);
    for (uint64_t i = 0; i < 100; ++i) {
        FrameEntry e;
        e.frame_id = i; e.timestamp = now_ms();
        e.width = 1920; e.height = 1080; e.data_size = 1920*1080*3;
        gov.submitFrame(e);
    }
    CHECK(gov.queueSize() <= 5, "Queue must stay bounded under pressure");
    CHECK(gov.totalEvictions() >= 95, "Most frames must be evicted under pressure");
}

int main() {
    std::cout << "=== FRAME PRESSURE BENCHMARK ===\n";
    test_max_frame_submission();
    test_auto_eviction();
    test_cooldown_enforcement();
    test_cooldown_recovery();
    test_manual_eviction();
    test_reset();
    test_pressure();
    std::cout << "Passed: " << tests_passed << " / " << (tests_passed + tests_failed) << "\n";
    if (tests_failed > 0) {
        std::cerr << "FAILED: " << tests_failed << " tests\n";
        return 1;
    }
    std::cout << "[OK] Frame pressure benchmark passed.\n";
    return 0;
}
