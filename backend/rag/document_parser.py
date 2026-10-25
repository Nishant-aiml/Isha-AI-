"""
IshaDocumentParser - Multi-format stream document parser with SHA-256 deduplication.
Supports PDF, DOCX, TXT, MD, CSV, JSON.
Implements page-by-page streaming extraction and hash-based duplicate avoidance.
"""

import os
import hashlib
from typing import List, Dict, Any, Tuple, Optional


def compute_file_hash(file_bytes: bytes) -> str:
    return hashlib.sha256(file_bytes).hexdigest()


def parse_document(file_bytes: bytes, filename: str) -> Tuple[List[Dict[str, Any]], str]:
    """
    Parses document bytes into a list of pages with page numbers and text.
    Returns (pages, sha256_hash).
    """
    doc_hash = compute_file_hash(file_bytes)
    ext = os.path.splitext(filename)[1].lower()
    pages: List[Dict[str, Any]] = []

    if ext == ".pdf":
        pages = _parse_pdf(file_bytes)
    elif ext in [".docx", ".doc"]:
        pages = _parse_docx(file_bytes)
    elif ext == ".csv":
        pages = _parse_csv(file_bytes)
    elif ext == ".json":
        pages = _parse_json(file_bytes)
    else:
        # Default text/markdown
        text = file_bytes.decode("utf-8", errors="replace")
        pages = [{"page_num": 1, "text": text}]

    return pages, doc_hash


def _parse_pdf(file_bytes: bytes) -> List[Dict[str, Any]]:
    pages = []
    # Try pypdf first
    try:
        import io
        import pypdf
        reader = pypdf.PdfReader(io.BytesIO(file_bytes))
        for idx, page in enumerate(reader.pages):
            txt = page.extract_text() or ""
            if txt.strip():
                pages.append({"page_num": idx + 1, "text": txt.strip()})
        if pages:
            return pages
    except Exception:
        pass

    # Try pymupdf (fitz) fallback
    try:
        import fitz
        doc = fitz.open(stream=file_bytes, filetype="pdf")
        for idx, page in enumerate(doc):
            txt = page.get_text() or ""
            if txt.strip():
                pages.append({"page_num": idx + 1, "text": txt.strip()})
        if pages:
            return pages
    except Exception:
        pass

    # Fallback to plain string extraction if all else fails
    raw_str = file_bytes.decode("latin-1", errors="ignore")
    import re
    cleaned = re.sub(r"[^\x20-\x7E\n\r\t]", " ", raw_str)
    return [{"page_num": 1, "text": cleaned[:10000]}]


def _parse_docx(file_bytes: bytes) -> List[Dict[str, Any]]:
    try:
        import io
        import docx
        doc = docx.Document(io.BytesIO(file_bytes))
        full_text = "\n".join([p.text for p in doc.paragraphs if p.text.strip()])
        return [{"page_num": 1, "text": full_text}]
    except Exception as e:
        return [{"page_num": 1, "text": f"Failed to parse docx: {str(e)}"}]


def _parse_csv(file_bytes: bytes) -> List[Dict[str, Any]]:
    text = file_bytes.decode("utf-8", errors="replace")
    lines = text.splitlines()
    header = lines[0] if lines else ""
    # Group every 30 rows into a logical page
    pages = []
    chunk_rows = []
    page_num = 1
    for row in lines[1:]:
        chunk_rows.append(row)
        if len(chunk_rows) >= 30:
            pages.append({"page_num": page_num, "text": header + "\n" + "\n".join(chunk_rows)})
            chunk_rows = []
            page_num += 1
    if chunk_rows:
        pages.append({"page_num": page_num, "text": header + "\n" + "\n".join(chunk_rows)})
    return pages or [{"page_num": 1, "text": text}]


def _parse_json(file_bytes: bytes) -> List[Dict[str, Any]]:
    import json
    try:
        data = json.loads(file_bytes.decode("utf-8", errors="replace"))
        formatted = json.dumps(data, indent=2)
        return [{"page_num": 1, "text": formatted}]
    except Exception:
        return [{"page_num": 1, "text": file_bytes.decode("utf-8", errors="replace")}]
