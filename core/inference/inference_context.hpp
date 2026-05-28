#ifndef ISHA_AI_INFERENCE_CONTEXT_HPP
#define ISHA_AI_INFERENCE_CONTEXT_HPP

#include <string>
#include <memory>
#include "cancellation_token.hpp"

namespace isha {

struct InferenceContext {
    std::string session_id;
    std::shared_ptr<CancellationToken> cancel_token;
    unsigned int memory_budget_mb = 0;
    unsigned int max_tokens_to_generate = 512;
    
    // Custom constructor
    InferenceContext(const std::string& s_id, std::shared_ptr<CancellationToken> token, unsigned int budget)
        : session_id(s_id), cancel_token(token), memory_budget_mb(budget) {}
};

} // namespace isha

#endif // ISHA_AI_INFERENCE_CONTEXT_HPP
