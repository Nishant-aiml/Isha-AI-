"""
EdgeFlatVectorIndex - High-efficiency local vector index with BM25-lite fallback.
Directly implements the ISHA C++ EdgeFlatVectorIndex architecture in Python:
- Normalized cosine similarity vector search
- Score thresholds (default 0.35)
- BM25-lite keyword search fallback for low-latency or thermal throttling scenarios
- Pack isolation & multi-pack query support
- JSON / Binary persistence
"""

import math
import re
import json
import os
from typing import List, Dict, Any, Optional, Tuple
from dataclasses import dataclass, asdict


@dataclass
class IndexedChunk:
    chunk_id: str
    text: str
    pack_id: str
    embedding: List[float]
    metadata: Optional[Dict[str, Any]] = None


@dataclass
class SearchResult:
    chunk_id: str
    text: str
    pack_id: str
    score: float
    metadata: Optional[Dict[str, Any]] = None


class EdgeFlatVectorIndex:
    """
    On-device vector index designed for low compute overhead and zero external dependencies.
    """
    def __init__(self, embedding_dim: int = 64):
        self.embedding_dim = embedding_dim
        self.chunks: List[IndexedChunk] = []
        self._doc_frequencies: Dict[str, int] = {}
        self._pack_counts: Dict[str, int] = {}

    def add_chunk(self, chunk: IndexedChunk) -> None:
        # Normalize embedding vector
        chunk.embedding = self._normalize(chunk.embedding)
        self.chunks.append(chunk)
        self._pack_counts[chunk.pack_id] = self._pack_counts.get(chunk.pack_id, 0) + 1
        
        # Update BM25 frequencies
        tokens = set(self._tokenize(chunk.text))
        for token in tokens:
            self._doc_frequencies[token] = self._doc_frequencies.get(token, 0) + 1

    def clear_pack(self, pack_id: str) -> None:
        self.chunks = [c for c in self.chunks if c.pack_id != pack_id]
        self._pack_counts.pop(pack_id, None)
        self._rebuild_bm25_frequencies()

    def _rebuild_bm25_frequencies(self) -> None:
        self._doc_frequencies.clear()
        for c in self.chunks:
            tokens = set(self._tokenize(c.text))
            for token in tokens:
                self._doc_frequencies[token] = self._doc_frequencies.get(token, 0) + 1

    def chunk_count(self) -> int:
        return len(self.chunks)

    def chunk_count_for_pack(self, pack_id: str) -> int:
        return self._pack_counts.get(pack_id, 0)

    def get_packs(self) -> List[str]:
        return list(self._pack_counts.keys())

    def _normalize(self, vec: List[float]) -> List[float]:
        norm = math.sqrt(sum(x * x for x in vec))
        if norm < 1e-8:
            return [0.0] * len(vec)
        return [x / norm for x in vec]

    def _cosine_similarity(self, a: List[float], b: List[float]) -> float:
        if len(a) != len(b):
            return 0.0
        return sum(x * y for x, y in zip(a, b))

    def _tokenize(self, text: str) -> List[str]:
        # Simple whitespace/punctuation-clean tokenization
        cleaned = re.sub(r'[^\w\s]', ' ', text.lower())
        return [w for w in cleaned.split() if len(w) > 2]

    def search_vector(
        self,
        query_embedding: List[float],
        pack_id: Optional[str] = None,
        limit: int = 4,
        score_threshold: float = 0.30
    ) -> List[SearchResult]:
        q_norm = self._normalize(query_embedding)
        results: List[SearchResult] = []

        for c in self.chunks:
            if pack_id and pack_id != "all" and c.pack_id != pack_id:
                continue
            sim = self._cosine_similarity(q_norm, c.embedding)
            if sim >= score_threshold:
                results.append(SearchResult(
                    chunk_id=c.chunk_id,
                    text=c.text,
                    pack_id=c.pack_id,
                    score=float(sim),
                    metadata=c.metadata
                ))

        results.sort(key=lambda r: r.score, reverse=True)
        return results[:limit]

    def search_bm25(
        self,
        query: str,
        pack_id: Optional[str] = None,
        limit: int = 4
    ) -> List[SearchResult]:
        """
        Lightweight BM25 keyword search fallback.
        """
        query_tokens = self._tokenize(query)
        if not query_tokens or not self.chunks:
            return []

        num_docs = len(self.chunks)
        avg_doc_len = sum(len(self._tokenize(c.text)) for c in self.chunks) / max(1, num_docs)
        k1 = 1.5
        b = 0.75

        scores: List[Tuple[IndexedChunk, float]] = []

        for c in self.chunks:
            if pack_id and pack_id != "all" and c.pack_id != pack_id:
                continue
            doc_tokens = self._tokenize(c.text)
            doc_len = len(doc_tokens)
            if doc_len == 0:
                continue

            score = 0.0
            term_counts: Dict[str, int] = {}
            for t in doc_tokens:
                term_counts[t] = term_counts.get(t, 0) + 1

            for qt in query_tokens:
                if qt in term_counts:
                    tf = term_counts[qt]
                    df = self._doc_frequencies.get(qt, 1)
                    idf = math.log(1.0 + (num_docs - df + 0.5) / (df + 0.5))
                    num = tf * (k1 + 1.0)
                    denom = tf + k1 * (1.0 - b + b * (doc_len / avg_doc_len))
                    score += idf * (num / denom)

            if score > 0.1:
                # Normalize BM25 score roughly into [0, 1] range for unified presentation
                norm_score = min(0.99, score / (score + 2.0))
                scores.append((c, norm_score))

        scores.sort(key=lambda x: x[1], reverse=True)
        return [
            SearchResult(
                chunk_id=c.chunk_id,
                text=c.text,
                pack_id=c.pack_id,
                score=round(s, 3),
                metadata=c.metadata
            )
            for c, s in scores[:limit]
        ]

    def hybrid_search(
        self,
        query: str,
        query_embedding: List[float],
        pack_id: Optional[str] = None,
        limit: int = 4,
        threshold: float = 0.30
    ) -> List[SearchResult]:
        # Vector search first
        v_results = self.search_vector(query_embedding, pack_id=pack_id, limit=limit, score_threshold=threshold)
        
        # If vector matches are insufficient, fall back / augment with BM25
        if len(v_results) < limit:
            bm25_results = self.search_bm25(query, pack_id=pack_id, limit=limit)
            seen_ids = {r.chunk_id for r in v_results}
            for bm in bm25_results:
                if bm.chunk_id not in seen_ids:
                    v_results.append(bm)
                    seen_ids.add(bm.chunk_id)
                    if len(v_results) >= limit:
                        break

        v_results.sort(key=lambda r: r.score, reverse=True)
        return v_results[:limit]

    def save_to_file(self, file_path: str) -> bool:
        os.makedirs(os.path.dirname(os.path.abspath(file_path)), exist_ok=True)
        data = {
            "dim": self.embedding_dim,
            "chunks": [asdict(c) for c in self.chunks]
        }
        with open(file_path, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False)
        return True

    def load_from_file(self, file_path: str) -> bool:
        if not os.path.exists(file_path):
            return False
        with open(file_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        self.embedding_dim = data.get("dim", self.embedding_dim)
        self.chunks = []
        for c_dict in data.get("chunks", []):
            self.add_chunk(IndexedChunk(
                chunk_id=c_dict["chunk_id"],
                text=c_dict["text"],
                pack_id=c_dict["pack_id"],
                embedding=c_dict["embedding"],
                metadata=c_dict.get("metadata")
            ))
        return True
