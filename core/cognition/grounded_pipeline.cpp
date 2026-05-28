#include "grounded_pipeline.hpp"
#include "../logging/logger.hpp"
#include <chrono>
#include <map>
#include <mutex>

namespace isha {

GroundedPipeline::GroundedPipeline(
    std::shared_ptr<IRetrievalEngine> retrieval,
    std::shared_ptr<IInferenceEngine> inference,
    const ContextBudget& budget
) : retrieval_(std::move(retrieval)),
    inference_(std::move(inference)),
    governor_(budget),
    system_prompt_("You are ISHA, an intelligent offline assistant. Answer based on the provided context. Be concise and helpful.") {
    ISHA_LOG_INFO("GroundedPipeline", "Initialized grounded cognition pipeline.");
}

void GroundedPipeline::addConversationTurn(const std::string& session_id, const std::string& query, const std::string& response) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    history_[session_id].push_back({query, response});
    ISHA_LOG_DEBUG("GroundedPipeline", "Added turn to session '" + session_id + "' (" + std::to_string(history_[session_id].size()) + " turns)");
}

void GroundedPipeline::clearHistory(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    history_.erase(session_id);
    ISHA_LOG_INFO("GroundedPipeline", "Cleared history for session '" + session_id + "'");
}

std::string GroundedPipeline::buildPrompt(const AssembledContext& ctx) const {
    std::string prompt;
    prompt.reserve(ctx.system_prompt.size() + ctx.retrieval_context.size()
                   + ctx.conversation_history.size() + ctx.user_query.size() + 200);
    
    prompt += "<|system|>\n" + ctx.system_prompt + "\n";
    
    if (!ctx.retrieval_context.empty()) {
        prompt += "\n<|context|>\n" + ctx.retrieval_context + "\n";
    }
    
    if (!ctx.conversation_history.empty()) {
        prompt += "\n<|history|>\n" + ctx.conversation_history + "\n";
    }
    
    prompt += "\n<|user|>\n" + ctx.user_query + "\n";
    prompt += "\n<|assistant|>\n";
    
    return prompt;
}

PipelineResponse GroundedPipeline::execute(const PipelineRequest& request) {
    PipelineResponse response{};
    response.was_cancelled = false;
    
    ISHA_LOG_INFO("GroundedPipeline", "Executing query: '" + request.user_query + "' (pack: " + request.pack_id + ")");
    
    // Check cancellation
    if (request.cancel_token && request.cancel_token->isCancelled()) {
        response.was_cancelled = true;
        return response;
    }
    
    // Step 1: Retrieval
    auto retrieval_start = std::chrono::steady_clock::now();
    auto chunks = retrieval_->retrieve(request.user_query, request.pack_id, 5);
    auto retrieval_end = std::chrono::steady_clock::now();
    response.retrieval_latency_ms = std::chrono::duration<double, std::milli>(retrieval_end - retrieval_start).count();
    response.retrieval_chunks_used = static_cast<unsigned int>(chunks.size());
    
    // Check cancellation
    if (request.cancel_token && request.cancel_token->isCancelled()) {
        response.was_cancelled = true;
        return response;
    }
    
    // Step 2: Context assembly
    std::vector<std::pair<std::string, std::string>> session_history;
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        auto it = history_.find(request.session_id);
        if (it != history_.end()) {
            session_history = it->second;
        }
    }
    
    auto assembled = governor_.assembleContext(system_prompt_, chunks, session_history, request.user_query);
    response.context_tokens_used = assembled.estimated_tokens;
    response.was_context_truncated = assembled.was_truncated;
    
    // Step 3: Build prompt and generate
    std::string prompt = buildPrompt(assembled);
    
    InferenceContext ctx(request.session_id, request.cancel_token, request.memory_budget_mb);
    
    auto inference_start = std::chrono::steady_clock::now();
    response.generated_text = inference_->generate(prompt, ctx);
    auto inference_end = std::chrono::steady_clock::now();
    response.inference_latency_ms = std::chrono::duration<double, std::milli>(inference_end - inference_start).count();
    
    if (response.generated_text == "GENERATION_CANCELLED") {
        response.was_cancelled = true;
    }
    
    // Store turn in history
    if (!response.was_cancelled) {
        addConversationTurn(request.session_id, request.user_query, response.generated_text);
    }
    
    ISHA_LOG_INFO("GroundedPipeline", "Pipeline complete: retrieval=" + std::to_string(response.retrieval_latency_ms)
                  + "ms, inference=" + std::to_string(response.inference_latency_ms) + "ms");
    
    return response;
}

PipelineResponse GroundedPipeline::executeStreaming(
    const PipelineRequest& request,
    std::function<void(const std::string& token)> on_token
) {
    PipelineResponse response{};
    response.was_cancelled = false;
    
    ISHA_LOG_INFO("GroundedPipeline", "Executing streaming query: '" + request.user_query + "'");
    
    // Step 1: Retrieval
    auto retrieval_start = std::chrono::steady_clock::now();
    auto chunks = retrieval_->retrieve(request.user_query, request.pack_id, 5);
    auto retrieval_end = std::chrono::steady_clock::now();
    response.retrieval_latency_ms = std::chrono::duration<double, std::milli>(retrieval_end - retrieval_start).count();
    response.retrieval_chunks_used = static_cast<unsigned int>(chunks.size());
    
    if (request.cancel_token && request.cancel_token->isCancelled()) {
        response.was_cancelled = true;
        return response;
    }
    
    // Step 2: Context assembly
    std::vector<std::pair<std::string, std::string>> session_history;
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        auto it = history_.find(request.session_id);
        if (it != history_.end()) {
            session_history = it->second;
        }
    }
    
    auto assembled = governor_.assembleContext(system_prompt_, chunks, session_history, request.user_query);
    response.context_tokens_used = assembled.estimated_tokens;
    response.was_context_truncated = assembled.was_truncated;
    
    // Step 3: Stream generation
    std::string prompt = buildPrompt(assembled);
    InferenceContext ctx(request.session_id, request.cancel_token, request.memory_budget_mb);
    
    std::string full_response;
    auto inference_start = std::chrono::steady_clock::now();
    bool stream_ok = inference_->stream(prompt, ctx, [&](const std::string& token) {
        full_response += token;
        if (on_token) {
            on_token(token);
        }
    });
    auto inference_end = std::chrono::steady_clock::now();
    response.inference_latency_ms = std::chrono::duration<double, std::milli>(inference_end - inference_start).count();
    
    response.generated_text = full_response;
    response.was_cancelled = !stream_ok;
    
    if (!response.was_cancelled) {
        addConversationTurn(request.session_id, request.user_query, response.generated_text);
    }
    
    return response;
}

} // namespace isha
