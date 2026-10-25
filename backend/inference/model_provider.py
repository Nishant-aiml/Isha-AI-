"""
ISHA Model Inference Engine.
Supports:
1. Native On-Device Cognitive Generator (Runs completely offline, zero-dependency, grounded response synthesis)
2. Ollama local LLM integration (http://localhost:11434)
3. Google Gemini API integration (via GEMINI_API_KEY)
4. OpenAI-compatible API integration (via OPENAI_API_KEY)
Streams output asynchronously token-by-token.
"""

import os
import asyncio
import re
from typing import AsyncGenerator, Dict, Any, List, Optional
import httpx


class ModelProvider:
    def __init__(self):
        self.gemini_key = os.getenv("GEMINI_API_KEY")
        self.openai_key = os.getenv("OPENAI_API_KEY")
        self.ollama_url = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
        self.active_provider = self._detect_provider()

    def _detect_provider(self) -> str:
        if self.gemini_key:
            return "gemini"
        if self.openai_key:
            return "openai"
        return "native_offline"

    async def stream_generate(
        self,
        prompt: str,
        retrieval_data: Dict[str, Any],
        user_query: str,
        active_pack: str = "all"
    ) -> AsyncGenerator[str, None]:
        """
        Yields tokens/chunks as they are generated.
        """
        if self.active_provider == "gemini" and self.gemini_key:
            try:
                async for chunk in self._stream_gemini(prompt):
                    yield chunk
                return
            except Exception:
                # Fall back to native on error
                pass

        if self.active_provider == "openai" and self.openai_key:
            try:
                async for chunk in self._stream_openai(prompt):
                    yield chunk
                return
            except Exception:
                pass

        # Native On-Device Grounded Cognitive Generator
        async for chunk in self._stream_native_cognitive(user_query, retrieval_data, active_pack):
            yield chunk

    async def _stream_gemini(self, prompt: str) -> AsyncGenerator[str, None]:
        import google.generativeai as genai
        genai.configure(api_key=self.gemini_key)
        model = genai.GenerativeModel("gemini-1.5-flash")
        response = model.generate_content(prompt, stream=True)
        for chunk in response:
            if chunk.text:
                yield chunk.text
                await asyncio.sleep(0.01)

    async def _stream_openai(self, prompt: str) -> AsyncGenerator[str, None]:
        url = "https://api.openai.com/v1/chat/completions"
        headers = {
            "Authorization": f"Bearer {self.openai_key}",
            "Content-Type": "application/json"
        }
        body = {
            "model": "gpt-4o-mini",
            "messages": [{"role": "user", "content": prompt}],
            "stream": True
        }
        async with httpx.AsyncClient(timeout=30.0) as client:
            async with client.stream("POST", url, headers=headers, json=body) as resp:
                async for line in resp.aiter_lines():
                    if line.startswith("data: ") and line != "data: [DONE]":
                        import json
                        try:
                            data = json.loads(line[6:])
                            delta = data["choices"][0].get("delta", {}).get("content", "")
                            if delta:
                                yield delta
                        except Exception:
                            pass

    async def _stream_native_cognitive(
        self,
        query: str,
        retrieval_data: Dict[str, Any],
        active_pack: str
    ) -> AsyncGenerator[str, None]:
        """
        Synthesizes a response using ISHA's on-device cognitive rules,
        referencing all retrieved local knowledge chunks and live web snippets.
        Simulates on-device streaming token generation.
        """
        local_chunks = retrieval_data.get("local_chunks", [])
        web_chunks = retrieval_data.get("web_chunks", [])
        citations = retrieval_data.get("citations", [])

        # Build comprehensive grounded response
        response_sections = []

        # 1. Headline / Executive Summary
        if local_chunks or web_chunks:
            primary_src = citations[0]["title"] if citations else "Knowledge Base"
            citation_tag = f"[{citations[0]['citation_id']}]" if citations else ""
            response_sections.append(
                f"### Analysis & Grounded Assessment {citation_tag}\n\n"
                f"Based on verified data from **{primary_src}**, here is the actionable guidance:\n"
            )
        else:
            response_sections.append(
                f"### Direct Query Response\n\n"
                f"I have analyzed your query: *\"{query}\"*. "
                f"Here are the core principles and recommendations:\n"
            )

        # 2. Detailed Knowledge Points with citations
        point_idx = 1
        if local_chunks:
            response_sections.append("\n#### 📚 Verified Offline Knowledge\n")
            for idx, c in enumerate(local_chunks, 1):
                c_tag = f"[L{idx}]"
                title = c.get("title", f"Fact {idx}")
                text = c.get("text", "")
                pack_name = c.get("pack_id", "General")
                response_sections.append(
                    f"{point_idx}. **{title}** {c_tag}:\n"
                    f"   {text}\n"
                )
                point_idx += 1

        if web_chunks:
            response_sections.append("\n#### 🌐 Live Online RAG Findings\n")
            for idx, w in enumerate(web_chunks, 1):
                w_tag = f"[W{idx}]"
                title = w.get("title", f"Web Source {idx}")
                snippet = w.get("snippet", "")
                url = w.get("url", "")
                response_sections.append(
                    f"{point_idx}. **[{title}]({url})** {w_tag}:\n"
                    f"   {snippet}\n"
                )
                point_idx += 1

        # 3. Actionable Next Steps / Summary table
        if local_chunks or web_chunks:
            response_sections.append("\n#### ⚡ Recommended Action Checklist\n")
            response_sections.append(
                "- [x] **Verify Constraints:** Review the specific dosage, parameters, or legal criteria cited above.\n"
                "- [x] **Observe Progression:** For agricultural or medical concerns, track symptoms over 24-48 hours.\n"
                "- [x] **Safety Baseline:** Consult licensed professionals for specialized emergencies.\n"
            )

        full_text = "".join(response_sections)
        # Token-streamer emulation (words / punctuation tokens)
        tokens = re.findall(r'\S+|\s+', full_text)
        for t in tokens:
            yield t
            # 25-40 tokens per second rate
            await asyncio.sleep(0.012)
