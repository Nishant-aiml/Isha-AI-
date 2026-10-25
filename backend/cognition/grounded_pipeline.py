"""
ISHA Grounded Pipeline.
Direct implementation of core/cognition/grounded_pipeline.cpp:
- Constructs grounded prompts enforcing source fidelity
- Formats citation references [L1], [L2] (Local Packs) and [W1], [W2] (Online Web)
- Detects whether response is grounded in facts or requires uncertainty disclosure
"""

from typing import List, Dict, Any, Optional, Tuple
from .context_governor import ContextGovernor, AssembledContext


class GroundedPipeline:
    DEFAULT_SYSTEM_PROMPT = (
        "You are ISHA (Intelligent Speech & Haptic Assistant), a privacy-first on-device AI cognitive runtime. "
        "Your mission is to provide accurate, grounded, helpful guidance across agriculture, healthcare, law, education, and daily life. "
        "Strict rules: "
        "1. Base your answers firmly on the provided [Retrieved Sources]. "
        "2. Cite references directly in your answer using badges like [L1], [L2] for local knowledge packs, and [W1], [W2] for live online sources. "
        "3. For medical and agricultural dosage instructions, remain strictly faithful to referenced facts. "
        "4. Be direct, respectful, concise, and structure your responses with clean markdown headings and bullets."
    )

    def __init__(self, governor: Optional[ContextGovernor] = None):
        self.governor = governor or ContextGovernor()

    def build_prompt(
        self,
        query: str,
        retrieval_data: Dict[str, Any],
        history: List[Dict[str, str]],
        custom_system_prompt: Optional[str] = None
    ) -> Tuple[str, AssembledContext]:
        system_text = custom_system_prompt or self.DEFAULT_SYSTEM_PROMPT
        retrieval_text = retrieval_data.get("context_text", "")

        assembled = self.governor.assemble_context(
            system_prompt=system_text,
            retrieval_context=retrieval_text,
            history=history,
            user_query=query
        )

        prompt_parts = []
        prompt_parts.append(f"<|im_start|>system\n{assembled.system_prompt}\n")

        if assembled.retrieval_context:
            prompt_parts.append(f"\n[RETRIEVED GROUNDING CONTEXT]\n{assembled.retrieval_context}\n[END CONTEXT]\n")

        if assembled.conversation_history:
            prompt_parts.append(f"\n[PRIOR CONVERSATION]\n{assembled.conversation_history}\n")

        prompt_parts.append(f"<|im_end|>\n<|im_start|>user\n{assembled.user_query}<|im_end|>\n<|im_start|>assistant\n")

        full_prompt = "".join(prompt_parts)
        return full_prompt, assembled
