#!/usr/bin/env python3
"""
edit_css_icon.py — Local pixel editor launcher for SSB64 CSS icons.

Usage:
    python3 tools/edit_css_icon.py [path/to/icon.png]

Defaults to port/assets/css_icons/fd_small.png relative to the repo root
(detected as the directory two levels up from this script).

Boots an HTTP server on 127.0.0.1:<random port>, injects the PNG as a
base64 data URL into the editor HTML, opens it in the default browser,
and stays alive until Ctrl-C or the editor sends POST /quit.
"""

import base64
import io
import json
import os
import pathlib
import random
import signal
import socket
import sys
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, HTTPServer

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
HTML_PATH = SCRIPT_DIR / "edit_css_icon.html"
DEFAULT_PNG = REPO_ROOT / "port" / "assets" / "css_icons" / "fd_small.png"


def resolve_target(argv: list[str]) -> pathlib.Path:
    if len(argv) > 1:
        p = pathlib.Path(argv[1])
        if not p.is_absolute():
            p = pathlib.Path.cwd() / p
        return p.resolve()
    return DEFAULT_PNG


# ---------------------------------------------------------------------------
# HTTP handler
# ---------------------------------------------------------------------------
class EditorHandler(BaseHTTPRequestHandler):
    # Injected by main():
    html_bytes: bytes = b""
    png_path: pathlib.Path = DEFAULT_PNG
    shutdown_event: threading.Event = None

    def log_message(self, fmt, *args):  # suppress access log spam
        pass

    def log_error(self, fmt, *args):
        sys.stderr.write(f"[server] ERROR: {fmt % args}\n")

    # ---- GET ---------------------------------------------------------------

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            self._send(200, "text/html; charset=utf-8", self.html_bytes)
        elif self.path == "/favicon.ico":
            self._send(204, "text/plain", b"")
        else:
            self._send(404, "text/plain", b"Not found")

    # ---- POST --------------------------------------------------------------

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length) if length else b""

        if self.path == "/save":
            self._handle_save(body)
        elif self.path == "/quit":
            self._send(200, "application/json", json.dumps({"ok": True}).encode())
            # Trigger shutdown from a daemon thread so we can still reply.
            t = threading.Thread(target=self._do_shutdown, daemon=True)
            t.start()
        else:
            self._send(404, "text/plain", b"Unknown endpoint")

    def _handle_save(self, body: bytes):
        try:
            payload = json.loads(body)
            data_url: str = payload["dataURL"]
            # data URL format: "data:image/png;base64,<b64>"
            header, b64 = data_url.split(",", 1)
            png_bytes = base64.b64decode(b64)
            self.png_path.write_bytes(png_bytes)
            sys.stderr.write(f"[server] Saved {len(png_bytes)} bytes → {self.png_path}\n")
            self._send(200, "application/json", json.dumps({"ok": True}).encode())
        except Exception as exc:
            sys.stderr.write(f"[server] Save error: {exc}\n")
            self._send(500, "application/json",
                       json.dumps({"ok": False, "error": str(exc)}).encode())

    def _do_shutdown(self):
        sys.stderr.write("[server] Shutdown requested.\n")
        self.shutdown_event.set()

    def _send(self, code: int, ctype: str, body: bytes):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)


# ---------------------------------------------------------------------------
# HTML injection
# ---------------------------------------------------------------------------
def build_html(html_template: str, png_path: pathlib.Path) -> bytes:
    """Inject the current PNG as a base64 data URL into the HTML template."""
    png_bytes = png_path.read_bytes()
    b64 = base64.b64encode(png_bytes).decode("ascii")
    data_url = f"data:image/png;base64,{b64}"
    # The HTML template contains the placeholder token __INITIAL_PNG_DATA_URL__
    out = html_template.replace("__INITIAL_PNG_DATA_URL__", data_url)
    return out.encode("utf-8")


# ---------------------------------------------------------------------------
# Port selection
# ---------------------------------------------------------------------------
def find_free_port() -> int:
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    png_path = resolve_target(sys.argv)
    if not png_path.exists():
        sys.exit(f"ERROR: PNG not found: {png_path}")
    if not HTML_PATH.exists():
        sys.exit(f"ERROR: Editor HTML not found: {HTML_PATH}")

    html_template = HTML_PATH.read_text(encoding="utf-8")

    port = find_free_port()
    url = f"http://127.0.0.1:{port}/"

    shutdown_event = threading.Event()

    # Wire class-level attributes before constructing the handler.
    EditorHandler.html_bytes = build_html(html_template, png_path)
    EditorHandler.png_path = png_path
    EditorHandler.shutdown_event = shutdown_event

    server = HTTPServer(("127.0.0.1", port), EditorHandler)
    server.timeout = 1.0  # allow poll loop to check shutdown_event

    sys.stderr.write(f"[editor] Serving at {url}\n")
    sys.stderr.write(f"[editor] Editing: {png_path}\n")
    sys.stderr.write("[editor] Press Ctrl-C to quit.\n")

    # Open browser after a short delay so the server is ready.
    def open_browser():
        import time; time.sleep(0.3)
        webbrowser.open(url)

    threading.Thread(target=open_browser, daemon=True).start()

    # Serve until shutdown event or KeyboardInterrupt.
    try:
        while not shutdown_event.is_set():
            server.handle_request()
    except KeyboardInterrupt:
        sys.stderr.write("\n[editor] Keyboard interrupt — exiting.\n")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
