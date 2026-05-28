#ifndef ISHA_AI_INFERENCE_ENGINE_HPP
#define ISHA_AI_INFERENCE_ENGINE_HPP

#include <string>
#include <memory>
#include <functional>
#include "inference_context.hpp"

namespace isha {

class IInferenceEngine {
public:
    virtual ~IInferenceEngine() = default;

    virtual bool load(const std::string& model_path, const InferenceContext& context) = 0;
    virtual void unload() = 0;
    virtual std::string generate(const std::string& prompt, const InferenceContext& context) = 0;
    
    // Asynchronous/Streaming inference callback
    using TokenCallback = std::function<void(const std::string& token)>;
    virtual bool stream(const std::string& prompt, const InferenceContext& context, TokenCallback callback) = 0;

    virtual unsigned int memoryUsageMB() const = 0;
    virtual bool isLoaded() const = 0;
};

} // namespace isha

#endif // ISHA_AI_INFERENCE_ENGINE_HPP
