#ifndef ISHA_AI_CONTEXT_GOVERNOR_HPP
#define ISHA_AI_CONTEXT_GOVERNOR_HPP

#include <string>
#include <vector>
#include "../retrieval/retrieval_interface.hpp"

namespace isha {

struct ContextBudget {
    unsigned int max_total_tokens = 2048;
    unsigned int max_system_tokens = 128;
    unsigned int max_retrieval_tokens = 512;
    unsigned int max_history_tokens = 512;
    unsigned int max_query_tokens = 256;
    unsigned int reserved_generation_tokens = 512;
};

struct AssembledContext {
    std::string system_prompt;
    std::string retrieval_context;
    std::string conversation_history;
    std::string user_query;
    unsigned int estimated_tokens;
    bool was_truncated;
};

class ContextGovernor {
public:
    explicit ContextGovernor(const ContextBudget& budget = ContextBudget());
    ~ContextGovernor() = default;

    // Assemble full context from components with budget enforcement
    AssembledContext assembleContext(
        const std::string& system_prompt,
        const std::vector<DocumentChunk>& retrieved_chunks,
        const std::vector<std::pair<std::string, std::string>>& conversation_history,
        const std::string& user_query
    ) const;
    
    // Estimate token count from text (space-based approximation)
    unsigned int estimateTokens(const std::string& text) const;
    
    // Truncate text to fit within token budget
    std::string truncateToTokenBudget(const std::string& text, unsigned int max_tokens) const;
    
    const ContextBudget& getBudget() const { return budget_; }

private:
    ContextBudget budget_;
};

} // namespace isha

#endif // ISHA_AI_CONTEXT_GOVERNOR_HPP
