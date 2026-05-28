#include "frame_governor.hpp"
#include "../logging/logger.hpp"
#include <string>

namespace isha {

FrameGovernor::FrameGovernor(size_t max_frames_per_session, uint32_t cooldown_ms)
    : max_frames_(max_frames_per_session),
      cooldown_(cooldown_ms),
      last_frame_time_(std::chrono::steady_clock::time_point::min()) {
    ISHA_LOG_INFO("FrameGovernor", "Initialized: max_frames=" + std::to_string(max_frames_per_session) +
                  ", cooldown=" + std::to_string(cooldown_ms) + "ms");
}

bool FrameGovernor::canAcceptFrame() const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check cooldown
    auto now = std::chrono::steady_clock::now();
    if (last_frame_time_ != std::chrono::steady_clock::time_point::min()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time_);
        if (elapsed < cooldown_) {
            return false;
        }
    }

    return true;
}

bool FrameGovernor::submitFrame(FrameEntry entry) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check cooldown
    auto now = std::chrono::steady_clock::now();
    if (last_frame_time_ != std::chrono::steady_clock::time_point::min()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time_);
        if (elapsed < cooldown_) {
            total_rejections_.fetch_add(1, std::memory_order_relaxed);
            ISHA_LOG_DEBUG("FrameGovernor", "Frame " + std::to_string(entry.frame_id) +
                          " rejected: cooldown not elapsed");
            return false;
        }
    }

    // Auto-evict oldest when queue is full
    if (frame_queue_.size() >= max_frames_) {
        uint64_t evicted_id = frame_queue_.front().frame_id;
        frame_queue_.pop_front();
        total_evictions_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_DEBUG("FrameGovernor", "Evicted oldest frame " + std::to_string(evicted_id) +
                      " (queue was full)");
    }

    frame_queue_.push_back(std::move(entry));
    last_frame_time_ = now;

    ISHA_LOG_DEBUG("FrameGovernor", "Frame " + std::to_string(frame_queue_.back().frame_id) +
                  " accepted, queue_size=" + std::to_string(frame_queue_.size()));
    return true;
}

void FrameGovernor::evictOldest() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!frame_queue_.empty()) {
        uint64_t evicted_id = frame_queue_.front().frame_id;
        frame_queue_.pop_front();
        total_evictions_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_DEBUG("FrameGovernor", "Manually evicted frame " + std::to_string(evicted_id));
    }
}

size_t FrameGovernor::queueSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frame_queue_.size();
}

void FrameGovernor::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    frame_queue_.clear();
    total_evictions_.store(0, std::memory_order_relaxed);
    total_rejections_.store(0, std::memory_order_relaxed);
    last_frame_time_ = std::chrono::steady_clock::time_point::min();
    ISHA_LOG_INFO("FrameGovernor", "Reset complete.");
}

} // namespace isha
