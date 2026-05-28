#ifndef ISHA_AI_TOKENIZER_MANAGER_HPP
#define ISHA_AI_TOKENIZER_MANAGER_HPP

#include <string>
#include <map>
#include <mutex>
#include <atomic>
#include <chrono>

namespace isha {

struct TokenizerInfo {
    std::string model_id;
    std::string tokenizer_path;
    unsigned int vocab_size = 0;
    unsigned int memory_usage_kb = 0;
    bool is_loaded = false;
    std::chrono::steady_clock::time_point loaded_at;
};

class TokenizerManager {
public:
    explicit TokenizerManager(unsigned int max_memory_kb = 8192);
    ~TokenizerManager() = default;

    // Load tokenizer for a model (from GGUF metadata)
    bool loadTokenizer(const std::string& model_id, const std::string& model_path);

    // Unload tokenizer
    void unloadTokenizer(const std::string& model_id);

    // Check if tokenizer is cached for model
    bool hasTokenizer(const std::string& model_id) const;

    // Get cached tokenizer info
    const TokenizerInfo* getTokenizerInfo(const std::string& model_id) const;

    // Check if tokenizer is compatible with model
    bool isCompatible(const std::string& model_id, const std::string& model_path) const;

    // Memory governance
    unsigned int totalMemoryUsageKB() const;
    bool wouldExceedBudget(unsigned int additional_kb) const;

    // Eviction
    void evictLeastRecentlyUsed();
    void evictAll();

    // Stats
    unsigned int loadCount() const { return load_count_.load(); }
    unsigned int evictionCount() const { return eviction_count_.load(); }
    unsigned int duplicationsPrevented() const { return duplications_prevented_.load(); }

private:
    unsigned int max_memory_kb_;
    std::map<std::string, TokenizerInfo> tokenizers_;

    std::atomic<unsigned int> load_count_{0};
    std::atomic<unsigned int> eviction_count_{0};
    std::atomic<unsigned int> duplications_prevented_{0};

    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_TOKENIZER_MANAGER_HPP
