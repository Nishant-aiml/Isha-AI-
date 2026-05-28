#include "telemetry.hpp"
#include "../logging/logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace isha {

Telemetry::Telemetry(size_t max_snapshots) : max_snapshots_(max_snapshots) {
    ISHA_LOG_INFO("Telemetry", "Initialized with max " + std::to_string(max_snapshots) + " snapshots.");
}

void Telemetry::recordSnapshot(const ProfilingSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.size() >= max_snapshots_) {
        snapshots_.erase(snapshots_.begin());
    }
    snapshots_.push_back(snapshot);
}

std::vector<ProfilingSnapshot> Telemetry::getRecentSnapshots(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count >= snapshots_.size()) return snapshots_;
    return std::vector<ProfilingSnapshot>(snapshots_.end() - count, snapshots_.end());
}

double Telemetry::averageInferenceLatency() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& s : snapshots_) sum += s.inference_latency_ms;
    return sum / snapshots_.size();
}

double Telemetry::averageRetrievalLatency() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& s : snapshots_) sum += s.retrieval_latency_ms;
    return sum / snapshots_.size();
}

double Telemetry::averageTokensPerSecond() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& s : snapshots_) sum += s.tokens_per_second;
    return sum / snapshots_.size();
}

double Telemetry::peakRamUsageMB() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.empty()) return 0.0;
    double peak = 0.0;
    for (const auto& s : snapshots_) peak = std::max(peak, static_cast<double>(s.ram_used_mb));
    return peak;
}

size_t Telemetry::snapshotCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshots_.size();
}

void Telemetry::dumpSummary() const {
    ISHA_LOG_INFO("Telemetry", "=== TELEMETRY SUMMARY ===");
    ISHA_LOG_INFO("Telemetry", "Snapshots: " + std::to_string(snapshotCount()));
    ISHA_LOG_INFO("Telemetry", "Avg inference latency: " + std::to_string(averageInferenceLatency()) + "ms");
    ISHA_LOG_INFO("Telemetry", "Avg retrieval latency: " + std::to_string(averageRetrievalLatency()) + "ms");
    ISHA_LOG_INFO("Telemetry", "Avg tokens/sec: " + std::to_string(averageTokensPerSecond()));
    ISHA_LOG_INFO("Telemetry", "Peak RAM: " + std::to_string(peakRamUsageMB()) + "MB");
}

void Telemetry::writeToFile(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(path);
    if (!file.is_open()) {
        ISHA_LOG_ERROR("Telemetry", "Failed to write telemetry to: " + path);
        return;
    }
    
    file << "timestamp,ram_used_mb,inference_ms,retrieval_ms,tokens_per_sec,temperature_c,battery_pct,queue_depth\n";
    for (const auto& s : snapshots_) {
        auto time_t_val = std::chrono::system_clock::to_time_t(s.timestamp);
        struct tm time_info;
#if defined(_WIN32)
        localtime_s(&time_info, &time_t_val);
#else
        localtime_r(&time_t_val, &time_info);
#endif
        char time_buf[64];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", &time_info);
        
        file << time_buf << ","
             << s.ram_used_mb << ","
             << std::fixed << std::setprecision(2) << s.inference_latency_ms << ","
             << s.retrieval_latency_ms << ","
             << s.tokens_per_second << ","
             << s.cpu_temperature_c << ","
             << s.battery_percent << ","
             << s.scheduler_queue_depth << "\n";
    }
    
    ISHA_LOG_INFO("Telemetry", "Wrote " + std::to_string(snapshots_.size()) + " snapshots to " + path);
}

double Telemetry::getP50InferenceLatency() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.empty()) return 0.0;
    std::vector<double> lats;
    for (const auto& s : snapshots_) lats.push_back(s.inference_latency_ms);
    std::sort(lats.begin(), lats.end());
    return lats[lats.size() / 2];
}

double Telemetry::getP95InferenceLatency() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.empty()) return 0.0;
    std::vector<double> lats;
    for (const auto& s : snapshots_) lats.push_back(s.inference_latency_ms);
    std::sort(lats.begin(), lats.end());
    size_t idx = static_cast<size_t>(lats.size() * 0.95);
    if (idx >= lats.size()) idx = lats.size() - 1;
    return lats[idx];
}

double Telemetry::getP99InferenceLatency() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.empty()) return 0.0;
    std::vector<double> lats;
    for (const auto& s : snapshots_) lats.push_back(s.inference_latency_ms);
    std::sort(lats.begin(), lats.end());
    size_t idx = static_cast<size_t>(lats.size() * 0.99);
    if (idx >= lats.size()) idx = lats.size() - 1;
    return lats[idx];
}

double Telemetry::getJitterVariance() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.size() < 2) return 0.0;
    std::vector<double> diffs;
    for (size_t i = 1; i < snapshots_.size(); ++i) {
        diffs.push_back(std::abs(snapshots_[i].inference_latency_ms - snapshots_[i - 1].inference_latency_ms));
    }
    double sum = std::accumulate(diffs.begin(), diffs.end(), 0.0);
    double avg = sum / diffs.size();
    double sq_sum = 0.0;
    for (double d : diffs) {
        sq_sum += (d - avg) * (d - avg);
    }
    return sq_sum / diffs.size();
}

double Telemetry::getQueueWaitVariance() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.empty()) return 0.0;
    std::vector<double> waits;
    for (const auto& s : snapshots_) waits.push_back(s.scheduler_latency_ms);
    double sum = std::accumulate(waits.begin(), waits.end(), 0.0);
    double avg = sum / waits.size();
    double sq_sum = 0.0;
    for (double w : waits) {
        sq_sum += (w - avg) * (w - avg);
    }
    return sq_sum / waits.size();
}

double Telemetry::getAllocationChurn() const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Allocation churn modeled as variance of RAM usage indicating dynamic heap allocations
    if (snapshots_.size() < 2) return 0.0;
    double sum = 0.0;
    for (const auto& s : snapshots_) sum += s.ram_used_mb;
    double avg = sum / snapshots_.size();
    double sq_sum = 0.0;
    for (const auto& s : snapshots_) {
        sq_sum += (s.ram_used_mb - avg) * (s.ram_used_mb - avg);
    }
    return sq_sum / snapshots_.size();
}

double Telemetry::getMmapReuseEfficiency() const {
    // 1.0 represents perfect reuse (no re-mappings), lower represents remap thrashing
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.empty()) return 1.0;
    unsigned int loads = 0;
    for (const auto& s : snapshots_) {
        if (s.model_loaded) loads++;
    }
    if (loads == 0) return 1.0;
    return 1.0 / static_cast<double>(loads);
}

double Telemetry::getFragmentationRatio() const {
    // Represents fragmentation growth index (derived from ratio of current RAM overhead vs active mappings)
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshots_.empty()) return 0.0;
    double ratio_sum = 0.0;
    for (const auto& s : snapshots_) {
        if (s.model_mapped_mb > 0) {
            ratio_sum += static_cast<double>(s.ram_used_mb) / s.model_mapped_mb;
        } else {
            ratio_sum += 0.1;
        }
    }
    return ratio_sum / snapshots_.size();
}

} // namespace isha
