#include "multimodal_context_governor.hpp"
#include "../logging/logger.hpp"
#include "../runtime/event_bus.hpp"

namespace isha {

MultimodalContextGovernor::MultimodalContextGovernor(const MultimodalBudget& budget)
    : budget_(budget)
    , base_governor_(ContextBudget{
        budget.max_total_tokens,
        budget.max_system_tokens,
        budget.max_retrieval_tokens,
        budget.max_history_tokens,
        budget.max_query_tokens,
        budget.reserved_generation_tokens
    })
{
    // CLASS_E: VLM disabled for V1. Force vision token budget to 0.
    if (isVlmDisabledV1()) {
        budget_.max_vision_tokens = 0;
    }
    ISHA_LOG_INFO("MultimodalContextGovernor", "Initialized with max budget: "
                  + std::to_string(budget_.max_total_tokens) + " tokens (ocr="
                  + std::to_string(budget_.max_ocr_tokens) + ", vision="
                  + std::to_string(budget_.max_vision_tokens)
                  + (isVlmDisabledV1() ? " [VLM DISABLED V1]" : "") + ")");
}

unsigned int MultimodalContextGovernor::estimateTokens(const std::string& text) const {
    return base_governor_.estimateTokens(text);
}

std::string MultimodalContextGovernor::truncateToTokenBudget(const std::string& text, unsigned int max_tokens) const {
    return base_governor_.truncateToTokenBudget(text, max_tokens);
}

MultimodalAssembledContext MultimodalContextGovernor::assembleMultimodalContext(
    const std::string& system_prompt,
    const std::vector<DocumentChunk>& retrieved_chunks,
    const std::vector<std::pair<std::string, std::string>>& conversation_history,
    const std::string& user_query,
    const std::string& ocr_text,
    const std::string& vision_description
) const {
    // Delegate base text context assembly to ContextGovernor
    AssembledContext base = base_governor_.assembleContext(
        system_prompt, retrieved_chunks, conversation_history, user_query
    );

    MultimodalAssembledContext ctx;
    ctx.system_prompt = std::move(base.system_prompt);
    ctx.retrieval_context = std::move(base.retrieval_context);
    ctx.conversation_history = std::move(base.conversation_history);
    ctx.user_query = std::move(base.user_query);
    ctx.was_truncated = base.was_truncated;

    // Truncate OCR text to budget
    ctx.ocr_text = truncateToTokenBudget(ocr_text, budget_.max_ocr_tokens);
    if (estimateTokens(ocr_text) > budget_.max_ocr_tokens) {
        ctx.was_truncated = true;
    }

    // -------------------------------------------------------
    // CLASS_E: VLM routing is DISABLED for V1 ship.
    // MobileVLM and LLaVA are excluded from V1.
    // Any vision_description input is cleared and replaced with
    // a DISABLED_V1 marker. VLM routing does not happen.
    // -------------------------------------------------------
    std::string effective_vision;
    if (isVlmDisabledV1()) {
        if (!vision_description.empty()) {
            ISHA_LOG_WARN("MultimodalContextGovernor",
                "VLM routing suppressed (CLASS_E DISABLED_V1): vision input cleared.");
        }
        effective_vision = ""; // Zero vision tokens — no VLM path
    } else {
        // Truncate vision description to budget
        effective_vision = truncateToTokenBudget(vision_description, budget_.max_vision_tokens);
        if (estimateTokens(vision_description) > budget_.max_vision_tokens) {
            ctx.was_truncated = true;
        }
    }
    ctx.vision_description = effective_vision;

    // Calculate total token count including multimodal slots
    ctx.estimated_tokens = base.estimated_tokens
                         + estimateTokens(ctx.ocr_text)
                         + estimateTokens(ctx.vision_description);

    // Enforce total budget
    unsigned int input_budget = budget_.max_total_tokens - budget_.reserved_generation_tokens;
    if (ctx.estimated_tokens > input_budget) {
        ISHA_LOG_WARN("MultimodalContextGovernor", "Context overflow: "
                      + std::to_string(ctx.estimated_tokens) + " tokens exceeds input budget of "
                      + std::to_string(input_budget));
        EventBus::getInstance().publish({EventType::CONTEXT_OVERFLOW, "MultimodalContextGovernor",
            "tokens=" + std::to_string(ctx.estimated_tokens)});
        ctx.was_truncated = true;
    }

    ISHA_LOG_INFO("MultimodalContextGovernor", "Assembled multimodal context: "
                  + std::to_string(ctx.estimated_tokens) + " tokens (ocr="
                  + std::to_string(estimateTokens(ctx.ocr_text)) + ", vision="
                  + std::to_string(estimateTokens(ctx.vision_description))
                  + ", truncated=" + (ctx.was_truncated ? "yes" : "no") + ")");

    return ctx;
}

} // namespace isha
