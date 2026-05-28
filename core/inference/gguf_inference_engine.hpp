#ifndef ISHA_AI_GGUF_INFERENCE_ENGINE_HPP
#define ISHA_AI_GGUF_INFERENCE_ENGINE_HPP

#include "inference_engine.hpp"
#include "gguf_loader.hpp"
#include <atomic>

namespace isha {

class MmapManager; // Forward declaration

class GGUFInferenceEngine : public IInferenceEngine {
public:
    GGUFInferenceEngine();
    ~GGUFInferenceEngine() override;

    bool load(const std::string& model_path, const InferenceContext& context) override;
    void unload() override;
    std::string generate(const std::string& prompt, const InferenceContext& context) override;
    bool stream(const std::string& prompt, const InferenceContext& context, TokenCallback callback) override;

    unsigned int memoryUsageMB() const override { return memory_usage_mb_; }
    bool isLoaded() const override { return is_loaded_; }

private:
    std::unique_ptr<GGUFLoader> loader_;
    std::unique_ptr<MmapManager> mmap_manager_;
    std::atomic<bool> is_loaded_;
    std::atomic<unsigned int> memory_usage_mb_;
};

} // namespace isha

#endif // ISHA_AI_GGUF_INFERENCE_ENGINE_HPP
