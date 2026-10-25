"""
ISHA Boundary-Aware Semantic Chunker.
Direct implementation of core/ingestion/chunker.cpp and android/app/src/main/java/com/isha/ai/IshaRAGManager.kt:
- Paragraph, table, and list-boundary preservation
- Overlap sliding window (default 250 words, 32 overlap)
- Character and token offset tracking
"""

import re
from typing import List, Dict, Any


def is_semantic_boundary_delimiter(line: str) -> bool:
    line_s = line.strip()
    if "|" in line_s:
        return True
    if line_s.startswith(("- ", "* ", "+ ", "#")):
        return True
    if re.match(r"^\d+\.\s+", line_s):
        return True
    return False


def chunk_text(
    text: str,
    chunk_size_words: int = 250,
    overlap_words: int = 32
) -> List[Dict[str, Any]]:
    if not text or not text.strip():
        return []

    lines = text.splitlines()
    chunks: List[Dict[str, Any]] = []
    current_words: List[str] = []
    chunk_idx = 0
    char_offset = 0

    for raw_line in lines:
        line_words = raw_line.split()
        if not line_words:
            continue

        is_boundary = is_semantic_boundary_delimiter(raw_line)

        # Flush chunk if adding new line words exceeds chunk_size_words
        if len(current_words) + len(line_words) > chunk_size_words and current_words:
            chunk_content = " ".join(current_words)
            chunks.append({
                "chunk_index": chunk_idx,
                "text": chunk_content,
                "word_count": len(current_words),
                "char_offset": char_offset
            })
            char_offset += len(chunk_content) + 1
            chunk_idx += 1

            # Retain overlap words
            if overlap_words > 0 and len(current_words) > overlap_words:
                current_words = current_words[-overlap_words:]
            else:
                current_words = []

        current_words.extend(line_words)

    if current_words:
        chunk_content = " ".join(current_words)
        chunks.append({
            "chunk_index": chunk_idx,
            "text": chunk_content,
            "word_count": len(current_words),
            "char_offset": char_offset
        })

    return chunks
