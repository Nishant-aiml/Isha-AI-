#include "tokenizer_manager.hpp"
#include "../logging/logger.hpp"
#include <filesystem>
#include <algorithm>

namespace isha {

TokenizerManager::TokenizerManager(unsigned int max_memory_kb)
    : max_memory_kb_(max_memory_kb) {
    ISHA_LOG_INFO("TokenizerMgr", "Initialized with " + std::to_string(max_memory_kb) + "KB budget.");
}

bool TokenizerManager::loadTokenizer(const std::string& model_id, const std::string& model_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if already cached — prevent duplication
    auto it = tokenizers_.find(model_id);
    if (it != tokenizers_.end() && it->second.is_loaded) {
        duplications_prevented_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_DEBUG("TokenizerMgr", "Tokenizer already cached for: " + model_id +
                       " (duplications_prevented=" + std::to_string(duplications_prevented_.load()) + ")");
        return true;
    }

    // Estimate tokenizer memory from model file size
    uint64_t file_size = 0;
    std::error_code ec;
    if (std::filesystem::exists(model_path, ec)) {
        file_size = std::filesystem::file_size(model_path, ec);
    }

    // Heuristic: vocab table is ~1/100 of model file, 4 bytes per token
    unsigned int estimated_vocab = static_cast<unsigned int>(
        std::min(static_cast<uint64_t>(128000), file_size / 100));
    if (estimated_vocab < 1000) estimated_vocab = 32000; // fallback default

    unsigned int estimated_memory_kb = (estimated_vocab * 4) / 1024;
    if (estimated_memory_kb < 64) estimated_memory_kb = 64; // minimum 64KB

    // Check memory budget
    unsigned int current_usage = 0;
    for (auto& [id, info] : tokenizers_) {
        if (info.is_loaded) current_usage += info.memory_usage_kb;
    }

    // Evict if needed to fit under budget
    while (current_usage + estimated_memory_kb > max_memory_kb_ && !tokenizers_.empty()) {
        ISHA_LOG_WARN("TokenizerMgr", "Budget exceeded (" +
                      std::to_string(current_usage + estimated_memory_kb) + "KB / " +
                      std::to_string(max_memory_kb_) + "KB). Evicting LRU.");

        // Find oldest loaded tokenizer
        std::string oldest_id;
        auto oldest_time = std::chrono::steady_clock::time_point::max();
        for (auto& [id, info] : tokenizers_) {
            if (info.is_loaded && info.loaded_at < oldest_time && id != model_id) {
                oldest_time = info.loaded_at;
                oldest_id = id;
            }
        }

        if (oldest_id.empty()) break; // Nothing to evict

        auto evict_it = tokenizers_.find(oldest_id);
        if (evict_it != tokenizers_.end()) {
            current_usage -= evict_it->second.memory_usage_kb;
            tokenizers_.erase(evict_it);
            eviction_count_.fetch_add(1, std::memory_order_relaxed);
            ISHA_LOG_INFO("TokenizerMgr", "Evicted tokenizer: " + oldest_id);
        }
    }

    // Final budget check
    if (current_usage + estimated_memory_kb > max_memory_kb_) {
        ISHA_LOG_ERROR("TokenizerMgr", "Cannot load tokenizer for " + model_id +
                       ": would exceed budget (" + std::to_string(estimated_memory_kb) +
                       "KB needed, " + std::to_string(max_memory_kb_ - current_usage) + "KB available)");
        return false;
    }

    // Load (simulated: tracks metadata, real load happens via llama.cpp)
    TokenizerInfo info;
    info.model_id = model_id;
    info.tokenizer_path = model_path;
    info.vocab_size = estimated_vocab;
    info.memory_usage_kb = estimated_memory_kb;
    info.is_loaded = true;
    info.loaded_at = std::chrono::steady_clock::now();

    tokenizers_[model_id] = info;
    load_count_.fetch_add(1, std::memory_order_relaxed);

    ISHA_LOG_INFO("TokenizerMgr", "Loaded tokenizer for: " + model_id +
                  " vocab=" + std::to_string(estimated_vocab) +
                  " mem=" + std::to_string(estimated_memory_kb) + "KB" +
                  " total_used=" + std::to_string(current_usage + estimated_memory_kb) + "KB");
    return true;
}

void TokenizerManager::unloadTokenizer(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tokenizers_.find(model_id);
    if (it == tokenizers_.end()) return;

    ISHA_LOG_INFO("TokenizerMgr", "Unloading tokenizer: " + model_id +
                  " (freeing " + std::to_string(it->second.memory_usage_kb) + "KB)");
    tokenizers_.erase(it);
}

bool TokenizerManager::hasTokenizer(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tokenizers_.find(model_id);
    return it != tokenizers_.end() && it->second.is_loaded;
}

const TokenizerInfo* TokenizerManager::getTokenizerInfo(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tokenizers_.find(model_id);
    if (it == tokenizers_.end()) return nullptr;
    return &it->second;
}

bool TokenizerManager::isCompatible(const std::string& model_id, const std::string& model_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tokenizers_.find(model_id);
    if (it == tokenizers_.end()) return false;
    // Basic compatibility: tokenizer was loaded from the same path
    return it->second.tokenizer_path == model_path && it->second.is_loaded;
}

unsigned int TokenizerManager::totalMemoryUsageKB() const {
    std::lock_guard<std::mutex> lock(mutex_);
    unsigned int total = 0;
    for (auto& [id, info] : tokenizers_) {
        if (info.is_loaded) total += info.memory_usage_kb;
    }
    return total;
}

bool TokenizerManager::wouldExceedBudget(unsigned int additional_kb) const {
    return totalMemoryUsageKB() + additional_kb > max_memory_kb_;
}

void TokenizerManager::evictLeastRecentlyUsed() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tokenizers_.empty()) return;

    std::string oldest_id;
    auto oldest_time = std::chrono::steady_clock::time_point::max();
    for (auto& [id, info] : tokenizers_) {
        if (info.is_loaded && info.loaded_at < oldest_time) {
            oldest_time = info.loaded_at;
            oldest_id = id;
        }
    }

    if (!oldest_id.empty()) {
        ISHA_LOG_INFO("TokenizerMgr", "LRU eviction: " + oldest_id);
        tokenizers_.erase(oldest_id);
        eviction_count_.fetch_add(1, std::memory_order_relaxed);
    }
}

void TokenizerManager::evictAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = tokenizers_.size();
    tokenizers_.clear();
    eviction_count_.fetch_add(static_cast<unsigned int>(count), std::memory_order_relaxed);
    ISHA_LOG_INFO("TokenizerMgr", "Evicted all tokenizers (" + std::to_string(count) + ")");
}

} // namespace isha
