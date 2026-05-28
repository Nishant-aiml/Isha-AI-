#include "latency_tracker.hpp"
#include "../logging/logger.hpp"
#include <algorithm>
#include <numeric>
#include <vector>

namespace isha {

LatencyTracker::LatencyTracker(size_t window_size)
    : window_size_(window_size) {}

void LatencyTracker::record(float latency_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    samples_.push_back(latency_ms);

    if (samples_.size() > window_size_) {
        samples_.pop_front();
    }

    if (latency_ms > worst_) {
        worst_ = latency_ms;
    }
}

float LatencyTracker::p50() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return percentile(0.5f);
}

float LatencyTracker::p95() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return percentile(0.95f);
}

float LatencyTracker::p99() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return percentile(0.99f);
}

float LatencyTracker::worst() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return worst_;
}

float LatencyTracker::mean() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (samples_.empty()) {
        return 0.0f;
    }
    float sum = std::accumulate(samples_.begin(), samples_.end(), 0.0f);
    return sum / static_cast<float>(samples_.size());
}

size_t LatencyTracker::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return samples_.size();
}

float LatencyTracker::recentDriftRatio() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (samples_.size() < 4) {
        return 1.0f;  // Not enough data for trend analysis
    }

    size_t quarter = samples_.size() / 4;
    if (quarter == 0) {
        return 1.0f;
    }

    // Mean of first 25% of samples (oldest)
    float first_sum = 0.0f;
    for (size_t i = 0; i < quarter; ++i) {
        first_sum += samples_[i];
    }
    float first_mean = first_sum / static_cast<float>(quarter);

    // Mean of last 25% of samples (newest)
    float last_sum = 0.0f;
    size_t last_start = samples_.size() - quarter;
    for (size_t i = last_start; i < samples_.size(); ++i) {
        last_sum += samples_[i];
    }
    float last_mean = last_sum / static_cast<float>(quarter);

    if (first_mean <= 0.0f) {
        return 1.0f;  // Avoid division by zero
    }

    return last_mean / first_mean;
}

void LatencyTracker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    samples_.clear();
    worst_ = 0.0f;
    ISHA_LOG_DEBUG("LatencyTracker", "Latency tracking state reset.");
}

float LatencyTracker::percentile(float p) const {
    // Caller must hold lock
    if (samples_.empty()) {
        return 0.0f;
    }

    // Sort a copy for percentile calculation
    std::vector<float> sorted(samples_.begin(), samples_.end());
    std::sort(sorted.begin(), sorted.end());

    size_t index = static_cast<size_t>(p * static_cast<float>(sorted.size() - 1));
    if (index >= sorted.size()) index = sorted.size() - 1;
    return sorted[index];
}

} // namespace isha
