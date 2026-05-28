#ifndef ISHA_AI_DRIFT_TRACKER_HPP
#define ISHA_AI_DRIFT_TRACKER_HPP

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace isha {

class DriftTracker {
public:
    struct Window {
        float tok_sec;
        float thermal;
        float frag;
        float cancel_lat;
        float cleanup_lat;
        uint64_t rss;
        uint64_t page_faults;
        float scheduler_latency;
    };

    void addWindow(const Window& w) {
        windows_.push_back(w);
    }

    void reset() {
        windows_.clear();
    }

    size_t size() const {
        return windows_.size();
    }

    // Metric offsets: 0=tok_sec, 1=thermal, 2=frag, 3=cancel_lat, 4=cleanup_lat, 5=rss, 6=page_faults, 7=scheduler_latency
    float computeDrift(int metric) const {
        if (windows_.size() < 2) return 0.0f;
        float first = getMetricValue(windows_.front(), metric);
        float last = getMetricValue(windows_.back(), metric);
        if (std::abs(first) < 1e-5f) return 0.0f;
        return ((last - first) / std::abs(first)) * 100.0f;
    }

    bool isMonotonicGrowth(int metric) const {
        if (windows_.size() < 3) return false;
        float prev = getMetricValue(windows_[0], metric);
        for (size_t i = 1; i < windows_.size(); ++i) {
            float cur = getMetricValue(windows_[i], metric);
            if (cur < prev) return false;
            prev = cur;
        }
        return getMetricValue(windows_.back(), metric) > getMetricValue(windows_.front(), metric);
    }

private:
    std::vector<Window> windows_;

    float getMetricValue(const Window& w, int metric) const {
        switch (metric) {
            case 0: return w.tok_sec;
            case 1: return w.thermal;
            case 2: return w.frag;
            case 3: return w.cancel_lat;
            case 4: return w.cleanup_lat;
            case 5: return static_cast<float>(w.rss);
            case 6: return static_cast<float>(w.page_faults);
            case 7: return w.scheduler_latency;
            default: return 0.0f;
        }
    }
};

} // namespace isha

#endif // ISHA_AI_DRIFT_TRACKER_HPP
