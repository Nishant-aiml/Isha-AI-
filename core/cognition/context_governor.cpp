#include "context_governor.hpp"
#include "../logging/logger.hpp"
#include "../runtime/event_bus.hpp"
#include <sstream>
#include <algorithm>

namespace isha {

ContextGovernor::ContextGovernor(const ContextBudget& budget) : budget_(budget) {
    ISHA_LOG_INFO("ContextGovernor", "Initialized with max budget: " + std::to_string(budget_.max_total_tokens) + " tokens");
}

unsigned int ContextGovernor::estimateTokens(const std::string& text) const {
    if (text.empty()) return 0;
    // Approximate: 1 token ~= 4 characters (standard GPT tokenizer approximation)
    return static_cast<unsigned int>((text.size() + 3) / 4);
}

std::string ContextGovernor::truncateToTokenBudget(const std::string& text, unsigned int max_tokens) const {
    unsigned int estimated = estimateTokens(text);
    if (estimated <= max_tokens) return text;
    
    // Truncate to approximate character count
    size_t max_chars = static_cast<size_t>(max_tokens) * 4;
    if (max_chars >= text.size()) return text;
    
    std::string truncated = text.substr(0, max_chars);
    // Find last space to avoid cutting mid-word
    size_t last_space = truncated.rfind(' ');
    if (last_space != std::string::npos && last_space > max_chars / 2) {
        truncated = truncated.substr(0, last_space);
    }
    truncated += " [truncated]";
    return truncated;
}

AssembledContext ContextGovernor::assembleContext(
    const std::string& system_prompt,
    const std::vector<DocumentChunk>& retrieved_chunks,
    const std::vector<std::pair<std::string, std::string>>& conversation_history,
    const std::string& user_query
) const {
    AssembledContext ctx;
    ctx.was_truncated = false;
    
    // 1. System prompt (truncate if needed)
    ctx.system_prompt = truncateToTokenBudget(system_prompt, budget_.max_system_tokens);
    
    // 2. Retrieval context — assemble from chunks
    std::string retrieval_text;
    for (const auto& chunk : retrieved_chunks) {
        std::string entry = "[" + chunk.pack_id + "/" + chunk.chunk_id + "] " + chunk.text + "\n";
        if (estimateTokens(retrieval_text + entry) > budget_.max_retrieval_tokens) {
            ctx.was_truncated = true;
            break;
        }
        retrieval_text += entry;
    }
    ctx.retrieval_context = retrieval_text;
    
    // 3. Conversation history — sliding window from most recent
    std::string history_text;
    // Build from newest to oldest, then reverse
    std::vector<std::string> turn_strings;
    for (auto it = conversation_history.rbegin(); it != conversation_history.rend(); ++it) {
        std::string turn = "User: " + it->first + "\nAssistant: " + it->second + "\n";
        if (estimateTokens(history_text + turn) > budget_.max_history_tokens) {
            ctx.was_truncated = true;
            break;
        }
        turn_strings.push_back(turn);
        history_text += turn;
    }
    // Reverse to get chronological order
    std::string ordered_history;
    for (auto it = turn_strings.rbegin(); it != turn_strings.rend(); ++it) {
        ordered_history += *it;
    }
    ctx.conversation_history = ordered_history;
    
    // 4. User query (truncate if needed)
    ctx.user_query = truncateToTokenBudget(user_query, budget_.max_query_tokens);
    
    // 5. Calculate total
    ctx.estimated_tokens = estimateTokens(ctx.system_prompt)
                         + estimateTokens(ctx.retrieval_context)
                         + estimateTokens(ctx.conversation_history)
                         + estimateTokens(ctx.user_query);
    
    // Check total budget (excluding generation reserve)
    unsigned int input_budget = budget_.max_total_tokens - budget_.reserved_generation_tokens;
    if (ctx.estimated_tokens > input_budget) {
        ISHA_LOG_WARN("ContextGovernor", "Context overflow: " + std::to_string(ctx.estimated_tokens)
                      + " tokens exceeds input budget of " + std::to_string(input_budget));
        EventBus::getInstance().publish({EventType::CONTEXT_OVERFLOW, "ContextGovernor",
            "tokens=" + std::to_string(ctx.estimated_tokens)});
        ctx.was_truncated = true;
    }
    
    ISHA_LOG_INFO("ContextGovernor", "Assembled context: " + std::to_string(ctx.estimated_tokens)
                  + " tokens (truncated=" + (ctx.was_truncated ? "yes" : "no") + ")");
    
    return ctx;
}

} // namespace isha
