#ifndef ISHA_AI_LOCK_HIERARCHY_HPP
#define ISHA_AI_LOCK_HIERARCHY_HPP

#include <mutex>
#include <system_error>
#include <atomic>
#include <cassert>

namespace isha {

// Explicit lock levels (Lower numbers = must be acquired FIRST)
// MmapManager -> ModelManager -> InferenceScheduler -> VoiceOrchestrator -> AudioSessionManager -> AudioBufferManager -> Telemetry -> Watchdog -> Vision -> OCR
enum class LockLevel : int {
    MMAP = 1,
    MODEL = 2,
    SCHEDULER = 3,
    ORCHESTRATOR = 4,
    SESSION = 5,
    BUFFER = 6,
    TELEMETRY = 7,
    WATCHDOG = 8,
    VISION = 9,
    OCR = 10
};

// Thread-local variable to track the currently held lock level
inline thread_local int tl_current_lock_level = 0;

class HierarchicalMutex {
public:
    explicit HierarchicalMutex(LockLevel level) : level_(static_cast<int>(level)) {}

    void lock() {
        // Enforce lock acquisition hierarchy: new lock level must be GREATER than current
        if (tl_current_lock_level != 0 && level_ <= tl_current_lock_level) {
            // Hierarchy violation - potential deadlock!
            assert(false && "Lock hierarchy violation! Potential deadlock.");
            throw std::system_error(std::make_error_code(std::errc::device_or_resource_busy), "Lock hierarchy violation");
        }
        
        mutex_.lock();
        previous_level_ = tl_current_lock_level;
        tl_current_lock_level = level_;
    }

    void unlock() {
        tl_current_lock_level = previous_level_;
        mutex_.unlock();
    }

    bool try_lock() {
        if (tl_current_lock_level != 0 && level_ <= tl_current_lock_level) {
            return false;
        }
        if (mutex_.try_lock()) {
            previous_level_ = tl_current_lock_level;
            tl_current_lock_level = level_;
            return true;
        }
        return false;
    }

private:
    std::mutex mutex_;
    int level_;
    int previous_level_ = 0;
};

} // namespace isha

#endif // ISHA_AI_LOCK_HIERARCHY_HPP
