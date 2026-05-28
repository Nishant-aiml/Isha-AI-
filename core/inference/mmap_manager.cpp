#include "mmap_manager.hpp"
#include "../logging/logger.hpp"
#include <chrono>
#include <thread>

#if !defined(_WIN32)
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace isha {

MmapManager::MmapManager()
    : state_(MappingLifecycleState::UNMAPPED)
#if defined(_WIN32)
    , file_handle_(INVALID_HANDLE_VALUE)
    , mapping_handle_(NULL)
#else
    , fd_(-1)
#endif
{
    // Initialize timestamps to past
    last_mapped_time_ = std::chrono::steady_clock::now() - std::chrono::hours(1);
    last_unmapped_time_ = std::chrono::steady_clock::now() - std::chrono::hours(1);
}

MmapManager::~MmapManager() {
    unmapFile();
}

bool MmapManager::mapFile(const std::string& path) {
    auto now = std::chrono::steady_clock::now();
    
    // Cooldown verification to avoid map-unmap thrashing
    auto time_since_unmap = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_unmapped_time_);
    if (time_since_unmap < cooldown_ms_) {
        ISHA_LOG_WARN("MmapManager", "Mapping cooldown active. Delaying map request.");
        std::this_thread::sleep_for(cooldown_ms_ - time_since_unmap);
    }

    if (state_ == MappingLifecycleState::MAPPED) {
        ISHA_LOG_WARN("MmapManager", "File already memory-mapped. Unmapping first.");
        unmapFile();
    }

#if defined(_WIN32)
    file_handle_ = CreateFileA(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file_handle_ == INVALID_HANDLE_VALUE) {
        ISHA_LOG_ERROR("MmapManager", "Failed to open file for mmap: " + path + " (ErrorCode: " + std::to_string(GetLastError()) + ")");
        state_ = MappingLifecycleState::FAILED;
        return false;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle_, &file_size)) {
        ISHA_LOG_ERROR("MmapManager", "Failed to get file size for mmap: " + path);
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        state_ = MappingLifecycleState::FAILED;
        return false;
    }

    region_.length = static_cast<size_t>(file_size.QuadPart);

    mapping_handle_ = CreateFileMappingA(
        file_handle_,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
    );

    if (mapping_handle_ == NULL) {
        ISHA_LOG_ERROR("MmapManager", "Failed to create file mapping handle: " + path + " (ErrorCode: " + std::to_string(GetLastError()) + ")");
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        state_ = MappingLifecycleState::FAILED;
        return false;
    }

    region_.address = MapViewOfFile(
        mapping_handle_,
        FILE_MAP_READ,
        0,
        0,
        0
    );

    if (region_.address == nullptr) {
        ISHA_LOG_ERROR("MmapManager", "Failed to map view of file: " + path + " (ErrorCode: " + std::to_string(GetLastError()) + ")");
        CloseHandle(mapping_handle_);
        CloseHandle(file_handle_);
        mapping_handle_ = NULL;
        file_handle_ = INVALID_HANDLE_VALUE;
        state_ = MappingLifecycleState::FAILED;
        return false;
    }

#else
    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        ISHA_LOG_ERROR("MmapManager", "Failed to open file for mmap: " + path);
        state_ = MappingLifecycleState::FAILED;
        return false;
    }

    struct stat sb;
    if (fstat(fd_, &sb) == -1) {
        ISHA_LOG_ERROR("MmapManager", "Failed to get file size for mmap: " + path);
        close(fd_);
        fd_ = -1;
        state_ = MappingLifecycleState::FAILED;
        return false;
    }

    region_.length = static_cast<size_t>(sb.st_size);

    region_.address = mmap(
        NULL,
        region_.length,
        PROT_READ,
        MAP_PRIVATE,
        fd_,
        0
    );

    if (region_.address == MAP_FAILED) {
        ISHA_LOG_ERROR("MmapManager", "Failed to map view of file (mmap): " + path);
        close(fd_);
        fd_ = -1;
        region_.address = nullptr;
        state_ = MappingLifecycleState::FAILED;
        return false;
    }
#endif

    state_ = MappingLifecycleState::MAPPED;
    last_mapped_time_ = std::chrono::steady_clock::now();
    ISHA_LOG_INFO("MmapManager", "Successfully mapped file: " + path + " (" + std::to_string(region_.length) + " bytes)");
    return true;
}

void MmapManager::unmapFile() {
    if (state_ != MappingLifecycleState::MAPPED) return;

    // Pinning protection check
    if (active_pinned_) {
        ISHA_LOG_WARN("MmapManager", "Unmap requested but region is pinned. Preserving active mapping.");
        return;
    }

    // Residency safety window check to prevent remap-unmap thrashing
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_mapped_time_);
    if (elapsed < min_residency_ms_) {
        ISHA_LOG_WARN("MmapManager", "Residency constraint active. Throttling unmap.");
        std::this_thread::sleep_for(min_residency_ms_ - elapsed);
    }

    last_unmapped_time_ = std::chrono::steady_clock::now();

#if defined(_WIN32)
    if (region_.address != nullptr) {
        UnmapViewOfFile(region_.address);
        region_.address = nullptr;
    }
    if (mapping_handle_ != NULL) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = NULL;
    }
    if (file_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (region_.address != nullptr) {
        munmap(region_.address, region_.length);
        region_.address = nullptr;
    }
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
#endif

    region_.length = 0;
    state_ = MappingLifecycleState::UNMAPPED;
    ISHA_LOG_INFO("MmapManager", "Successfully unmapped file.");
}

} // namespace isha
