#ifndef ISHA_AI_PROFILING_SNAPSHOT_HPP
#define ISHA_AI_PROFILING_SNAPSHOT_HPP

#include <string>
#include <chrono>

namespace isha {

struct ProfilingSnapshot {
    // Timestamp
    std::chrono::system_clock::time_point timestamp;
    
    // Memory metrics
    unsigned int ram_used_mb = 0;
    unsigned int ram_available_mb = 0;
    unsigned int kv_cache_used_mb = 0;
    unsigned int model_mapped_mb = 0;
    
    // Latency metrics (milliseconds)
    double inference_latency_ms = 0.0;
    double retrieval_latency_ms = 0.0;
    double scheduler_latency_ms = 0.0;
    double context_assembly_ms = 0.0;
    
    // Throughput
    unsigned int tokens_generated = 0;
    double tokens_per_second = 0.0;
    
    // Health
    float cpu_temperature_c = 0.0f;
    float battery_percent = 100.0f;
    unsigned int active_sessions = 0;
    unsigned int watchdog_timeouts = 0;
    unsigned int cancellations = 0;
    unsigned int scheduler_queue_depth = 0;
    
    // State
    std::string active_model;
    bool model_loaded = false;
};

} // namespace isha

#endif // ISHA_AI_PROFILING_SNAPSHOT_HPP
