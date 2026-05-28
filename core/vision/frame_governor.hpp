#ifndef ISHA_AI_FRAME_GOVERNOR_HPP
#define ISHA_AI_FRAME_GOVERNOR_HPP

#include <cstdint>
#include <cstddef>
#include <deque>
#include <mutex>
#include <atomic>
#include <chrono>

namespace isha {

struct FrameEntry {
    uint64_t frame_id = 0;
    uint64_t timestamp = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    size_t data_size = 0;
};

class FrameGovernor {
public:
    explicit FrameGovernor(size_t max_frames_per_session = 5,
                           uint32_t cooldown_ms = 500);
    ~FrameGovernor() = default;

    // Frame acceptance check
    bool canAcceptFrame() const;

    // Submit a frame - returns false if rejected (cooldown not elapsed)
    bool submitFrame(FrameEntry entry);

    // Circular eviction of oldest frame
    void evictOldest();

    // Queue inspection
    size_t queueSize() const;

    // Statistics
    size_t totalEvictions() const { return total_evictions_.load(); }
    size_t totalRejections() const { return total_rejections_.load(); }

    // Reset all state
    void reset();

private:
    size_t max_frames_;
    std::chrono::milliseconds cooldown_;

    std::deque<FrameEntry> frame_queue_;
    std::chrono::steady_clock::time_point last_frame_time_;

    std::atomic<size_t> total_evictions_{0};
    std::atomic<size_t> total_rejections_{0};

    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_FRAME_GOVERNOR_HPP
