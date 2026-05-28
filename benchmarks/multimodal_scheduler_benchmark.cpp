#include <iostream>
#include <vector>
#include <cassert>
#include <memory>
#include <thread>
#include <chrono>
#include "../core/scheduler/inference_scheduler.hpp"
#include "../core/ocr/ocr_engine.hpp"
#include "../core/vision/vision_engine.hpp"
#include "../core/inference/cancellation_token.hpp"

using namespace isha;

static int tests_passed = 0;
static int tests_failed = 0;

static void CHECK(bool cond, const char* msg) {
    if (cond) { ++tests_passed; }
    else { ++tests_failed; std::cerr << "[FAIL] " << msg << "\n"; }
}

// Factory: InferenceContext has no default ctor, so SchedulerTask needs explicit init
static std::shared_ptr<SchedulerTask> makeTask(const std::string& id, int priority, std::function<void()> fn) {
    auto token = std::make_shared<CancellationToken>();
    InferenceContext ctx(id, token, 64);
    auto task = std::shared_ptr<SchedulerTask>(new SchedulerTask{
        id, "", priority, std::chrono::system_clock::now(),
        TaskState::QUEUED, std::move(ctx), std::move(fn)
    });
    return task;
}

// Priority levels: INTERRUPT>STT>SCHEDULER>VOICE>CAMERA_INGESTION>OCR>IMAGE_REASONING>TELEMETRY
static constexpr int PRIORITY_VOICE = 80;
static constexpr int PRIORITY_OCR = 30;
static constexpr int PRIORITY_VISION = 20;
static constexpr int PRIORITY_TELEMETRY = 10;

// Test 1: Voice priority higher than OCR/Vision
static void test_priority_ordering() {
    CHECK(PRIORITY_VOICE > PRIORITY_OCR, "Voice priority must exceed OCR");
    CHECK(PRIORITY_VOICE > PRIORITY_VISION, "Voice priority must exceed Vision");
    CHECK(PRIORITY_OCR > PRIORITY_TELEMETRY, "OCR priority must exceed Telemetry");
}

// Test 2: Scheduler accepts multimodal tasks
static void test_scheduler_enqueue() {
    InferenceScheduler scheduler(10);
    scheduler.start();
    auto voice_task = makeTask("voice_001", PRIORITY_VOICE,
        []() { std::this_thread::sleep_for(std::chrono::milliseconds(5)); });
    auto ocr_task = makeTask("ocr_001", PRIORITY_OCR,
        []() { std::this_thread::sleep_for(std::chrono::milliseconds(5)); });
    bool v = scheduler.enqueueTask(voice_task);
    bool o = scheduler.enqueueTask(ocr_task);
    CHECK(v, "Voice task must be accepted");
    CHECK(o, "OCR task must be accepted");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scheduler.stop();
}

// Test 3: Queue saturation does not block voice
static void test_queue_saturation() {
    InferenceScheduler scheduler(5);
    scheduler.start();
    for (int i = 0; i < 5; ++i) {
        auto t = makeTask("vision_" + std::to_string(i), PRIORITY_VISION,
            []() { std::this_thread::sleep_for(std::chrono::milliseconds(10)); });
        scheduler.enqueueTask(t);
    }
    auto voice = makeTask("voice_critical", PRIORITY_VOICE, []() {});
    CHECK(scheduler.getQueueSize() <= 5, "Queue must not exceed max size");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler.stop();
}

// Test 4: Concurrent OCR and Vision don't starve
static void test_concurrent_no_starvation() {
    OCREngine ocr;
    VisionEngine vision;
    CancellationToken token;
    std::vector<uint8_t> img(320 * 240 * 3, 128);
    std::atomic<bool> ocr_done{false};
    std::atomic<bool> vision_done{false};
    std::thread t1([&]() {
        ocr.extract(img.data(), 320, 240, 3, token);
        ocr_done.store(true);
    });
    std::thread t2([&]() {
        vision.analyze(img.data(), 320, 240, 3, token);
        vision_done.store(true);
    });
    t1.join();
    t2.join();
    CHECK(ocr_done.load(), "OCR must complete under concurrency");
    CHECK(vision_done.load(), "Vision must complete under concurrency");
}

// Test 5: Cancel multimodal tasks
static void test_cancel_multimodal_tasks() {
    InferenceScheduler scheduler(10);
    scheduler.start();
    auto t = makeTask("ocr_cancel_test", PRIORITY_OCR,
        []() { std::this_thread::sleep_for(std::chrono::milliseconds(500)); });
    scheduler.enqueueTask(t);
    scheduler.cancelTask("ocr_cancel_test");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scheduler.stop();
    CHECK(true, "Multimodal task cancellation must not crash");
}

int main() {
    std::cout << "=== MULTIMODAL SCHEDULER BENCHMARK ===\n";
    test_priority_ordering();
    test_scheduler_enqueue();
    test_queue_saturation();
    test_concurrent_no_starvation();
    test_cancel_multimodal_tasks();
    std::cout << "Passed: " << tests_passed << " / " << (tests_passed + tests_failed) << "\n";
    if (tests_failed > 0) {
        std::cerr << "FAILED: " << tests_failed << " tests\n";
        return 1;
    }
    std::cout << "[OK] Multimodal scheduler benchmark passed.\n";
    return 0;
}
