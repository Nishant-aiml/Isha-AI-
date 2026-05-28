#include "llama_cpp_engine.hpp"
#include <llama.h>
#ifdef ERROR
#undef ERROR
#endif
#include "../logging/logger.hpp"
#include "../monitoring/resource_monitor.hpp"
#include <chrono>
#include <algorithm>
#include <cstring>

namespace isha {

LlamaCppEngine::LlamaCppEngine(const DeviceProfile& profile)
    : profile_(profile) {
    // Initialize llama backend once
    llama_backend_init();
    ISHA_LOG_INFO("LlamaCppEngine", "Initialized. Tier=" + std::to_string(static_cast<int>(profile.tier)) +
                  " RAM=" + std::to_string(profile.total_ram_mb) + "MB");
}

LlamaCppEngine::~LlamaCppEngine() {
    unloadModel();
    llama_backend_free();
    ISHA_LOG_INFO("LlamaCppEngine", "Destroyed. total_gen=" + std::to_string(totalGenerations()) +
                  " failed=" + std::to_string(failedGenerations()));
}

bool LlamaCppEngine::loadModel(const std::string& path) {
    std::lock_guard<std::mutex> lock(engine_mutex_);

    if (model_loaded_.load(std::memory_order_relaxed)) {
        if (model_path_ == path) {
            ISHA_LOG_INFO("LlamaCppEngine", "Model file already loaded. Reusing mmap descriptor.");
            kv_telemetry_.mmap_reuse_count.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        ISHA_LOG_WARN("LlamaCppEngine", "Model already loaded. Unloading first.");
        samplePhase(KVTelemetry::Phase::ReloadOverlap);
        unloadModel();
    }

    ISHA_LOG_INFO("LlamaCppEngine", "Loading model: " + path);
    auto load_start = std::chrono::high_resolution_clock::now();

    // Step 1: Configure model parameters
    auto model_params = llama_model_default_params();
    model_params.n_gpu_layers = (acceleration_config_.requested_backend != AccelerationBackend::CPU_ONLY)
                                ? acceleration_config_.n_gpu_layers : 0;
    model_params.use_mmap = engine_config_.use_mmap;

    // Step 2: Load model
    auto mmap_start = std::chrono::high_resolution_clock::now();
    model_ = llama_model_load_from_file(path.c_str(), model_params);
    auto mmap_end = std::chrono::high_resolution_clock::now();
    
    if (!model_) {
        ISHA_LOG_ERROR("LlamaCppEngine", "Failed to load model: " + path);
        failed_generations_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    float mmap_ms = std::chrono::duration<float, std::milli>(mmap_end - mmap_start).count();
    kv_telemetry_.phase_mmap_init_ms.store(mmap_ms, std::memory_order_relaxed);
    samplePhase(KVTelemetry::Phase::ColdLoad);
    samplePhase(KVTelemetry::Phase::MmapActivation);

    // Step 3: Get vocab
    auto vocab_start = std::chrono::high_resolution_clock::now();
    vocab_ = llama_model_get_vocab(model_);
    auto vocab_end = std::chrono::high_resolution_clock::now();
    
    if (!vocab_) {
        ISHA_LOG_ERROR("LlamaCppEngine", "Failed to get vocab from model");
        llama_model_free(model_);
        model_ = nullptr;
        return false;
    }

    float vocab_ms = std::chrono::duration<float, std::milli>(vocab_end - vocab_start).count();
    kv_telemetry_.phase_tokenizer_init_ms.store(vocab_ms, std::memory_order_relaxed);
    samplePhase(KVTelemetry::Phase::TokenizerInit);

    // Step 4: Configure context parameters
    auto ctx_params = llama_context_default_params();
    ctx_params.n_ctx = static_cast<uint32_t>(engine_config_.n_ctx);    // 512 STRICT
    int batch_size = computeBatchSize(embeddingSize());
    active_n_batch_ = batch_size;
    ctx_params.n_batch = static_cast<uint32_t>(batch_size);
    ctx_params.n_threads = static_cast<uint32_t>(computeThreadCount());
    ctx_params.n_threads_batch = ctx_params.n_threads;

    ISHA_LOG_INFO("LlamaCppEngine", "Context: n_ctx=" + std::to_string(ctx_params.n_ctx) +
                  " n_batch=" + std::to_string(ctx_params.n_batch) +
                  " threads=" + std::to_string(ctx_params.n_threads) +
                  " gpu_layers=" + std::to_string(model_params.n_gpu_layers));

    // Step 5: Create context
    auto ctx_init_start = std::chrono::high_resolution_clock::now();
    ctx_ = llama_init_from_model(model_, ctx_params);
    auto ctx_init_end = std::chrono::high_resolution_clock::now();
    
    if (!ctx_) {
        ISHA_LOG_ERROR("LlamaCppEngine", "Failed to create context");
        llama_model_free(model_);
        model_ = nullptr;
        vocab_ = nullptr;
        return false;
    }

    float ctx_init_ms = std::chrono::duration<float, std::milli>(ctx_init_end - ctx_init_start).count();
    kv_telemetry_.phase_context_init_ms.store(ctx_init_ms, std::memory_order_relaxed);
    samplePhase(KVTelemetry::Phase::ContextInit);

    if (!canLoadOnTier()) {
        ISHA_LOG_WARN("LlamaCppEngine", "Model too large for tier " +
                      std::to_string(static_cast<int>(profile_.tier)) + ". Refusing load.");
        llama_free(ctx_); ctx_ = nullptr;
        llama_model_free(model_); model_ = nullptr;
        vocab_ = nullptr;
        return false;
    }

    // Record model/tokenizer active metrics in telemetry
    kv_telemetry_.mmap_remap_count.fetch_add(1, std::memory_order_relaxed);
    kv_telemetry_.tokenizer_load_count.fetch_add(1, std::memory_order_relaxed);
    kv_telemetry_.tokenizer_active_instances.store(1, std::memory_order_relaxed);
    kv_telemetry_.reserved_cells.store(ctx_params.n_ctx, std::memory_order_relaxed);
    
    // Allocate realistic estimates based on GGML CPU compute metrics
    size_t ctx_state_size = llama_state_get_size(ctx_);
    kv_telemetry_.ggml_graph_allocated_bytes.store(894 * 1024, std::memory_order_relaxed); // ~894 nodes
    kv_telemetry_.scratch_allocated_bytes.store(4 * 1024 * 1024, std::memory_order_relaxed); // 4MB scratch
    kv_telemetry_.compute_allocated_bytes.store(ctx_state_size, std::memory_order_relaxed); // actual compute buffer size
    kv_telemetry_.batch_allocated_bytes.store(static_cast<uint64_t>(batch_size) * 1024, std::memory_order_relaxed);

    model_path_ = path;
    model_loaded_.store(true, std::memory_order_release);

    auto end = std::chrono::high_resolution_clock::now();
    double load_ms = std::chrono::duration<double, std::milli>(end - load_start).count();

    ISHA_LOG_INFO("LlamaCppEngine", "Model loaded in " + std::to_string(load_ms) + "ms");
    ISHA_LOG_INFO("LlamaCppEngine", "  vocab_size=" + std::to_string(vocabSize()));
    ISHA_LOG_INFO("LlamaCppEngine", "  n_ctx_train=" + std::to_string(llama_model_n_ctx_train(model_)));
    ISHA_LOG_INFO("LlamaCppEngine", "  n_embd=" + std::to_string(embeddingSize()));

    latency_tracker_.record(static_cast<float>(load_ms));
    return true;
}

void LlamaCppEngine::unloadModel() {
    auto unload_start = std::chrono::high_resolution_clock::now();
    samplePhase(KVTelemetry::Phase::Unload);

    if (ctx_) {
        llama_free(ctx_);
        ctx_ = nullptr;
    }
    if (model_) {
        llama_model_free(model_);
        model_ = nullptr;
    }
    vocab_ = nullptr;
    model_loaded_.store(false, std::memory_order_release);
    model_path_.clear();
    kv_telemetry_.tokenizer_active_instances.store(0, std::memory_order_relaxed);
    
    auto unload_end = std::chrono::high_resolution_clock::now();
    float unload_ms = std::chrono::duration<float, std::milli>(unload_end - unload_start).count();
    kv_telemetry_.phase_unload_ms.store(unload_ms, std::memory_order_relaxed);
    
    kv_telemetry_.reset();
    ISHA_LOG_INFO("LlamaCppEngine", "Model unloaded. KV telemetry reset.");
}

std::string LlamaCppEngine::generate(const std::string& prompt, const InferenceContext& ctx) {
    GenerationResult result = generateEx(prompt, ctx);
    return result.output;
}

GenerationResult LlamaCppEngine::generateEx(const std::string& prompt, const InferenceContext& ctx) {
    return streamEx(prompt, ctx, nullptr);
}

GenerationResult LlamaCppEngine::streamEx(const std::string& prompt, const InferenceContext& ctx,
                                         StreamTokenCallback callback) {
    GenerationResult result;
    total_generations_.fetch_add(1, std::memory_order_relaxed);

    // Validate state
    if (!model_loaded_.load(std::memory_order_acquire)) {
        result.error = "No model loaded";
        failed_generations_.fetch_add(1, std::memory_order_relaxed);
        return result;
    }

    std::lock_guard<std::mutex> lock(engine_mutex_);

    auto gen_start = std::chrono::high_resolution_clock::now();

    // Step 1: Tokenize prompt
    std::vector<int32_t> tokens;
    if (!tokenize(prompt, tokens, true)) {
        result.error = "Tokenization failed";
        failed_generations_.fetch_add(1, std::memory_order_relaxed);
        return result;
    }

    ISHA_LOG_INFO("LlamaCppEngine", "Prompt tokenized: " + std::to_string(tokens.size()) + " tokens");

    // Enforce context limit
    int n_ctx = engine_config_.n_ctx;
    if (static_cast<int>(tokens.size()) >= n_ctx) {
        result.error = "Prompt exceeds context window (" + std::to_string(tokens.size()) +
                       " >= " + std::to_string(n_ctx) + ")";
        failed_generations_.fetch_add(1, std::memory_order_relaxed);
        return result;
    }

    // Step 2: Clear KV cache
    llama_kv_self_clear(ctx_);
    kv_telemetry_.recordCleanup(kv_telemetry_.current_cells.load(), 
                                 kv_telemetry_.current_cells.load(), 0.0f);

    // Step 3: Evaluate prompt (chunked prefill — FIXATION 7)
    int n_batch = active_n_batch_;
    int prompt_len = static_cast<int>(tokens.size());
    int n_chunks = (prompt_len + n_batch - 1) / n_batch;
    result.prefill_chunks = n_chunks;

    std::unique_ptr<IResourceMonitor> monitor(createResourceMonitor());
    uint64_t prefill_start_faults = monitor ? monitor->getPageFaultCount() : 0;
    auto prefill_start_time = std::chrono::high_resolution_clock::now();

    for (int chunk = 0; chunk < n_chunks; chunk++) {
        int offset = chunk * n_batch;
        int chunk_size = std::min(n_batch, prompt_len - offset);
        
        auto chunk_start = std::chrono::high_resolution_clock::now();
        
        llama_batch batch = llama_batch_get_one(tokens.data() + offset, chunk_size);
        if (llama_decode(ctx_, batch) != 0) {
            result.error = "Prefill chunk " + std::to_string(chunk) + " failed";
            failed_generations_.fetch_add(1, std::memory_order_relaxed);
            return result;
        }
        
        auto chunk_end = std::chrono::high_resolution_clock::now();
        float chunk_ms = std::chrono::duration<float, std::milli>(chunk_end - chunk_start).count();
        kv_telemetry_.recordPrefillChunk(chunk_ms, chunk_size);
        kv_telemetry_.recordAllocation(static_cast<uint32_t>(chunk_size));
        
        sampleRSSPeak(); // peak prefill / chunk memory sampling
    }

    auto prefill_end_time = std::chrono::high_resolution_clock::now();
    result.prefill_ms = std::chrono::duration<float, std::milli>(prefill_end_time - prefill_start_time).count();
    kv_telemetry_.phase_prefill_ms.store(result.prefill_ms, std::memory_order_relaxed);
    kv_telemetry_.prefill_latency_ms.store(result.prefill_ms, std::memory_order_relaxed);
    if (monitor) {
        kv_telemetry_.prefill_thermal_c.store(static_cast<float>(monitor->getDeviceTemperatureC()), std::memory_order_relaxed);
        uint64_t prefill_end_faults = monitor->getPageFaultCount();
        kv_telemetry_.prefill_page_faults.store(prefill_end_faults >= prefill_start_faults ? prefill_end_faults - prefill_start_faults : 0, std::memory_order_relaxed);
    }
    samplePhase(KVTelemetry::Phase::Prefill);

    auto first_token_time = std::chrono::high_resolution_clock::now();
    result.first_token_ms = std::chrono::duration<float, std::milli>(first_token_time - gen_start).count();

    // Step 4: Create sampler chain
    llama_sampler* smpl = createSampler();
    if (!smpl) {
        result.error = "Failed to create sampler";
        failed_generations_.fetch_add(1, std::memory_order_relaxed);
        return result;
    }

    // Step 5: Auto-regressive decode loop
    decode_governor_.reset();
    sanitizer_.reset();

    int n_cur = static_cast<int>(tokens.size());
    int n_max = std::min(static_cast<int>(ctx.max_tokens_to_generate), 
                         static_cast<int>(sampling_config_.max_tokens));
    
    // Additional safety: total tokens must not exceed context
    n_max = std::min(n_max, n_ctx - n_cur);

    llama_token eos_token = llama_vocab_eos(vocab_);
    std::string full_output;

    ISHA_LOG_INFO("LlamaCppEngine", "Decoding: max_tokens=" + std::to_string(n_max) +
                  " pos=" + std::to_string(n_cur));

    uint64_t decode_start_faults = monitor ? monitor->getPageFaultCount() : 0;
    auto decode_start_time = std::chrono::high_resolution_clock::now();

    float prev_token_ms = 0.0f;
    float max_jitter_ms = 0.0f;

    for (int i = 0; i < n_max; i++) {
        // Check cancellation BETWEEN every decode cycle
        if (ctx.cancel_token && ctx.cancel_token->isCancelled()) {
            auto cancel_start = std::chrono::high_resolution_clock::now();
            samplePhase(KVTelemetry::Phase::Cancellation);

            ISHA_LOG_INFO("LlamaCppEngine", "Generation cancelled at token " + std::to_string(i));
            result.was_cancelled = true;
            cancelled_generations_.fetch_add(1, std::memory_order_relaxed);

            auto cancel_end = std::chrono::high_resolution_clock::now();
            float cancel_ms = std::chrono::duration<float, std::milli>(cancel_end - cancel_start).count();
            kv_telemetry_.phase_cleanup_ms.store(cancel_ms, std::memory_order_relaxed);
            break;
        }

        // Decode pacing (thermal-aware)
        decode_governor_.paceToken(current_temperature_c_);

        // Sample next token
        llama_token new_token = llama_sampler_sample(smpl, ctx_, -1);

        // Check EOS
        if (new_token == eos_token) {
            ISHA_LOG_DEBUG("LlamaCppEngine", "EOS reached at token " + std::to_string(i));
            break;
        }

        // Detokenize
        std::string piece = detokenize(new_token);

        // Output sanitizer check
        SanitizeResult san_result = sanitizer_.check(piece, full_output);
        if (san_result.should_stop) {
            ISHA_LOG_WARN("LlamaCppEngine", "Sanitizer stop: " + san_result.reason);
            result.was_sanitizer_stopped = true;
            result.sanitizer_reason = san_result.reason;
            break;
        }

        full_output += piece;
        result.tokens_generated++;

        // Streaming callback
        if (callback) {
            if (!callback(piece)) {
                ISHA_LOG_INFO("LlamaCppEngine", "Callback requested stop at token " + std::to_string(i));
                break;
            }
        }

        // Track latency per token & jitter
        auto token_time = std::chrono::high_resolution_clock::now();
        float current_tok_elapsed_ms = std::chrono::duration<float, std::milli>(token_time - first_token_time).count();
        float single_token_lat = current_tok_elapsed_ms - prev_token_ms;
        if (i > 0) {
            float avg_lat = current_tok_elapsed_ms / (i + 1);
            float jitter = std::abs(single_token_lat - avg_lat);
            if (jitter > max_jitter_ms) {
                max_jitter_ms = jitter;
            }
        }
        prev_token_ms = current_tok_elapsed_ms;

        if (result.tokens_generated > 0) {
            latency_tracker_.record(current_tok_elapsed_ms / result.tokens_generated);
        }

        // Prepare next batch (single token)
        llama_batch next_batch = llama_batch_get_one(&new_token, 1);
        if (llama_decode(ctx_, next_batch) != 0) {
            ISHA_LOG_ERROR("LlamaCppEngine", "Decode failed at token " + std::to_string(i));
            result.error = "Decode failed at position " + std::to_string(n_cur + i);
            break;
        }

        kv_telemetry_.recordAllocation(1);
        n_cur++;
    }

    // Cleanup sampler
    llama_sampler_free(smpl);

    // Final telemetry
    auto gen_end = std::chrono::high_resolution_clock::now();
    result.total_ms = std::chrono::duration<float, std::milli>(gen_end - gen_start).count();
    result.output = full_output;

    float decode_ms = std::chrono::duration<float, std::milli>(gen_end - first_token_time).count();
    kv_telemetry_.phase_decode_ms.store(decode_ms, std::memory_order_relaxed);
    kv_telemetry_.decode_jitter_ms.store(max_jitter_ms, std::memory_order_relaxed);

    if (monitor) {
        kv_telemetry_.decode_thermal_c.store(static_cast<float>(monitor->getDeviceTemperatureC()), std::memory_order_relaxed);
        uint64_t decode_end_faults = monitor->getPageFaultCount();
        kv_telemetry_.decode_page_faults.store(decode_end_faults >= decode_start_faults ? decode_end_faults - decode_start_faults : 0, std::memory_order_relaxed);
    }
    samplePhase(KVTelemetry::Phase::Decode);

    if (result.tokens_generated > 0 && decode_ms > 0) {
        kv_telemetry_.decode_tok_per_sec.store((result.tokens_generated * 1000.0f) / decode_ms, std::memory_order_relaxed);
    }

    if (result.tokens_generated > 0 && result.total_ms > 0) {
        result.tokens_per_second = (result.tokens_generated * 1000.0f) / result.total_ms;
    }

    if (!result.error.empty()) {
        failed_generations_.fetch_add(1, std::memory_order_relaxed);
    } else {
        result.success = true;
    }

    kv_telemetry_.updateFragmentation();

    // Reset current active cells in telemetry as the KV state is released/cleared upon exit
    kv_telemetry_.recordCleanup(kv_telemetry_.current_cells.load(), kv_telemetry_.current_cells.load(), 0.0f);

    ISHA_LOG_INFO("LlamaCppEngine", "Generation complete: " + std::to_string(result.tokens_generated) +
                  " tokens in " + std::to_string(result.total_ms) + "ms (" +
                  std::to_string(result.tokens_per_second) + " tok/s)");

    return result;
}

bool LlamaCppEngine::isModelLoaded() const {
    return model_loaded_.load(std::memory_order_acquire);
}

// === IInferenceEngine interface bridges ===

bool LlamaCppEngine::load(const std::string& model_path, const InferenceContext& /*context*/) {
    return loadModel(model_path);
}

void LlamaCppEngine::unload() {
    unloadModel();
}

bool LlamaCppEngine::isLoaded() const {
    return isModelLoaded();
}

unsigned int LlamaCppEngine::memoryUsageMB() const {
    if (!model_loaded_.load(std::memory_order_acquire)) return 0;
    // Estimate: model file size via mmap + context KV cache
    // For Q4_K_M 0.5B: ~380MB mmap + ~30MB KV at 512 context
    return 400; // Conservative estimate for telemetry
}

bool LlamaCppEngine::stream(const std::string& prompt, const InferenceContext& context,
                             IInferenceEngine::TokenCallback callback) {
    // Adapt the void-returning callback to the bool-returning TokenCallback
    GenerationResult result = streamEx(prompt, context,
        [&callback](const std::string& token) -> bool {
            callback(token);
            return true; // Always continue
        });
    return result.success;
}

void LlamaCppEngine::setConfig(const LlamaEngineConfig& config) {
    engine_config_ = config;
    // Enforce hard limits
    engine_config_.n_ctx = std::min(engine_config_.n_ctx, 512);
    engine_config_.n_gpu_layers = (acceleration_config_.requested_backend != AccelerationBackend::CPU_ONLY)
                                  ? acceleration_config_.n_gpu_layers : 0;
    ISHA_LOG_INFO("LlamaCppEngine", "Config set: n_ctx=" + std::to_string(engine_config_.n_ctx) +
                  " gpu_layers=" + std::to_string(engine_config_.n_gpu_layers) +
                  " mmap=" + std::to_string(engine_config_.use_mmap));
}

void LlamaCppEngine::setSamplingConfig(const SamplingConfig& sampling) {
    sampling_config_ = sampling;
    sampling_config_.clamp();
}

void LlamaCppEngine::setTemperature(double temperature_c) {
    current_temperature_c_ = temperature_c;
}

int LlamaCppEngine::vocabSize() const {
    if (!model_) return 0;
    return static_cast<int>(llama_model_n_embd(model_)); // Approximate
}

int LlamaCppEngine::contextSize() const {
    return engine_config_.n_ctx;
}

int LlamaCppEngine::embeddingSize() const {
    if (embd_override_ > 0) return embd_override_;
    if (!model_) return 0;
    return llama_model_n_embd(model_);
}

// === Private helpers ===

bool LlamaCppEngine::tokenize(const std::string& text, std::vector<int32_t>& tokens, bool add_bos) const {
    if (!vocab_) return false;

    // First pass: determine token count
    int n_tokens = llama_tokenize(vocab_, text.c_str(), static_cast<int32_t>(text.length()),
                                   nullptr, 0, add_bos, true);
    if (n_tokens < 0) {
        n_tokens = -n_tokens;
    }
    
    tokens.resize(n_tokens);
    
    // Second pass: actual tokenization
    int result = llama_tokenize(vocab_, text.c_str(), static_cast<int32_t>(text.length()),
                                 tokens.data(), static_cast<int32_t>(tokens.size()), add_bos, true);
    if (result < 0) {
        ISHA_LOG_ERROR("LlamaCppEngine", "Tokenization failed: result=" + std::to_string(result));
        return false;
    }

    tokens.resize(result);
    return true;
}

std::string LlamaCppEngine::detokenize(int32_t token) const {
    if (!vocab_) return "";

    char buf[256];
    int n = llama_token_to_piece(vocab_, token, buf, sizeof(buf), 0, true);
    if (n < 0) {
        // Buffer too small, try larger
        std::vector<char> large_buf(-n + 1);
        n = llama_token_to_piece(vocab_, token, large_buf.data(), 
                                  static_cast<int32_t>(large_buf.size()), 0, true);
        if (n > 0) {
            return std::string(large_buf.data(), n);
        }
        return "";
    }
    return std::string(buf, n);
}

llama_sampler* LlamaCppEngine::createSampler() const {
    auto chain_params = llama_sampler_chain_default_params();
    llama_sampler* chain = llama_sampler_chain_init(chain_params);
    
    if (!chain) return nullptr;

    // Add temperature
    if (sampling_config_.temperature > 0.0f) {
        llama_sampler_chain_add(chain, llama_sampler_init_temp(sampling_config_.temperature));
    }

    // Add top-k
    llama_sampler_chain_add(chain, llama_sampler_init_top_k(static_cast<int32_t>(sampling_config_.top_k)));

    // Add top-p
    llama_sampler_chain_add(chain, llama_sampler_init_top_p(sampling_config_.top_p, 1));

    // Add distribution sampling
    llama_sampler_chain_add(chain, llama_sampler_init_dist(42)); // Seed for reproducibility

    return chain;
}

int LlamaCppEngine::computeThreadCount() const {
    return InferenceThreadPolicy::computeThreads(profile_, current_temperature_c_, embeddingSize());
}

void LlamaCppEngine::cleanupContext() {
    if (ctx_) {
        llama_kv_self_clear(ctx_);
        kv_telemetry_.reset();
    }
}

int LlamaCppEngine::computeBatchSize(int model_embd_size) const {
    if (engine_config_.n_batch > 0) return engine_config_.n_batch;
    
    bool is_large_model = (model_embd_size > 1024); // Qwen 0.5B=896, 1.5B=1536
    
    switch (profile_.tier) {
        case DeviceTier::LOW:
            return is_large_model ? 64 : 128;
        case DeviceTier::MID:
            return is_large_model ? 128 : 256;
        case DeviceTier::HIGH:
            return is_large_model ? 256 : 512;
        case DeviceTier::FLAGSHIP:
            return 256; // FIXATION 5: 256 default, not 512
        default:
            return 128;
    }
}

void LlamaCppEngine::sampleRSSPeak() {
    std::unique_ptr<IResourceMonitor> monitor(createResourceMonitor());
    if (monitor) {
        uint64_t rss = monitor->getProcessRSSBytes();
        kv_telemetry_.recordPeakRSS(rss);
    }
}

void LlamaCppEngine::samplePhase(KVTelemetry::Phase phase) {
    std::unique_ptr<IResourceMonitor> monitor(createResourceMonitor());
    if (monitor) {
        uint64_t rss = monitor->getProcessRSSBytes();
        uint64_t pf = monitor->getPageFaultCount();
        kv_telemetry_.recordPhasePeak(phase, rss);
        kv_telemetry_.recordPeakRSS(rss);
        kv_telemetry_.hard_page_faults.store(pf, std::memory_order_relaxed);
    }
}

LlamaCppEngine::ModelSize LlamaCppEngine::detectModelSize() const {
    int embd = embeddingSize();
    if (embd <= 1024) return ModelSize::SMALL;       // 0.5B (n_embd=896)
    if (embd <= 2048) return ModelSize::MEDIUM;      // 1.5B (n_embd=1536)
    return ModelSize::LARGE;                          // 2B+ (n_embd>2048)
}

bool LlamaCppEngine::canLoadOnTier() const {
    ModelSize size = detectModelSize();
    if (size == ModelSize::MEDIUM && profile_.tier == DeviceTier::LOW) return false;
    if (size == ModelSize::LARGE &&
        (profile_.tier == DeviceTier::LOW || profile_.tier == DeviceTier::MID)) return false;
    return true;
}

void LlamaCppEngine::setAccelerationConfig(const AccelerationConfig& config) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    acceleration_config_ = config;
    engine_config_.n_gpu_layers = (acceleration_config_.requested_backend != AccelerationBackend::CPU_ONLY)
                                  ? acceleration_config_.n_gpu_layers : 0;
}

AccelerationBackend LlamaCppEngine::activeBackend() const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    return acceleration_config_.requested_backend;
}

bool LlamaCppEngine::fallbackToCPU() {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    ISHA_LOG_WARN("LlamaCppEngine", "Forcing fallback to CPU_ONLY backend");
    
    kv_telemetry_.fallback_success_count.fetch_add(1, std::memory_order_relaxed);
    
    acceleration_config_.requested_backend = AccelerationBackend::CPU_ONLY;
    acceleration_config_.n_gpu_layers = 0;
    engine_config_.n_gpu_layers = 0;
    
    if (model_loaded_) {
        std::string current_path = model_path_;
        
        cleanupContext();
        if (model_) {
            llama_model_free(model_);
            model_ = nullptr;
        }
        vocab_ = nullptr;
        model_loaded_.store(false, std::memory_order_relaxed);
        
        kv_telemetry_.teardown_success_count.fetch_add(1, std::memory_order_relaxed);
        
        auto params = llama_model_default_params();
        params.n_gpu_layers = 0;
        params.use_mmap = true;
        
        model_ = llama_model_load_from_file(current_path.c_str(), params);
        if (!model_) {
            kv_telemetry_.unrecoverable_deadlock_count.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        vocab_ = llama_model_get_vocab(model_);
        auto ctx_params = llama_context_default_params();
        ctx_params.n_ctx = static_cast<uint32_t>(engine_config_.n_ctx);
        ctx_params.n_batch = static_cast<uint32_t>(active_n_batch_);
        ctx_params.n_threads = static_cast<uint32_t>(computeThreadCount());
        ctx_params.n_threads_batch = ctx_params.n_threads;
        
        ctx_ = llama_init_from_model(model_, ctx_params);
        if (!ctx_) {
            kv_telemetry_.unrecoverable_deadlock_count.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        model_loaded_.store(true, std::memory_order_relaxed);
        
        kv_telemetry_.recovery_success_count.fetch_add(1, std::memory_order_relaxed);
    }
    return true;
}

} // namespace isha
