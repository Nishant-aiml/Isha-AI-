#include "../core/cognition/context_governor.hpp"
#include "../core/retrieval/retrieval_interface.hpp"
#include <iostream>
#include <vector>
#include <cassert>

int main() {
    std::cout << "=== CONTEXT BUDGET BENCHMARK ===\n";
    
    isha::ContextBudget budget;
    budget.max_total_tokens = 500;
    budget.max_system_tokens = 50;
    budget.max_retrieval_tokens = 100;
    budget.max_history_tokens = 150;
    budget.max_query_tokens = 50;
    budget.reserved_generation_tokens = 100;
    
    isha::ContextGovernor governor(budget);
    
    // Test 1: Exact fit
    std::cout << "[TEST 1] Exact fit context assembly... ";
    std::vector<isha::DocumentChunk> chunks = {
        {"chunk_1", "Short retrieval text context description.", "kisan_saathi", 0.9}
    };
    std::vector<std::pair<std::string, std::string>> history = {
        {"Hello", "Hi there assistant."}
    };
    
    auto ctx = governor.assembleContext("You are a helpful assistant.", chunks, history, "What is wheat rust?");
    assert(!ctx.was_truncated);
    std::cout << "PASS (estimated=" << ctx.estimated_tokens << ")\n";
    
    // Test 2: Retrieval context budget overflow & truncation
    std::cout << "[TEST 2] Retrieval budget truncation... ";
    std::vector<isha::DocumentChunk> large_chunks;
    for (int i = 0; i < 20; ++i) {
        large_chunks.push_back({"chunk_" + std::to_string(i), "This is a very long sentence that will consume token budget rapidly.", "kisan_saathi", 0.8});
    }
    
    auto ctx_ret = governor.assembleContext("Helpful assistant.", large_chunks, history, "Short query");
    assert(ctx_ret.was_truncated);
    assert(governor.estimateTokens(ctx_ret.retrieval_context) <= budget.max_retrieval_tokens);
    std::cout << "PASS (retrieval tokens=" << governor.estimateTokens(ctx_ret.retrieval_context) << ")\n";
    
    // Test 3: History window eviction (sliding context)
    std::cout << "[TEST 3] Conversation history sliding window... ";
    std::vector<std::pair<std::string, std::string>> long_history;
    for (int i = 0; i < 30; ++i) {
        long_history.push_back({"User query message number " + std::to_string(i), "Assistant response text description number " + std::to_string(i)});
    }
    
    auto ctx_hist = governor.assembleContext("Helpful assistant.", chunks, long_history, "Short query");
    assert(ctx_hist.was_truncated);
    assert(governor.estimateTokens(ctx_hist.conversation_history) <= budget.max_history_tokens);
    std::cout << "PASS (history tokens=" << governor.estimateTokens(ctx_hist.conversation_history) << ")\n";
    
    std::cout << "=== CONTEXT BUDGET BENCHMARK COMPLETED SUCCESSFULLY ===\n";
    return 0;
}
