#include "runtime_watchdog.hpp"
#include "../logging/logger.hpp"
#include "../runtime/event_bus.hpp"

namespace isha {

RuntimeWatchdog::RuntimeWatchdog(std::chrono::milliseconds check_interval)
    : check_interval_(check_interval), is_running_(false), timeout_count_(0) {
    ISHA_LOG_INFO("Watchdog", "Initialized with check interval: " + std::to_string(check_interval.count()) + "ms");
}

RuntimeWatchdog::~RuntimeWatchdog() {
    stop();
}

void RuntimeWatchdog::start() {
    if (is_running_.load()) return;
    is_running_.store(true);
    watch_thread_ = std::thread(&RuntimeWatchdog::watchLoop, this);
    ISHA_LOG_INFO("Watchdog", "Started monitoring thread.");
}

void RuntimeWatchdog::stop() {
    if (!is_running_.load()) return;
    is_running_.store(false);
    if (watch_thread_.joinable()) {
        watch_thread_.join();
    }
    ISHA_LOG_INFO("Watchdog", "Stopped monitoring thread.");
}

void RuntimeWatchdog::registerTask(const std::string& task_id, std::chrono::milliseconds timeout, std::shared_ptr<CancellationToken> cancel_token) {
    std::lock_guard<std::mutex> lock(mutex_);
    watched_tasks_[task_id] = {
        task_id,
        std::chrono::steady_clock::now(),
        timeout,
        std::move(cancel_token)
    };
    ISHA_LOG_DEBUG("Watchdog", "Registered task '" + task_id + "' with timeout " + std::to_string(timeout.count()) + "ms");
}

void RuntimeWatchdog::unregisterTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    watched_tasks_.erase(task_id);
    ISHA_LOG_DEBUG("Watchdog", "Unregistered task '" + task_id + "'");
}

void RuntimeWatchdog::registerAudioPipeline(const std::string& stream_id, std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(mutex_);
    watched_pipelines_[stream_id] = {
        stream_id,
        std::chrono::steady_clock::now(),
        timeout
    };
    ISHA_LOG_DEBUG("Watchdog", "Registered audio pipeline '" + stream_id + "' with timeout " + std::to_string(timeout.count()) + "ms");
}

void RuntimeWatchdog::unregisterAudioPipeline(const std::string& stream_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    watched_pipelines_.erase(stream_id);
    ISHA_LOG_DEBUG("Watchdog", "Unregistered audio pipeline '" + stream_id + "'");
}

void RuntimeWatchdog::feedAudioPipelineProgress(const std::string& stream_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = watched_pipelines_.find(stream_id);
    if (it != watched_pipelines_.end()) {
        it->second.last_progress_time = std::chrono::steady_clock::now();
    }
}

size_t RuntimeWatchdog::activeTaskCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return watched_tasks_.size();
}

size_t RuntimeWatchdog::activeAudioPipelineCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return watched_pipelines_.size();
}

void RuntimeWatchdog::watchLoop() {
    while (is_running_.load()) {
        std::this_thread::sleep_for(check_interval_);
        
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        
        // 1. Check watched scheduler / inference tasks
        std::vector<std::string> timed_out;
        for (auto& [id, task] : watched_tasks_) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - task.start_time);
            if (elapsed >= task.timeout) {
                ISHA_LOG_WARN("Watchdog", "Task timed out.");
                if (task.cancel_token) {
                    task.cancel_token->cancel();
                }
                timeout_count_.fetch_add(1);
                
                EventBus::getInstance().publish({EventType::WATCHDOG_ALERT, "RuntimeWatchdog",
                    "task=" + id + ",elapsed=" + std::to_string(elapsed.count()) + "ms"});
                EventBus::getInstance().publish({EventType::INFERENCE_TIMEOUT, "RuntimeWatchdog",
                    "task=" + id});
                
                timed_out.push_back(id);
            }
        }
        for (const auto& id : timed_out) {
            watched_tasks_.erase(id);
        }

        // 2. Check watched audio pipelines for silence or microphone stalls
        std::vector<std::string> stalled_pipelines;
        for (auto& [id, pipeline] : watched_pipelines_) {
            auto inactive_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - pipeline.last_progress_time);
            if (inactive_duration >= pipeline.timeout) {
                ISHA_LOG_WARN("Watchdog", "Audio pipeline stall detected on stream '" + id + "'");
                
                EventBus::getInstance().publish({EventType::AUDIO_PIPELINE_STALL, "RuntimeWatchdog",
                    "stream_id=" + id + ",inactive=" + std::to_string(inactive_duration.count()) + "ms"});
                
                stalled_pipelines.push_back(id);
            }
        }
        for (const auto& id : stalled_pipelines) {
            watched_pipelines_.erase(id);
        }
    }
}

} // namespace isha
