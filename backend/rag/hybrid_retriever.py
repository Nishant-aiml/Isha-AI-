"""
ISHA Hybrid Retriever.
Seamlessly orchestrates:
- Offline Domain Knowledge Packs (Kisan Saathi, Swasthya, Bharat Core, Shiksha)
- Custom Ingested User Documents (PDF, TXT, DOCX, etc.)
- Online Live Web Search RAG (Wikipedia, DuckDuckGo)
- Cosine Vector Similarity + BM25-lite keyword fallback
"""

import os
from typing import List, Dict, Any, Optional
from .index import EdgeFlatVectorIndex, IndexedChunk, SearchResult
from .embeddings import IshaEmbeddingEngine
from .knowledge_packs import DOMAIN_PACKS
from .online_rag import OnlineLiveRAG


class HybridRetriever:
    def __init__(self, data_dir: str = "backend/data"):
        self.data_dir = data_dir
        os.makedirs(self.data_dir, exist_ok=True)
        self.index_path = os.path.join(self.data_dir, "knowledge_index.json")

        self.embedder = IshaEmbeddingEngine(dimensions=64)
        self.index = EdgeFlatVectorIndex(embedding_dim=64)
        self.online_rag = OnlineLiveRAG()

        # Initialize and preload knowledge packs
        self._initialize_index()

    def _initialize_index(self) -> None:
        # Load from disk if exists
        loaded = False
        if os.path.exists(self.index_path):
            loaded = self.index.load_from_file(self.index_path)

        # Preload built-in domain packs if missing
        for pack_id, pack_data in DOMAIN_PACKS.items():
            if self.index.chunk_count_for_pack(pack_id) == 0:
                for chunk in pack_data["chunks"]:
                    embedding = self.embedder.generate_embedding(chunk["text"])
                    self.index.add_chunk(IndexedChunk(
                        chunk_id=chunk["id"],
                        text=chunk["text"],
                        pack_id=pack_id,
                        embedding=embedding,
                        metadata={"title": chunk["title"], "pack_title": pack_data["title"]}
                    ))

        # Save synced index
        self.index.save_to_file(self.index_path)

    async def retrieve(
        self,
        query: str,
        pack_id: Optional[str] = "all",
        use_online_rag: bool = False,
        limit: int = 4,
        threshold: float = 0.28
    ) -> Dict[str, Any]:
        """
        Executes hybrid retrieval across local vector index and optional online live web.
        """
        query_embedding = self.embedder.generate_embedding(query)

        # 1. Local Offline Search (Vector + BM25-lite)
        local_results: List[SearchResult] = self.index.hybrid_search(
            query=query,
            query_embedding=query_embedding,
            pack_id=pack_id if pack_id != "all" else None,
            limit=limit,
            threshold=threshold
        )

        formatted_local = []
        for r in local_results:
            formatted_local.append({
                "chunk_id": r.chunk_id,
                "text": r.text,
                "pack_id": r.pack_id,
                "score": round(r.score, 3),
                "title": (r.metadata or {}).get("title", f"Document Chunk ({r.chunk_id})"),
                "source_type": "offline_knowledge_pack"
            })

        # 2. Live Online Web RAG (if enabled)
        web_results: List[Dict[str, Any]] = []
        image_url: Optional[str] = None
        if use_online_rag:
            web_results, image_url = await self.online_rag.search_live_web_with_image(query, max_results=3)

        # 3. Assemble combined context and citations
        retrieval_text_blocks = []
        citations = []

        # Local citations
        for idx, item in enumerate(formatted_local, 1):
            title = item.get("title", "Offline Reference")
            pack = item.get("pack_id", "local")
            retrieval_text_blocks.append(f"[Local Source {idx} | Pack: {pack}] {title}:\n{item['text']}")
            citations.append({
                "citation_id": f"L{idx}",
                "type": "local",
                "title": title,
                "pack_id": pack,
                "score": item["score"],
                "snippet": item["text"][:160] + "..."
            })

        # Web citations
        for idx, item in enumerate(web_results, 1):
            title = item.get("title", "Web Source")
            url = item.get("url", "")
            snippet = item.get("snippet", "")
            retrieval_text_blocks.append(f"[Web Source {idx}] {title} ({url}):\n{snippet}")
            citations.append({
                "citation_id": f"W{idx}",
                "type": "web",
                "title": title,
                "url": url,
                "score": item.get("confidence", 0.85),
                "snippet": snippet[:160] + "..."
            })

        combined_context = "\n\n".join(retrieval_text_blocks)

        return {
            "query": query,
            "pack_id": pack_id,
            "use_online_rag": use_online_rag,
            "total_local_matches": len(formatted_local),
            "total_web_matches": len(web_results),
            "context_text": combined_context,
            "local_chunks": formatted_local,
            "web_chunks": web_results,
            "citations": citations,
            "image_url": image_url
        }

    def ingest_custom_chunk(
        self,
        chunk_id: str,
        text: str,
        pack_id: str = "user_library",
        metadata: Optional[Dict[str, Any]] = None
    ) -> None:
        embedding = self.embedder.generate_embedding(text)
        self.index.add_chunk(IndexedChunk(
            chunk_id=chunk_id,
            text=text,
            pack_id=pack_id,
            embedding=embedding,
            metadata=metadata
        ))
        self.index.save_to_file(self.index_path)

    def get_packs_overview(self) -> List[Dict[str, Any]]:
        packs = []
        for pack_id, pack_info in DOMAIN_PACKS.items():
            packs.append({
                "id": pack_id,
                "title": pack_info["title"],
                "description": pack_info["description"],
                "icon": pack_info["icon"],
                "chunks_count": self.index.chunk_count_for_pack(pack_id),
                "is_system": True
            })

        user_count = self.index.chunk_count_for_pack("user_library")
        packs.append({
            "id": "user_library",
            "title": "Custom Document Library",
            "description": "User-uploaded documents, PDFs, manuals, and custom notes.",
            "icon": "📁",
            "chunks_count": user_count,
            "is_system": False
        })

        return packs
