"""
ISHA AI — Native Desktop Application Launcher.
Launches the high-performance local cognitive backend and opens the interface
as a dedicated native desktop window (frameless app window via Chromium/Edge App Mode or PyWebView).
"""

import sys
import os
import subprocess
import threading
import time
import socket
import uvicorn

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
if BASE_DIR not in sys.path:
    sys.path.insert(0, BASE_DIR)


def find_available_port(default_port: int = 8000) -> int:
    port = default_port
    while port < default_port + 20:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind(('127.0.0.1', port))
                return port
        except OSError:
            port += 1
    return default_port


def launch_native_desktop_window(port: int):
    """
    Launches interface in dedicated App Mode (Chromium/Edge/Chrome)
    so it renders as a standalone desktop application window with no browser tabs or URL bars.
    """
    time.sleep(1.0)
    url = f"http://127.0.0.1:{port}"

    # Try PyWebView if installed
    try:
        import webview
        print(f"[Desktop] Launching via PyWebView native desktop window...")
        webview.create_window("Isha AI Studio v2.0", url, width=1280, height=820, resizable=True)
        webview.start()
        return
    except ImportError:
        pass

    # Windows Native Chromium / Edge App Window Mode
    if sys.platform == "win32":
        edge_paths = [
            os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft\Edge\Application\msedge.exe"),
            os.path.expandvars(r"%ProgramFiles%\Microsoft\Edge\Application\msedge.exe"),
            os.path.expandvars(r"%ProgramFiles%\Google\Chrome\Application\chrome.exe"),
            os.path.expandvars(r"%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe"),
            os.path.expandvars(r"%LocalAppData%\Google\Chrome\Application\chrome.exe")
        ]
        for exe in edge_paths:
            if os.path.exists(exe):
                print(f"[Desktop] Launching Standalone Desktop Window: {exe} --app={url}")
                os.system(f'start "" "{exe}" --app={url} --window-size=1280,840')
                return
        try:
            os.startfile(url)
            return
        except Exception:
            pass

    # Fallback to standard system browser
    import webbrowser
    print(f"[Desktop] Opening browser canvas: {url}")
    webbrowser.open(url)


def main():
    port = find_available_port(8000)
    host = "127.0.0.1"

    print("=" * 68)
    print("      ISHA AI — Desktop Studio Edition v2.0")
    print("      All-in-One AI Fiesta Architecture • Dual Cloud/Offline Engine")
    print("=" * 68)
    print(f" * Local Desktop Server: http://{host}:{port}")
    print(f" * Pay-As-You-Go Meter: Active (Free Offline Bypass Enabled)")
    print(f" * AI Studios: Chat Compare, Image, Deck, Web Builder, Video")
    print("=" * 68)

    threading.Thread(target=launch_native_desktop_window, args=(port,), daemon=True).start()

    uvicorn.run(
        "backend.app:app",
        host=host,
        port=port,
        log_level="warning",
        reload=False
    )


if __name__ == "__main__":
    main()
