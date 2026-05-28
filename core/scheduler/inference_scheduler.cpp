#include "inference_scheduler.hpp"
#include "../logging/logger.hpp"

namespace isha {

InferenceScheduler::InferenceScheduler(size_t max_queue_size)
    : max_queue_size_(max_queue_size), is_running_(false) {}

InferenceScheduler::~InferenceScheduler() {
    stop();
}

void InferenceScheduler::start() {
    if (is_running_) return;
    is_running_ = true;
    worker_thread_ = std::thread(&InferenceScheduler::workerLoop, this);
    ISHA_LOG_INFO("Scheduler", "Started inference scheduler worker thread.");
}

void InferenceScheduler::stop() {
    if (!is_running_) return;
    is_running_ = false;
    cv_.notify_all();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        auto task = queue_.top();
        task->state = TaskState::FAILED;
        queue_.pop();
    }
    active_tasks_.clear();
    ISHA_LOG_INFO("Scheduler", "Stopped inference scheduler.");
}

bool InferenceScheduler::enqueueTask(std::shared_ptr<SchedulerTask> task) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Backpressure: Overload rejection when queue limits are reached
    if (queue_.size() >= max_queue_size_) {
        ISHA_LOG_ERROR("Scheduler", "Queue overloaded. Rejecting task: " + task->task_id);
        task->state = TaskState::FAILED;
        return false;
    }

    task->state = TaskState::QUEUED;
    queue_.push(task);
    active_tasks_.push_back(task);
    
    ISHA_LOG_INFO("Scheduler", "Enqueued task: " + task->task_id + " (Priority: " 
                  + std::to_string(task->priority) + "). Queue size: " + std::to_string(queue_.size()));
    
    cv_.notify_one();
    return true;
}

void InferenceScheduler::cancelTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& task : active_tasks_) {
        if (task->task_id == task_id) {
            if (task->state == TaskState::QUEUED || task->state == TaskState::RUNNING) {
                task->state = TaskState::CANCELLED;
                if (task->context.cancel_token) {
                    task->context.cancel_token->cancel();
                }
                ISHA_LOG_INFO("Scheduler", "Requested cancellation for task: " + task_id);
            }
            break;
        }
    }
}

void InferenceScheduler::workerLoop() {
    while (is_running_) {
        std::this_thread::yield(); // Micro-yield scheduler yield checkpoint
        std::shared_ptr<SchedulerTask> task = nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return !queue_.empty() || !is_running_; });

            if (!is_running_) break;

            // Starvation/aging check: if oldest task in active list is waiting too long (> 2000ms), 
            // we override queue selection and execute it immediately to prevent starving under thermal pressure
            auto now = std::chrono::system_clock::now();
            size_t starv_idx = active_tasks_.size();
            for (size_t i = 0; i < active_tasks_.size(); ++i) {
                if (active_tasks_[i]->state == TaskState::QUEUED) {
                    auto wait_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - active_tasks_[i]->queued_at).count();
                    if (wait_ms > 2000) {
                        starv_idx = i;
                        break;
                    }
                }
            }

            if (starv_idx < active_tasks_.size()) {
                task = active_tasks_[starv_idx];
                // We must pop this specific task from the priority queue
                // Since std::priority_queue doesn't support random access erase, we pop the top, 
                // and if it's not the chosen aged task, we keep it aside and push it back after.
                std::vector<std::shared_ptr<SchedulerTask>> temp;
                while (!queue_.empty()) {
                    auto t = queue_.top();
                    queue_.pop();
                    if (t->task_id == task->task_id) {
                        break;
                    }
                    temp.push_back(t);
                }
                for (auto& t : temp) {
                    queue_.push(t);
                }
            } else {
                task = queue_.top();
                queue_.pop();
            }
        }

        if (!task) continue;

        // Verify task is not cancelled before running
        if (task->state == TaskState::CANCELLED || (task->context.cancel_token && task->context.cancel_token->isCancelled())) {
            ISHA_LOG_INFO("Scheduler", "Task " + task->task_id + " cancelled prior to execution.");
            continue;
        }

        task->state = TaskState::RUNNING;
        ISHA_LOG_INFO("Scheduler", "Executing task: " + task->task_id);

        try {
            if (task->run_fn) {
                task->run_fn();
            }
            if (task->state == TaskState::RUNNING) {
                task->state = TaskState::COMPLETED;
                ISHA_LOG_INFO("Scheduler", "Completed task: " + task->task_id);
            }
        } catch (const std::exception& e) {
            task->state = TaskState::FAILED;
            ISHA_LOG_ERROR("Scheduler", "Task failed: " + task->task_id + " with error: " + e.what());
        }

        // Cleanup task tracker
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto it = active_tasks_.begin(); it != active_tasks_.end(); ++it) {
                if ((*it)->task_id == task->task_id) {
                    active_tasks_.erase(it);
                    break;
                }
            }
        }
    }
}

} // namespace isha
