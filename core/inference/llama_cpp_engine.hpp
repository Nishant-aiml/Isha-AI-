#ifndef ISHA_AI_LLAMA_CPP_ENGINE_HPP
#define ISHA_AI_LLAMA_CPP_ENGINE_HPP

#include "inference_engine.hpp"
#include "inference_context.hpp"
#include "inference_thread_policy.hpp"
#include "decode_governor.hpp"
#include "sampling_config.hpp"
#include "output_sanitizer.hpp"
#include "kv_telemetry.hpp"
#include "latency_tracker.hpp"
#include "acceleration_backend.hpp"
#include "../config/device_profile.hpp"
#include <string>
#include <mutex>
#include <atomic>
#include <functional>

// Forward declarations from llama.h
struct llama_model;
struct llama_context;
struct llama_vocab;
struct llama_sampler;

namespace isha {

using StreamTokenCallback = std::function<bool(const std::string& token)>;

struct LlamaEngineConfig {
    int n_ctx = 512;           // Context window — STRICT 512 limit
    int n_gpu_layers = 0;      // GPU layers — STRICT 0 (CPU-only)
    int n_threads = 4;         // Will be overridden by InferenceThreadPolicy
    int n_batch = 0;           // 0 = auto-select from tier + model size
    bool use_mmap = true;      // Memory-mapped model loading
    bool verbose = false;      // llama.cpp logging
};

struct GenerationResult {
    bool success = false;
    std::string output;
    std::string error;
    uint32_t tokens_generated = 0;
    float first_token_ms = 0.0f;
    float total_ms = 0.0f;
    float tokens_per_second = 0.0f;
    bool was_cancelled = false;
    bool was_sanitizer_stopped = false;
    std::string sanitizer_reason;
    float prefill_ms = 0.0f;
    uint32_t prefill_chunks = 0;
};

class LlamaCppEngine : public IInferenceEngine {
public:
    explicit LlamaCppEngine(const DeviceProfile& profile);
    ~LlamaCppEngine() override;

    // IInferenceEngine interface
    bool load(const std::string& model_path, const InferenceContext& context) override;
    void unload() override;
    std::string generate(const std::string& prompt, const InferenceContext& ctx) override;
    bool stream(const std::string& prompt, const InferenceContext& context, IInferenceEngine::TokenCallback callback) override;
    unsigned int memoryUsageMB() const override;
    bool isLoaded() const override;

    // Extended API (real load/unload with full control)
    bool loadModel(const std::string& path);
    void unloadModel();
    bool isModelLoaded() const;

    // Extended API
    GenerationResult generateEx(const std::string& prompt, const InferenceContext& ctx);
    GenerationResult streamEx(const std::string& prompt, const InferenceContext& ctx, StreamTokenCallback callback);

    // Configuration
    void setConfig(const LlamaEngineConfig& config);
    void setSamplingConfig(const SamplingConfig& sampling);
    void setTemperature(double temperature_c);
    void setEmbeddingSizeOverride(int embd) { embd_override_ = embd; }

    // Governance accessors
    DecodeGovernor& decodeGovernor() { return decode_governor_; }
    OutputSanitizer& outputSanitizer() { return sanitizer_; }
    KVTelemetry& kvTelemetry() { return kv_telemetry_; }
    LatencyTracker& latencyTracker() { return latency_tracker_; }
    const DecodeGovernor& decodeGovernor() const { return decode_governor_; }
    const LatencyTracker& latencyTracker() const { return latency_tracker_; }

    // Acceleration Support
    void setAccelerationConfig(const AccelerationConfig& config);
    AccelerationBackend activeBackend() const;
    bool fallbackToCPU();

    // Model info
    int vocabSize() const;
    int contextSize() const;
    int embeddingSize() const;
    std::string modelName() const { return model_path_; }

    enum class ModelSize { UNKNOWN, SMALL, MEDIUM, LARGE };
    ModelSize detectModelSize() const;
    bool canLoadOnTier() const;

    // Telemetry
    uint32_t totalGenerations() const { return total_generations_.load(std::memory_order_relaxed); }
    uint32_t failedGenerations() const { return failed_generations_.load(std::memory_order_relaxed); }
    uint32_t cancelledGenerations() const { return cancelled_generations_.load(std::memory_order_relaxed); }

private:
    // Core llama.cpp state
    llama_model* model_ = nullptr;
    llama_context* ctx_ = nullptr;
    const llama_vocab* vocab_ = nullptr;

    // Configuration
    DeviceProfile profile_;
    LlamaEngineConfig engine_config_;
    SamplingConfig sampling_config_;
    double current_temperature_c_ = 25.0;
    std::string model_path_;

    // Governance components
    DecodeGovernor decode_governor_;
    OutputSanitizer sanitizer_;
    KVTelemetry kv_telemetry_;
    LatencyTracker latency_tracker_;

    // Thread safety
    mutable std::mutex engine_mutex_;

    // Acceleration Members
    AccelerationConfig acceleration_config_;
    AccelerationCapability cached_capability_;

    // Telemetry
    std::atomic<uint32_t> total_generations_{0};
    std::atomic<uint32_t> failed_generations_{0};
    std::atomic<uint32_t> cancelled_generations_{0};
    std::atomic<bool> model_loaded_{false};
    std::atomic<bool> abort_request_{false};
    int active_n_batch_ = 512;
    int embd_override_ = 0;

    // Internal helpers
    bool tokenize(const std::string& text, std::vector<int32_t>& tokens, bool add_bos) const;
    std::string detokenize(int32_t token) const;
    llama_sampler* createSampler() const;
    int computeThreadCount() const;
    int computeBatchSize(int model_embd_size) const;
    void cleanupContext();
    void sampleRSSPeak();
    void samplePhase(KVTelemetry::Phase phase);
};

} // namespace isha

#endif // ISHA_AI_LLAMA_CPP_ENGINE_HPP
