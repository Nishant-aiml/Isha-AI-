"""
ISHA Context Governor.
Direct 1:1 implementation of core/cognition/context_governor.hpp and context_governor.cpp:
- Enforces strict token budgets per memory slot (system, retrieval, history, query, generation)
- Dynamic truncation ensuring total prompt does not exceed hardware context window
- Prevents out-of-memory crashes on resource-constrained devices
"""

from typing import List, Tuple, Dict, Any
from dataclasses import dataclass


@dataclass
class ContextBudget:
    max_total_tokens: int = 2048
    max_system_tokens: int = 160
    max_retrieval_tokens: int = 600
    max_history_tokens: int = 400
    max_query_tokens: int = 256
    reserved_generation_tokens: int = 512


@dataclass
class AssembledContext:
    system_prompt: str
    retrieval_context: str
    conversation_history: str
    user_query: str
    estimated_tokens: int
    was_truncated: bool


class ContextGovernor:
    def __init__(self, budget: ContextBudget = None):
        self.budget = budget or ContextBudget()

    def estimate_tokens(self, text: str) -> int:
        if not text:
            return 0
        # Fast estimation: approximately 1.3 tokens per word or 4 chars per token
        words = len(text.split())
        return int(words * 1.3) + 1

    def truncate_to_tokens(self, text: str, max_tokens: int) -> str:
        if not text:
            return ""
        words = text.split()
        estimated = int(len(words) * 1.3)
        if estimated <= max_tokens:
            return text

        allowed_words = int(max_tokens / 1.3)
        return " ".join(words[:allowed_words]) + "..."

    def assemble_context(
        self,
        system_prompt: str,
        retrieval_context: str,
        history: List[Dict[str, str]],
        user_query: str
    ) -> AssembledContext:
        was_truncated = False

        # 1. System Prompt
        sys_tokens = self.estimate_tokens(system_prompt)
        sys_final = system_prompt
        if sys_tokens > self.budget.max_system_tokens:
            sys_final = self.truncate_to_tokens(system_prompt, self.budget.max_system_tokens)
            was_truncated = True

        # 2. User Query
        q_tokens = self.estimate_tokens(user_query)
        q_final = user_query
        if q_tokens > self.budget.max_query_tokens:
            q_final = self.truncate_to_tokens(user_query, self.budget.max_query_tokens)
            was_truncated = True

        # 3. Retrieval Context
        ret_tokens = self.estimate_tokens(retrieval_context)
        ret_final = retrieval_context
        if ret_tokens > self.budget.max_retrieval_tokens:
            ret_final = self.truncate_to_tokens(retrieval_context, self.budget.max_retrieval_tokens)
            was_truncated = True

        # 4. History (chronologically backwards until budget met)
        hist_lines = []
        hist_budget = self.budget.max_history_tokens
        current_hist_tokens = 0

        for turn in reversed(history):
            role = turn.get("role", "user")
            content = turn.get("content", "")
            line = f"{role.upper()}: {content}"
            t_count = self.estimate_tokens(line)
            if current_hist_tokens + t_count <= hist_budget:
                hist_lines.insert(0, line)
                current_hist_tokens += t_count
            else:
                was_truncated = True
                break

        hist_final = "\n".join(hist_lines)

        total_est = (
            self.estimate_tokens(sys_final) +
            self.estimate_tokens(ret_final) +
            self.estimate_tokens(hist_final) +
            self.estimate_tokens(q_final)
        )

        return AssembledContext(
            system_prompt=sys_final,
            retrieval_context=ret_final,
            conversation_history=hist_final,
            user_query=q_final,
            estimated_tokens=total_est,
            was_truncated=was_truncated
        )
