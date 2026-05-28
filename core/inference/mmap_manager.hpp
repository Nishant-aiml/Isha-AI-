#ifndef ISHA_AI_MMAP_MANAGER_HPP
#define ISHA_AI_MMAP_MANAGER_HPP

#include <string>
#include <memory>
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace isha {

enum class MappingLifecycleState {
    UNMAPPED,
    MAPPED,
    FAILED
};

struct MappedRegion {
    void* address = nullptr;
    size_t length = 0;
};

class MmapManager {
public:
    MmapManager();
    ~MmapManager();

    bool mapFile(const std::string& path);
    void unmapFile();

    MappingLifecycleState getState() const { return state_; }
    const MappedRegion& getRegion() const { return region_; }

    void setPinned(bool pin) { active_pinned_ = pin; }
    bool isPinned() const { return active_pinned_; }

private:
    MappingLifecycleState state_;
    MappedRegion region_;
    bool active_pinned_ = false;

    std::chrono::steady_clock::time_point last_mapped_time_;
    std::chrono::steady_clock::time_point last_unmapped_time_;

    std::chrono::milliseconds min_residency_ms_{100};
    std::chrono::milliseconds cooldown_ms_{200};

#if defined(_WIN32)
    HANDLE file_handle_;
    HANDLE mapping_handle_;
#else
    int fd_;
#endif
};

} // namespace isha

#endif // ISHA_AI_MMAP_MANAGER_HPP
