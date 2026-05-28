#include "kv_cache_manager.hpp"
#include "../logging/logger.hpp"
#include "../runtime/event_bus.hpp"

namespace isha {

KVCacheManager::KVCacheManager(unsigned int soft_limit_mb, unsigned int hard_limit_mb)
    : soft_limit_mb_(soft_limit_mb), hard_limit_mb_(hard_limit_mb), total_allocated_mb_(0) {}

bool KVCacheManager::allocateCache(const std::string& session_id, unsigned int size_mb) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Overflow protection: Hard limit check
    if (total_allocated_mb_ + size_mb > hard_limit_mb_) {
        ISHA_LOG_ERROR("KVCache", "KV Cache allocation rejected for session " + session_id 
                       + ". Requested: " + std::to_string(size_mb) + "MB. Cache size (" 
                       + std::to_string(total_allocated_mb_ + size_mb) + "MB) exceeds hard limit: " 
                       + std::to_string(hard_limit_mb_) + "MB");
        return false;
    }

    // Soft limit eviction policy check
    if (total_allocated_mb_ + size_mb > soft_limit_mb_) {
        ISHA_LOG_WARN("KVCache", "KV Cache soft limit exceeded. Triggering LRU eviction.");
        
        // Eviction policy: Evict the first allocation found to make room
        if (!allocations_.empty()) {
            auto first_it = allocations_.begin();
            std::string evict_session = first_it->first;
            unsigned int evict_size = first_it->second;
            
            total_allocated_mb_ -= evict_size;
            allocations_.erase(first_it);
            
            ISHA_LOG_INFO("KVCache", "Evicted KV cache of session " + evict_session + " (" + std::to_string(evict_size) + "MB)");
            EventBus::getInstance().publish({EventType::SESSION_CLEANED, "KVCacheManager", evict_session});
        }
    }

    allocations_[session_id] += size_mb;
    total_allocated_mb_ += size_mb;
    
    ISHA_LOG_INFO("KVCache", "Allocated " + std::to_string(size_mb) + "MB KV cache for session " 
                  + session_id + ". Total allocation: " + std::to_string(total_allocated_mb_) + "MB");
    return true;
}

void KVCacheManager::releaseCache(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = allocations_.find(session_id);
    if (it != allocations_.end()) {
        unsigned int size = it->second;
        total_allocated_mb_ -= size;
        allocations_.erase(it);
        ISHA_LOG_INFO("KVCache", "Released " + std::to_string(size) + "MB KV cache for session " + session_id);
    }
}

void KVCacheManager::clearAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    allocations_.clear();
    total_allocated_mb_ = 0;
    ISHA_LOG_INFO("KVCache", "Cleared all KV cache allocations.");
}

} // namespace isha
