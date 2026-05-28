#ifndef ISHA_AI_RUNTIME_WATCHDOG_HPP
#define ISHA_AI_RUNTIME_WATCHDOG_HPP

#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include "../inference/cancellation_token.hpp"

namespace isha {

struct WatchedTask {
    std::string task_id;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::milliseconds timeout;
    std::shared_ptr<CancellationToken> cancel_token;
};

struct WatchedAudioPipeline {
    std::string stream_id;
    std::chrono::steady_clock::time_point last_progress_time;
    std::chrono::milliseconds timeout;
};

class RuntimeWatchdog {
public:
    explicit RuntimeWatchdog(std::chrono::milliseconds check_interval = std::chrono::milliseconds(100));
    ~RuntimeWatchdog();

    void start();
    void stop();
    
    // Register a task to be monitored
    void registerTask(const std::string& task_id, std::chrono::milliseconds timeout, std::shared_ptr<CancellationToken> cancel_token);
    
    // Unregister a completed task
    void unregisterTask(const std::string& task_id);
    
    // Register an audio pipeline to be monitored
    void registerAudioPipeline(const std::string& stream_id, std::chrono::milliseconds timeout);
    void unregisterAudioPipeline(const std::string& stream_id);
    void feedAudioPipelineProgress(const std::string& stream_id);

    // Get count of active watched tasks
    size_t activeTaskCount() const;
    size_t activeAudioPipelineCount() const;
    
    // Get count of total timeouts triggered
    unsigned int totalTimeouts() const { return timeout_count_.load(); }
    
    bool isRunning() const { return is_running_.load(); }

private:
    void watchLoop();
    
    std::chrono::milliseconds check_interval_;
    std::map<std::string, WatchedTask> watched_tasks_;
    std::map<std::string, WatchedAudioPipeline> watched_pipelines_;
    mutable std::mutex mutex_;
    std::thread watch_thread_;
    std::atomic<bool> is_running_;
    std::atomic<unsigned int> timeout_count_;
};

} // namespace isha

#endif // ISHA_AI_RUNTIME_WATCHDOG_HPP
