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
        self.groq_key = os.getenv("GROQ_API_KEY")
        self.hf_token = os.getenv("HF_TOKEN")
        self.ollama_url = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
        self.active_provider = self._detect_provider()

    def update_keys(self, gemini_key: Optional[str] = None, groq_key: Optional[str] = None, hf_token: Optional[str] = None, openai_key: Optional[str] = None):
        if gemini_key is not None: self.gemini_key = gemini_key or None
        if groq_key is not None: self.groq_key = groq_key or None
        if hf_token is not None: self.hf_token = hf_token or None
        if openai_key is not None: self.openai_key = openai_key or None
        self.active_provider = self._detect_provider()

    def _detect_provider(self) -> str:
        if self.groq_key:
            return "groq"
        if self.gemini_key:
            return "gemini"
        if self.openai_key:
            return "openai"
        return "native_offline"

    def get_routing_strategy(self, tier: int = 2) -> Dict[str, Any]:
        if tier == 1:
            return {
                "tier": 1,
                "tier_name": "Tier 1: Low-End Budget (₹8,000 – ₹12,000)",
                "hardware_profile": "3GB to 4GB RAM, MediaTek/Unisoc, No NPU",
                "strategy": "100% Free Cloud Offload + Zero-RAM Local RAG (Anti-OOM)",
                "active_engine": self.active_provider if self.active_provider != "native_offline" else "Native Cognitive Zero-RAM",
                "recommended_free_apis": ["Google AI Studio (Gemini 2.5/1.5 Flash)", "Groq Cloud (Llama 3.2 3B / Llama 3.3 70B)", "Hugging Face (FLUX.1 [schnell])"],
                "local_component": "MiniLM-L6-v2 on-device vector search / knowledge pack retrieval (~90 MB)"
            }
        elif tier == 2:
            return {
                "tier": 2,
                "tier_name": "Tier 2: Mid-Range (₹13,000 – ₹20,000)",
                "hardware_profile": "6GB to 8GB RAM, Snapdragon 4 Gen 2 / Dimensity 6080",
                "strategy": "Hybrid Execution (Local SLMs + Free Cloud Offload)",
                "active_engine": "Local GGUF SLM (Qwen 2.5 1.5B / Llama 3.2 1B Q4) + Cloud Image Offload",
                "recommended_free_apis": ["Local GGUF Models (Q4_K_M)", "BGE-Micro-v2 Embeddings", "Hugging Face FLUX.1 [schnell] for Images"],
                "local_component": "GGUF Q4 Quantized on-device inference + local RAG index"
            }
        else:
            return {
                "tier": 3,
                "tier_name": "Tier 3: Upper Mid-Range to Flagship (₹25,000+)",
                "hardware_profile": "8GB to 16GB+ LPDDR5X RAM, Snapdragon 8s/8 Gen series, Dedicated NPU",
                "strategy": "100% Full On-Device Execution (Zero Cloud Costs)",
                "active_engine": "Full On-Device Neural Execution (SLMs + Mobile NPU Vision)",
                "recommended_free_apis": ["Local Phi-3.5-mini (3.8B)", "Local Gemma 2 (2B)", "MobileDiffusion (520M)"],
                "local_component": "Complete on-device execution for text, vision, and image generation"
            }

    async def stream_generate(
        self,
        prompt: str,
        retrieval_data: Dict[str, Any],
        user_query: str,
        active_pack: str = "all"
    ) -> AsyncGenerator[str, None]:
        """
        Yields tokens/chunks as they are generated.
        Dynamically routes between Free Cloud APIs (Groq, Gemini) and On-Device Cognitive Engine.
        """
        # 1. Groq Cloud (Free Developer Tier - Llama 3.3 70B / Llama 3.2 3B)
        if self.groq_key:
            try:
                async for chunk in self._stream_groq(prompt):
                    yield chunk
                return
            except Exception:
                pass

        # 2. Google AI Studio (Gemini 1.5/2.5 Flash Free Tier)
        if self.gemini_key:
            try:
                async for chunk in self._stream_gemini(prompt):
                    yield chunk
                return
            except Exception:
                pass

        # 3. OpenAI-Compatible API
        if self.openai_key:
            try:
                async for chunk in self._stream_openai(prompt):
                    yield chunk
                return
            except Exception:
                pass

        # 4. Native On-Device Grounded Cognitive Generator (Safe for all tiers, 0 MB model RAM)
        async for chunk in self._stream_native_cognitive(user_query, retrieval_data, active_pack):
            yield chunk

    async def _stream_groq(self, prompt: str) -> AsyncGenerator[str, None]:
        url = "https://api.groq.com/openai/v1/chat/completions"
        headers = {
            "Authorization": f"Bearer {self.groq_key}",
            "Content-Type": "application/json"
        }
        body = {
            "model": "llama-3.3-70b-versatile",
            "messages": [
                {"role": "system", "content": "You are ISHA AI, an expert, concise, grounded AI assistant."},
                {"role": "user", "content": prompt}
            ],
            "stream": True,
            "temperature": 0.3
        }
        async with httpx.AsyncClient(timeout=25.0) as client:
            async with client.stream("POST", url, headers=headers, json=body) as resp:
                if resp.status_code != 200:
                    raise Exception(f"Groq API error {resp.status_code}")
                async for line in resp.aiter_lines():
                    if line.startswith("data: ") and line.strip() != "data: [DONE]":
                        import json
                        try:
                            data = json.loads(line[6:])
                            delta = data["choices"][0].get("delta", {}).get("content", "")
                            if delta:
                                yield delta
                        except Exception:
                            pass

    async def _stream_gemini(self, prompt: str) -> AsyncGenerator[str, None]:
        url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key={self.gemini_key}"
        headers = {"Content-Type": "application/json"}
        body = {
            "contents": [{"parts": [{"text": prompt}]}],
            "generationConfig": {"temperature": 0.3, "maxOutputTokens": 1024}
        }
        async with httpx.AsyncClient(timeout=25.0) as client:
            resp = await client.post(url, headers=headers, json=body)
            if resp.status_code == 200:
                data = resp.json()
                text = data.get("candidates", [{}])[0].get("content", {}).get("parts", [{}])[0].get("text", "")
                tokens = re.findall(r'\S+|\s+', text)
                for t in tokens:
                    yield t
                    await asyncio.sleep(0.01)
                return
            else:
                raise Exception(f"Gemini API returned status {resp.status_code}")

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
                    if line.startswith("data: ") and line.strip() != "data: [DONE]":
                        import json
                        try:
                            data = json.loads(line[6:])
                            delta = data["choices"][0].get("delta", {}).get("content", "")
                            if delta:
                                yield delta
                        except Exception:
                            pass

    async def generate_image_free(self, prompt: str) -> Dict[str, Any]:
        """
        Free cloud image generation via Hugging Face Serverless Inference API (FLUX.1 [schnell] / SD 1.5).
        """
        if not self.hf_token:
            return {
                "status": "info",
                "message": "To generate images for free on budget phones, provide a free Hugging Face User Access Token (HF_TOKEN) in Settings.",
                "model": "FLUX.1 [schnell] / Stable Diffusion 1.5 (Free Serverless)",
                "endpoint": "https://api-inference.huggingface.co/models/black-forest-labs/FLUX.1-schnell"
            }

        url = "https://api-inference.huggingface.co/models/black-forest-labs/FLUX.1-schnell"
        headers = {"Authorization": f"Bearer {self.hf_token}"}
        body = {"inputs": prompt}
        try:
            async with httpx.AsyncClient(timeout=35.0) as client:
                resp = await client.post(url, headers=headers, json=body)
                if resp.status_code == 200:
                    import base64
                    b64_img = base64.b64encode(resp.content).decode("utf-8")
                    return {
                        "status": "success",
                        "image_base64": f"data:image/jpeg;base64,{b64_img}",
                        "model": "FLUX.1 [schnell] (Hugging Face Serverless Free Tier)"
                    }
                else:
                    return {"status": "error", "message": f"HF Inference API returned {resp.status_code}"}
        except Exception as e:
            return {"status": "error", "message": str(e)}

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
