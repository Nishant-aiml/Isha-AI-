#include "gguf_inference_engine.hpp"
#include "mmap_manager.hpp"
#include "../logging/logger.hpp"
#include <thread>

namespace isha {

GGUFInferenceEngine::GGUFInferenceEngine()
    : loader_(std::make_unique<GGUFLoader>()),
      mmap_manager_(std::make_unique<MmapManager>()),
      is_loaded_(false),
      memory_usage_mb_(0) {}

GGUFInferenceEngine::~GGUFInferenceEngine() {
    unload();
}

bool GGUFInferenceEngine::load(const std::string& model_path, const InferenceContext& context) {
    if (is_loaded_) {
        ISHA_LOG_WARN("GGUFEngine", "Model already loaded. Unloading first.");
        unload();
    }

    ISHA_LOG_INFO("GGUFEngine", "Initializing GGUF parser for: " + model_path);
    if (!loader_->parseFile(model_path)) {
        ISHA_LOG_ERROR("GGUFEngine", "GGUF header parsing failed.");
        return false;
    }

    if (!mmap_manager_->mapFile(model_path)) {
        ISHA_LOG_ERROR("GGUFEngine", "Failed to memory-map model files.");
        return false;
    }

    // Check if context memory budget is respected
    unsigned int mapped_size_mb = static_cast<unsigned int>(mmap_manager_->getRegion().length / (1024 * 1024));
    if (mapped_size_mb > context.memory_budget_mb) {
        ISHA_LOG_ERROR("GGUFEngine", "Model mapped size (" + std::to_string(mapped_size_mb) 
                       + "MB) exceeds context budget limit: " + std::to_string(context.memory_budget_mb) + "MB");
        unload();
        return false;
    }

    memory_usage_mb_.store(mapped_size_mb);
    is_loaded_.store(true);
    ISHA_LOG_INFO("GGUFEngine", "GGUF Engine loaded model successfully. Mapped size: " + std::to_string(mapped_size_mb) + "MB");
    return true;
}

void GGUFInferenceEngine::unload() {
    if (!is_loaded_) return;

    ISHA_LOG_INFO("GGUFEngine", "Unloading model and cleaning up regions.");
    mmap_manager_->unmapFile();
    
    loader_ = std::make_unique<GGUFLoader>();
    is_loaded_.store(false);
    memory_usage_mb_.store(0);
}

std::string GGUFInferenceEngine::generate(const std::string& prompt, const InferenceContext& context) {
    if (!is_loaded_) {
        ISHA_LOG_ERROR("GGUFEngine", "Cannot generate: model not loaded.");
        return "";
    }

    ISHA_LOG_INFO("GGUFEngine", "Executing token generation loop for prompt: '" + prompt + "'");
    std::string response = "Mock GGUF response to prompt: '" + prompt + "'";
    
    // Simulate generation checkpoints
    for (unsigned int i = 0; i < context.max_tokens_to_generate; ++i) {
        // Micro-yield checkpoint 1
        std::this_thread::yield();
        if (context.cancel_token && context.cancel_token->isCancelled()) {
            ISHA_LOG_WARN("GGUFEngine", "Generation cancelled at token index (pre-decode): " + std::to_string(i));
            return "GENERATION_CANCELLED";
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(50)); // Simulating first half of token decode

        // Micro-yield checkpoint 2
        std::this_thread::yield();
        if (context.cancel_token && context.cancel_token->isCancelled()) {
            ISHA_LOG_WARN("GGUFEngine", "Generation cancelled at token index (mid-decode): " + std::to_string(i));
            return "GENERATION_CANCELLED";
        }

        std::this_thread::sleep_for(std::chrono::microseconds(50)); // Simulating second half of token decode
    }

    return response;
}

bool GGUFInferenceEngine::stream(const std::string& prompt, const InferenceContext& context, TokenCallback callback) {
    if (!is_loaded_) {
        ISHA_LOG_ERROR("GGUFEngine", "Cannot stream: model not loaded.");
        return false;
    }

    ISHA_LOG_INFO("GGUFEngine", "Streaming GGUF output tokens for prompt: '" + prompt + "'");
    
    std::vector<std::string> tokens = {"This", " ", "is", " ", "a", " ", "real", " ", "quantized", " ", "stream", " ", "token", "."};
    
    for (size_t i = 0; i < tokens.size() && i < context.max_tokens_to_generate; ++i) {
        // Micro-yield checkpoint 1
        std::this_thread::yield();
        if (context.cancel_token && context.cancel_token->isCancelled()) {
            ISHA_LOG_WARN("GGUFEngine", "Streaming cancelled at index (pre-token): " + std::to_string(i));
            return false;
        }
        
        if (callback) {
            callback(tokens[i]);
        }

        // Micro-yield checkpoint 2
        for (int step = 0; step < 5; ++step) {
            std::this_thread::yield();
            if (context.cancel_token && context.cancel_token->isCancelled()) {
                ISHA_LOG_WARN("GGUFEngine", "Streaming cancelled at index (mid-token): " + std::to_string(i));
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 5ms simulated total split into 1ms chunks
        }
    }

    return true;
}

} // namespace isha
