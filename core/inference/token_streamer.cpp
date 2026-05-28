#include "token_streamer.hpp"
#include "../logging/logger.hpp"
#include <thread>

namespace isha {

TokenStreamer::TokenStreamer(size_t max_queue_size)
    : max_queue_size_(max_queue_size) {}

bool TokenStreamer::pushToken(const std::string& token, const CancellationToken& cancel_token) {
    if (cancel_token.isCancelled()) return false;

    std::unique_lock<std::mutex> lock(mutex_);
    
    // Backpressure: If queue is full, throttle execution to allow consumers to catch up
    while (token_queue_.size() >= max_queue_size_) {
        lock.unlock();
        ISHA_LOG_WARN("TokenStreamer", "Queue full, applying backpressure throttling (5ms)...");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        lock.lock();

        if (cancel_token.isCancelled()) return false;
    }

    token_queue_.push(token);
    return true;
}

std::vector<std::string> TokenStreamer::flushBatch() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> batch;
    while (!token_queue_.empty()) {
        batch.push_back(token_queue_.front());
        token_queue_.pop();
    }
    return batch;
}

void TokenStreamer::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::queue<std::string> empty;
    std::swap(token_queue_, empty);
}

} // namespace isha
