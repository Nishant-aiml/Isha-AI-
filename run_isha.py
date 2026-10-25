"""
ISHA AI - Standalone Production Launcher.
Starts the high-performance local cognitive backend and launches the Google Stitch UI in your default browser.
"""

import sys
import os
import webbrowser
import threading
import time
import uvicorn

# Ensure the workspace directory is in sys.path
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
if BASE_DIR not in sys.path:
    sys.path.insert(0, BASE_DIR)


def open_browser(port: int = 8000):
    time.sleep(1.2)
    url = f"http://127.0.0.1:{port}"
    print(f"\n[ISHA AI] Launching Google Stitch Canvas UI at: {url}\n")
    webbrowser.open(url)


def main():
    port = int(os.environ.get("PORT", 8000))
    host = os.environ.get("HOST", "127.0.0.1")

    print("=" * 65)
    print("      ISHA AI — Intelligent Speech & Haptic Assistant")
    print("      Cognitive Runtime • Zero Cloud Dependency • Online Live RAG")
    print("=" * 65)
    print(f" * Local Server: http://{host}:{port}")
    print(f" * Knowledge Base: Pre-loaded (Kisan Saathi, Swasthya, Bharat Vidhi, Shiksha)")
    print(f" * Online Live RAG: DuckDuckGo & Wikipedia Connectors Active")
    print("=" * 65)

    threading.Thread(target=open_browser, args=(port,), daemon=True).start()

    uvicorn.run(
        "backend.app:app",
        host=host,
        port=port,
        log_level="info",
        reload=False
    )


if __name__ == "__main__":
    main()
