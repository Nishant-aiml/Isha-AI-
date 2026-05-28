#ifndef ISHA_AI_INFERENCE_SCHEDULER_HPP
#define ISHA_AI_INFERENCE_SCHEDULER_HPP

#include <string>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include "../inference/inference_context.hpp"

namespace isha {

enum class TaskState {
    QUEUED,
    RUNNING,
    CANCELLED,
    FAILED,
    COMPLETED
};

inline std::string taskStateToString(TaskState state) {
    switch (state) {
        case TaskState::QUEUED: return "QUEUED";
        case TaskState::RUNNING: return "RUNNING";
        case TaskState::CANCELLED: return "CANCELLED";
        case TaskState::FAILED: return "FAILED";
        case TaskState::COMPLETED: return "COMPLETED";
        default: return "UNKNOWN";
    }
}

struct SchedulerTask {
    std::string task_id;
    std::string prompt;
    int priority = 0; // Higher values = higher priority
    std::chrono::system_clock::time_point queued_at;
    TaskState state = TaskState::QUEUED;
    InferenceContext context;
    std::function<void()> run_fn;
};

// Custom comparator for priority queue with aging support
struct TaskComparator {
    bool operator()(const std::shared_ptr<SchedulerTask>& lhs, const std::shared_ptr<SchedulerTask>& rhs) {
        // Starvation prevention/aging: Adjust priority based on queue wait time
        auto now = std::chrono::system_clock::now();
        auto wait_lhs = std::chrono::duration_cast<std::chrono::seconds>(now - lhs->queued_at).count();
        auto wait_rhs = std::chrono::duration_cast<std::chrono::seconds>(now - rhs->queued_at).count();

        // Every 5 seconds of wait time boosts priority by 1
        double priority_lhs = lhs->priority + (wait_lhs / 5.0);
        double priority_rhs = rhs->priority + (wait_rhs / 5.0);

        return priority_lhs < priority_rhs;
    }
};

class InferenceScheduler {
public:
    InferenceScheduler(size_t max_queue_size = 10);
    ~InferenceScheduler();

    void start();
    void stop();

    bool enqueueTask(std::shared_ptr<SchedulerTask> task);
    void cancelTask(const std::string& task_id);

    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    void workerLoop();

    size_t max_queue_size_;
    std::priority_queue<std::shared_ptr<SchedulerTask>, std::vector<std::shared_ptr<SchedulerTask>>, TaskComparator> queue_;
    std::vector<std::shared_ptr<SchedulerTask>> active_tasks_;

    std::thread worker_thread_;
    std::atomic<bool> is_running_;
    
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace isha

#endif // ISHA_AI_INFERENCE_SCHEDULER_HPP
