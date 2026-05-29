#ifndef ISHA_AI_MULTIMODAL_CONTEXT_GOVERNOR_HPP
#define ISHA_AI_MULTIMODAL_CONTEXT_GOVERNOR_HPP

#include <string>
#include <vector>
#include "../cognition/context_governor.hpp"

namespace isha {

struct MultimodalBudget {
    // Inherited from ContextBudget fields
    unsigned int max_total_tokens = 2048;
    unsigned int max_system_tokens = 128;
    unsigned int max_retrieval_tokens = 512;
    unsigned int max_history_tokens = 512;
    unsigned int max_query_tokens = 256;
    unsigned int reserved_generation_tokens = 512;
    // Multimodal-specific
    unsigned int max_ocr_tokens = 128;
    unsigned int max_vision_tokens = 256;
};

struct MultimodalAssembledContext {
    std::string system_prompt;
    std::string retrieval_context;
    std::string conversation_history;
    std::string user_query;
    std::string ocr_text;
    std::string vision_description;
    unsigned int estimated_tokens;
    bool was_truncated;
};

class MultimodalContextGovernor {
public:
    explicit MultimodalContextGovernor(const MultimodalBudget& budget = MultimodalBudget());
    ~MultimodalContextGovernor() = default;

    MultimodalAssembledContext assembleMultimodalContext(
        const std::string& system_prompt,
        const std::vector<DocumentChunk>& retrieved_chunks,
        const std::vector<std::pair<std::string, std::string>>& conversation_history,
        const std::string& user_query,
        const std::string& ocr_text,
        const std::string& vision_description
    ) const;

    unsigned int estimateTokens(const std::string& text) const;
    std::string truncateToTokenBudget(const std::string& text, unsigned int max_tokens) const;

    const MultimodalBudget& getBudget() const { return budget_; }

    // CLASS_E: MobileVLM and LLaVA are DISABLED for V1 ship.
    // VLM routing, image encoders, camera pipelines all return DISABLED_V1.
    // Re-enable in V2+ only after survivability validation.
    static bool isVlmDisabledV1() { return true; }

private:
    MultimodalBudget budget_;
    ContextGovernor base_governor_;
};

} // namespace isha

#endif // ISHA_AI_MULTIMODAL_CONTEXT_GOVERNOR_HPP
