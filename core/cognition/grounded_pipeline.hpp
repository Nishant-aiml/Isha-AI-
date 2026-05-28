#ifndef ISHA_AI_GROUNDED_PIPELINE_HPP
#define ISHA_AI_GROUNDED_PIPELINE_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include "context_governor.hpp"
#include "../retrieval/retrieval_interface.hpp"
#include "../inference/inference_engine.hpp"
#include "../inference/inference_context.hpp"

namespace isha {

struct PipelineRequest {
    std::string session_id;
    std::string user_query;
    std::string pack_id;  // Active knowledge pack
    std::shared_ptr<CancellationToken> cancel_token;
    unsigned int memory_budget_mb = 512;
};

struct PipelineResponse {
    std::string generated_text;
    unsigned int context_tokens_used;
    unsigned int retrieval_chunks_used;
    double retrieval_latency_ms;
    double inference_latency_ms;
    bool was_context_truncated;
    bool was_cancelled;
};

class GroundedPipeline {
public:
    GroundedPipeline(
        std::shared_ptr<IRetrievalEngine> retrieval,
        std::shared_ptr<IInferenceEngine> inference,
        const ContextBudget& budget = ContextBudget()
    );
    ~GroundedPipeline() = default;

    // Execute full grounded query pipeline
    PipelineResponse execute(const PipelineRequest& request);
    
    // Execute with streaming callback
    PipelineResponse executeStreaming(
        const PipelineRequest& request,
        std::function<void(const std::string& token)> on_token
    );
    
    // Set system prompt
    void setSystemPrompt(const std::string& prompt) { system_prompt_ = prompt; }
    
    // Add conversation turn to history
    void addConversationTurn(const std::string& session_id, const std::string& query, const std::string& response);
    
    // Clear history for session
    void clearHistory(const std::string& session_id);

private:
    std::shared_ptr<IRetrievalEngine> retrieval_;
    std::shared_ptr<IInferenceEngine> inference_;
    ContextGovernor governor_;
    std::string system_prompt_;
    
    // Per-session conversation history
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> history_;
    std::mutex history_mutex_;
    
    std::string buildPrompt(const AssembledContext& ctx) const;
};

} // namespace isha

#endif // ISHA_AI_GROUNDED_PIPELINE_HPP
