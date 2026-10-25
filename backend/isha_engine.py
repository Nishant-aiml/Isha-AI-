"""
ISHA AI Engine Orchestrator.
Central runtime coordinator binding all cognitive layers:
- Ingestion Pipeline & Knowledge Packs
- Live Online RAG & MCP Tools
- Context Governor & Grounded Synthesis
- Vision & Multimodal Engine
- Voice & Speech Processing
- Telemetry & Watchdog Monitoring
"""

import time
import os
from typing import Dict, Any, List, Optional, AsyncGenerator
from .rag.hybrid_retriever import HybridRetriever
from .rag.document_parser import parse_document
from .rag.chunker import chunk_text
from .cognition.context_governor import ContextGovernor, ContextBudget
from .cognition.grounded_pipeline import GroundedPipeline
from .inference.model_provider import ModelProvider
from .vision.vision_engine import VisionEngine
from .voice.voice_service import VoiceService
from .telemetry.monitor import TelemetryMonitor


class IshaEngine:
    _instance: Optional["IshaEngine"] = None

    @classmethod
    def get_instance(cls) -> "IshaEngine":
        if cls._instance is None:
            cls._instance = IshaEngine()
        return cls._instance

    def __init__(self):
        self.retriever = HybridRetriever()
        self.governor = ContextGovernor()
        self.grounded_pipeline = GroundedPipeline(self.governor)
        self.model_provider = ModelProvider()
        self.vision = VisionEngine()
        self.voice = VoiceService()
        self.telemetry = TelemetryMonitor()
        self.ingested_documents: Dict[str, Dict[str, Any]] = {}

    async def process_query_stream(
        self,
        query: str,
        pack_id: str = "all",
        use_online_rag: bool = False,
        history: Optional[List[Dict[str, str]]] = None
    ) -> AsyncGenerator[Dict[str, Any], None]:
        """
        Executes complete cognitive loop with streaming token delivery.
        Yields:
        - {"type": "retrieval_meta", "data": ...}
        - {"type": "token", "token": "..."}
        - {"type": "complete", "stats": ...}
        """
        start_time = time.time()
        history = history or []

        # 1. Retrieval Stage (Local Packs + Live Online RAG)
        retrieval_start = time.time()
        retrieval_data = await self.retriever.retrieve(
            query=query,
            pack_id=pack_id,
            use_online_rag=use_online_rag
        )
        retrieval_ms = round((time.time() - retrieval_start) * 1000, 1)

        # Emit retrieval metadata early to the UI
        yield {
            "type": "retrieval_meta",
            "data": {
                "retrieval_time_ms": retrieval_ms,
                "local_matches": retrieval_data["total_local_matches"],
                "web_matches": retrieval_data["total_web_matches"],
                "citations": retrieval_data["citations"],
                "local_chunks": retrieval_data["local_chunks"],
                "web_chunks": retrieval_data["web_chunks"],
                "image_url": retrieval_data.get("image_url")
            }
        }

        # 2. Assemble Grounded Prompt with Context Budgeting
        full_prompt, assembled = self.grounded_pipeline.build_prompt(
            query=query,
            retrieval_data=retrieval_data,
            history=history
        )

        # 3. Stream Inference Tokens
        token_count = 0
        first_token_time = None

        async for token in self.model_provider.stream_generate(
            prompt=full_prompt,
            retrieval_data=retrieval_data,
            user_query=query,
            active_pack=pack_id
        ):
            if first_token_time is None:
                first_token_time = time.time()
            token_count += 1
            yield {"type": "token", "token": token}

        # 4. Record Telemetry
        total_time = time.time() - start_time
        ttft_ms = round(((first_token_time or time.time()) - start_time) * 1000, 1)
        tokens_per_sec = round(token_count / max(0.1, total_time), 1)

        self.telemetry.record_generation(token_count)

        yield {
            "type": "complete",
            "stats": {
                "total_tokens": token_count,
                "total_time_sec": round(total_time, 2),
                "ttft_ms": ttft_ms,
                "tokens_per_sec": tokens_per_sec,
                "citations": retrieval_data["citations"]
            }
        }

    def ingest_document(self, file_bytes: bytes, filename: str, pack_id: str = "user_library") -> Dict[str, Any]:
        """
        Ingests and chunks a file into the knowledge index with hash deduplication.
        """
        pages, doc_hash = parse_document(file_bytes, filename)
        if not pages:
            return {"status": "error", "message": "No text extracted from document"}

        # Duplicate check
        if doc_hash in self.ingested_documents:
            return {
                "status": "already_indexed",
                "message": f"Document '{filename}' already indexed (hash matches).",
                "doc_hash": doc_hash,
                "total_chunks": self.ingested_documents[doc_hash]["chunk_count"]
            }

        total_chunks = 0
        for page in pages:
            page_text = page["text"]
            page_num = page["page_num"]
            chunks = chunk_text(page_text, chunk_size_words=250, overlap_words=32)

            for c in chunks:
                chunk_id = f"{filename}_p{page_num}_c{c['chunk_index']}"
                self.retriever.ingest_custom_chunk(
                    chunk_id=chunk_id,
                    text=c["text"],
                    pack_id=pack_id,
                    metadata={
                        "title": f"{filename} (Page {page_num})",
                        "filename": filename,
                        "page_num": page_num,
                        "word_count": c["word_count"]
                    }
                )
                total_chunks += 1

        self.ingested_documents[doc_hash] = {
            "filename": filename,
            "pack_id": pack_id,
            "chunk_count": total_chunks,
            "size_bytes": len(file_bytes),
            "pages": len(pages)
        }

        return {
            "status": "success",
            "message": f"Successfully indexed {total_chunks} chunks across {len(pages)} pages.",
            "doc_hash": doc_hash,
            "filename": filename,
            "total_chunks": total_chunks,
            "pages": len(pages)
        }
