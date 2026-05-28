#include "audio_buffer_manager.hpp"

namespace isha {

AudioBufferManager::AudioBufferManager(size_t max_memory_bytes, size_t max_queued_frames)
    : max_memory_bytes_(max_memory_bytes), max_queued_frames_(max_queued_frames) {}

bool AudioBufferManager::pushChunk(const AudioChunk& chunk) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t chunk_bytes = chunk.samples.size() * sizeof(float);
    
    // Safety check: if chunk itself exceeds hard budget
    if (chunk_bytes > max_memory_bytes_) {
        return false;
    }
    
    // Evict old frames until we fit under capacity limits
    while ((current_memory_bytes_ + chunk_bytes > max_memory_bytes_ || 
            queue_.size() >= max_queued_frames_) && !queue_.empty()) {
        evictStaleFrames();
    }
    
    // Double check constraints
    if (current_memory_bytes_ + chunk_bytes > max_memory_bytes_ || 
        queue_.size() >= max_queued_frames_) {
        return false; // Budget overflow rejected
    }

    queue_.push_back(chunk);
    current_memory_bytes_ += chunk_bytes;
    return true;
}

bool AudioBufferManager::popChunk(AudioChunk& out_chunk) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    out_chunk = std::move(queue_.front());
    queue_.pop_front();
    
    size_t chunk_bytes = out_chunk.samples.size() * sizeof(float);
    if (current_memory_bytes_ >= chunk_bytes) {
        current_memory_bytes_ -= chunk_bytes;
    } else {
        current_memory_bytes_ = 0;
    }
    return true;
}

void AudioBufferManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
    current_memory_bytes_ = 0;
}

size_t AudioBufferManager::getCurrentMemoryBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_memory_bytes_;
}

size_t AudioBufferManager::getQueuedFramesCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void AudioBufferManager::evictStaleFrames() {
    // Assumed locked
    if (queue_.empty()) return;
    
    size_t front_bytes = queue_.front().samples.size() * sizeof(float);
    queue_.pop_front();
    if (current_memory_bytes_ >= front_bytes) {
        current_memory_bytes_ -= front_bytes;
    } else {
        current_memory_bytes_ = 0;
    }
}

} // namespace isha
