#ifndef ISHA_AI_LATENCY_TRACKER_HPP
#define ISHA_AI_LATENCY_TRACKER_HPP

#include <cstddef>
#include <deque>
#include <mutex>

namespace isha {

class LatencyTracker {
public:
    explicit LatencyTracker(size_t window_size = 1000);

    void record(float latency_ms);

    // Percentiles computed over rolling window
    float p50() const;
    float p95() const;
    float p99() const;
    float worst() const;
    float mean() const;
    size_t count() const;

    // Trend analysis: is latency increasing over time?
    float recentDriftRatio() const;  // >1.0 means getting slower

    void reset();

private:
    size_t window_size_;
    std::deque<float> samples_;
    float worst_ = 0.0f;
    mutable std::mutex mutex_;

    float percentile(float p) const;  // internal, expects lock held
};

} // namespace isha

#endif // ISHA_AI_LATENCY_TRACKER_HPP
