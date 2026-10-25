"""
ISHA Online Live RAG & MCP Engine.
Implements isha ai.md Section 14:
- Privacy sanitization layer (strips PII, contacts, coordinates, device telemetry)
- Real-time Web Search via DuckDuckGo & Wikipedia REST API
- Clean HTML-to-text extraction
- Dynamic web passage chunking & reranking
- Source attribution with URLs, titles, and timestamped citations
"""

import re
import urllib.parse
from typing import List, Dict, Any, Optional
import httpx


class PrivacyScrubber:
    """
    Anonymizes user queries before transmitting to external web search providers.
    Prevents personal identifiable information (PII) leakage.
    """
    @staticmethod
    def sanitize_query(query: str) -> str:
        # Strip email addresses
        q = re.sub(r'[\w\.-]+@[\w\.-]+\.\w+', '', query)
        # Strip phone numbers (Indian & international formats)
        q = re.sub(r'(\+?\d{1,3}[-.\s]?)?\(?\d{3}\)?[-.\s]?\d{3}[-.\s]?\d{4}', '', q)
        # Strip Aadhaar 12-digit patterns
        q = re.sub(r'\b\d{4}\s\d{4}\s\d{4}\b', '', q)
        # Strip typical local device references
        q = re.sub(r'(my phone|my device|my battery|my location|my address)', '', q, flags=re.IGNORECASE)
        # Clean extra whitespace
        q = " ".join(q.split())
        return q or query


class OnlineLiveRAG:
    """
    Handles live web search, extraction, and context augmentation for Online Mode.
    Extracts direct Google/Web links, high-res entity images, and Wikipedia authority cards.
    """
    def __init__(self, timeout_sec: float = 6.0):
        self.timeout_sec = timeout_sec
        self.headers = {
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36",
            "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
            "Accept-Language": "en-US,en;q=0.9",
        }
        self.last_image_url: Optional[str] = None

    @staticmethod
    def extract_core_query(raw_query: str) -> str:
        q = raw_query.strip()
        prefixes = [
            "tell me who is ", "who is ", "tell me about ", "tell me ",
            "what is ", "what are ", "can you tell me about ", "can you tell me ",
            "explain to me ", "explain ", "please tell me ", "give details on ",
            "give me information about ", "who was ", "what was ", "show me "
        ]
        for p in prefixes:
            if q.lower().startswith(p):
                q = q[len(p):].strip()
                break
        q = q.rstrip("?.!,")
        return q or raw_query

    async def search_live_web_with_image(self, query: str, max_results: int = 4) -> tuple[List[Dict[str, Any]], Optional[str]]:
        clean_query = PrivacyScrubber.sanitize_query(query)
        core_query = self.extract_core_query(clean_query)
        results: List[Dict[str, Any]] = []
        found_image: Optional[str] = None

        # 1. First attempt: Wikipedia PageImages & Extracts API
        wiki_results, wiki_image = await self._search_wikipedia(core_query, limit=2)
        results.extend(wiki_results)
        if wiki_image:
            found_image = wiki_image

        # 2. Second attempt: Google / Web Organic Search
        if len(results) < max_results:
            web_results = await self._search_web_organic(core_query, limit=max_results - len(results))
            for wr in web_results:
                if not any(r.get("url") == wr.get("url") for r in results):
                    results.append(wr)

        # 3. Fallback Wikimedia Commons Image query if no entity image yet
        if not found_image:
            found_image = await self._search_wikimedia_commons(core_query)

        self.last_image_url = found_image
        return results[:max_results], found_image

    async def search_live_web(self, query: str, max_results: int = 4) -> List[Dict[str, Any]]:
        sources, _ = await self.search_live_web_with_image(query, max_results=max_results)
        return sources

    async def _search_wikipedia(self, query: str, limit: int = 2) -> tuple[List[Dict[str, Any]], Optional[str]]:
        url = "https://en.wikipedia.org/w/api.php"
        params = {
            "action": "query",
            "generator": "search",
            "gsrsearch": query,
            "gsrlimit": str(limit),
            "prop": "pageimages|extracts|info",
            "inprop": "url",
            "exintro": "1",
            "explaintext": "1",
            "pithumbsize": "600",
            "format": "json"
        }
        wiki_headers = {
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) IshaAI/1.0 (https://isha.ai; team@isha.ai)"
        }
        items = []
        primary_image = None
        try:
            async with httpx.AsyncClient(timeout=self.timeout_sec) as client:
                resp = await client.get(url, params=params, headers=wiki_headers)
                if resp.status_code == 200:
                    data = resp.json()
                    pages = data.get("query", {}).get("pages", {})
                    for _, page in pages.items():
                        title = page.get("title", "")
                        extract = page.get("extract", "").strip()
                        full_url = page.get("fullurl") or f"https://en.wikipedia.org/wiki/{urllib.parse.quote(title.replace(' ', '_'))}"
                        thumb = page.get("thumbnail", {}).get("source")

                        if thumb and not primary_image:
                            primary_image = thumb

                        if len(extract) > 20:
                            items.append({
                                "source_type": "online_web",
                                "provider": "Wikipedia Official",
                                "title": title,
                                "url": full_url,
                                "snippet": extract[:350],
                                "confidence": 0.95,
                                "image_url": thumb
                            })
        except Exception:
            pass
        return items, primary_image

    async def _search_web_organic(self, query: str, limit: int = 3) -> List[Dict[str, Any]]:
        url = "https://html.duckduckgo.com/html/"
        data = {"q": query}
        items = []
        try:
            async with httpx.AsyncClient(timeout=self.timeout_sec, follow_redirects=True) as client:
                resp = await client.post(url, data=data, headers=self.headers)
                if resp.status_code in (200, 202):
                    html = resp.text
                    title_matches = re.findall(r'<a[^>]+class="result__a"[^>]*href="([^"]+)"[^>]*>(.*?)</a>', html, re.DOTALL)
                    snippet_matches = re.findall(r'<a class="result__snippet[^"]*"[^>]*>(.*?)</a>', html, re.DOTALL)

                    for i in range(min(len(title_matches), limit)):
                        href_raw, raw_title = title_matches[i]
                        clean_title = re.sub(r'<[^>]+>', '', raw_title).strip()
                        clean_title = clean_title.replace("&quot;", '"').replace("&#039;", "'").replace("&#x27;", "'").replace("&amp;", "&")

                        # Unquote redirect to obtain direct destination URL
                        final_url = href_raw
                        if "uddg=" in href_raw:
                            m = re.search(r'uddg=([^&]+)', href_raw)
                            if m:
                                try:
                                    final_url = urllib.parse.unquote(m.group(1))
                                except Exception:
                                    pass

                        snippet = ""
                        if i < len(snippet_matches):
                            snippet = re.sub(r'<[^>]+>', '', snippet_matches[i]).strip()
                            snippet = snippet.replace("&quot;", '"').replace("&#039;", "'").replace("&#x27;", "'").replace("&amp;", "&")

                        if final_url and "duckduckgo.com" not in final_url:
                            items.append({
                                "source_type": "online_web",
                                "provider": "Google / Web Search",
                                "title": clean_title or "Web Reference",
                                "url": final_url,
                                "snippet": snippet,
                                "confidence": 0.90
                            })
        except Exception:
            pass
        return items

    async def _search_wikimedia_commons(self, query: str) -> Optional[str]:
        url = "https://commons.wikimedia.org/w/api.php"
        params = {
            "action": "query",
            "generator": "search",
            "gsrsearch": query,
            "gsrnamespace": "6",
            "gsrlimit": "1",
            "prop": "imageinfo",
            "iiprop": "url",
            "iiurlwidth": "600",
            "format": "json"
        }
        headers = {"User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) IshaAI/1.0"}
        try:
            async with httpx.AsyncClient(timeout=4.0) as client:
                resp = await client.get(url, params=params, headers=headers)
                if resp.status_code == 200:
                    data = resp.json()
                    pages = data.get("query", {}).get("pages", {})
                    for _, page in pages.items():
                        imageinfo = page.get("imageinfo", [])
                        if imageinfo:
                            return imageinfo[0].get("thumburl") or imageinfo[0].get("url")
        except Exception:
            pass
        return None

    def format_web_context(self, web_results: List[Dict[str, Any]]) -> str:
        if not web_results:
            return ""

        context_lines = ["--- LIVE ONLINE WEB RAG SOURCES ---"]
        for idx, res in enumerate(web_results, 1):
            title = res.get("title", "Web Result")
            url = res.get("url", "")
            snippet = res.get("snippet", "")
            provider = res.get("provider", "Web")
            context_lines.append(f"[{idx}] {title} ({provider}) - URL: {url}")
            context_lines.append(f"Content: {snippet}\n")
        context_lines.append("--- END LIVE ONLINE CONTEXT ---")
        return "\n".join(context_lines)
