#ifndef ISHA_AI_KV_CACHE_MANAGER_HPP
#define ISHA_AI_KV_CACHE_MANAGER_HPP

#include <string>
#include <mutex>
#include <map>

namespace isha {

class KVCacheManager {
public:
    KVCacheManager(unsigned int soft_limit_mb = 128, unsigned int hard_limit_mb = 256);
    ~KVCacheManager() = default;

    bool allocateCache(const std::string& session_id, unsigned int size_mb);
    void releaseCache(const std::string& session_id);
    void clearAll();

    unsigned int getTotalAllocationMB() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return total_allocated_mb_;
    }

private:
    unsigned int soft_limit_mb_;
    unsigned int hard_limit_mb_;
    unsigned int total_allocated_mb_;
    std::map<std::string, unsigned int> allocations_;
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_KV_CACHE_MANAGER_HPP
