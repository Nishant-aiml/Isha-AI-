#include "inference/token_streamer.hpp"
#include "logging/logger.hpp"
#include <iostream>
#include <cassert>
#include <thread>

int main() {
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::DEBUG);
    ISHA_LOG_INFO("Benchmark", "=== STARTING STREAMING BENCHMARK ===");

    // Bounded queue with max 3 tokens
    isha::TokenStreamer streamer(3);
    isha::CancellationToken cancel_token;

    // 1. Push within bounds
    bool ok1 = streamer.pushToken("Token1", cancel_token);
    bool ok2 = streamer.pushToken("Token2", cancel_token);
    assert(ok1 && ok2);
    assert(streamer.getQueueSize() == 2);

    // 2. Test flushing batch
    std::vector<std::string> batch = streamer.flushBatch();
    assert(batch.size() == 2);
    assert(batch[0] == "Token1");
    assert(streamer.getQueueSize() == 0);

    // 3. Test backpressure throttling (thread push will block if queue is full)
    streamer.pushToken("T1", cancel_token);
    streamer.pushToken("T2", cancel_token);
    streamer.pushToken("T3", cancel_token);
    assert(streamer.getQueueSize() == 3);

    // Push fourth token in a background thread to verify it blocks and applies backpressure
    std::thread t([&streamer, &cancel_token]() {
        // Will block until main thread flushes queue
        bool ok = streamer.pushToken("T4", cancel_token);
        assert(ok);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    assert(streamer.getQueueSize() == 3); // Still 3, fourth is blocked

    // Flush batch to release backpressure
    std::vector<std::string> batch2 = streamer.flushBatch();
    assert(batch2.size() == 3);
    t.join();
    assert(streamer.getQueueSize() == 1); // T4 now inserted

    // 4. Test cancellation storm
    cancel_token.cancel();
    bool fail_ok = streamer.pushToken("T5", cancel_token);
    assert(!fail_ok); // Rejects token due to cancellation

    ISHA_LOG_INFO("Benchmark", "=== STREAMING BENCHMARK COMPLETED SUCCESSFULLY ===");
    return 0;
}
