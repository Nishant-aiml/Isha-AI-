#ifndef ISHA_AI_TELEMETRY_HPP
#define ISHA_AI_TELEMETRY_HPP

#include "profiling_snapshot.hpp"
#include <string>
#include <vector>
#include <mutex>

namespace isha {

class Telemetry {
public:
    explicit Telemetry(size_t max_snapshots = 100);
    ~Telemetry() = default;

    // Record a snapshot
    void recordSnapshot(const ProfilingSnapshot& snapshot);
    
    // Get recent snapshots
    std::vector<ProfilingSnapshot> getRecentSnapshots(size_t count = 10) const;
    
    // Get aggregated statistics
    double averageInferenceLatency() const;
    double averageRetrievalLatency() const;
    double averageTokensPerSecond() const;
    double peakRamUsageMB() const;

    // Advanced Latency Distribution Analysis (p50, p95, p99, jitter)
    double getP50InferenceLatency() const;
    double getP95InferenceLatency() const;
    double getP99InferenceLatency() const;
    double getJitterVariance() const;
    double getQueueWaitVariance() const;

    // Fragmentation Telemetry
    double getAllocationChurn() const;
    double getMmapReuseEfficiency() const;
    double getFragmentationRatio() const;
    
    // Dump summary to log
    void dumpSummary() const;
    
    // Write telemetry to file
    void writeToFile(const std::string& path) const;
    
    size_t snapshotCount() const;

private:
    std::vector<ProfilingSnapshot> snapshots_;
    size_t max_snapshots_;
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_TELEMETRY_HPP
