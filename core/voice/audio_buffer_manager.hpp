#ifndef ISHA_AI_AUDIO_BUFFER_MANAGER_HPP
#define ISHA_AI_AUDIO_BUFFER_MANAGER_HPP

#include "audio_types.hpp"
#include <deque>
#include <mutex>
#include <memory>

namespace isha {

class AudioBufferManager {
public:
    AudioBufferManager(size_t max_memory_bytes = 5 * 1024 * 1024, size_t max_queued_frames = 200);

    bool pushChunk(const AudioChunk& chunk);
    bool popChunk(AudioChunk& out_chunk);
    void clear();

    size_t getCurrentMemoryBytes() const;
    size_t getQueuedFramesCount() const;
    
    size_t getMaxMemoryBytes() const { return max_memory_bytes_; }
    size_t getMaxQueuedFrames() const { return max_queued_frames_; }

private:
    void evictStaleFrames();

    size_t max_memory_bytes_;
    size_t max_queued_frames_;
    size_t current_memory_bytes_ = 0;

    std::deque<AudioChunk> queue_;
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_AUDIO_BUFFER_MANAGER_HPP
