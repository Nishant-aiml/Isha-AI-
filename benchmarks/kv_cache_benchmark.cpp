#include "inference/kv_cache_manager.hpp"
#include "logging/logger.hpp"
#include <iostream>
#include <cassert>

int main() {
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::DEBUG);
    ISHA_LOG_INFO("Benchmark", "=== STARTING KV CACHE BENCHMARK ===");

    // Soft limit 20MB, Hard limit 50MB
    isha::KVCacheManager manager(20, 50);

    // 1. Allocate within soft limits
    bool alloc1 = manager.allocateCache("session_A", 15);
    assert(alloc1);
    assert(manager.getTotalAllocationMB() == 15);

    // 2. Allocate exceeding soft limit but within hard limit (triggers LRU eviction)
    // Needs to allocate 10MB (Total: 25MB > 20MB soft limit).
    // Evicts session_A (size 15), leaving session_B (size 10). Total allocated: 10MB.
    bool alloc2 = manager.allocateCache("session_B", 10);
    assert(alloc2);
    assert(manager.getTotalAllocationMB() == 10);

    // 3. Overflow protection: Exceeding hard limit (50MB)
    bool alloc3 = manager.allocateCache("session_C", 45); // 10 + 45 = 55MB > 50MB hard limit
    assert(!alloc3); // Rejects allocation

    // 4. Release allocations
    manager.releaseCache("session_B");
    assert(manager.getTotalAllocationMB() == 0);

    ISHA_LOG_INFO("Benchmark", "=== KV CACHE BENCHMARK COMPLETED SUCCESSFULLY ===");
    return 0;
}
