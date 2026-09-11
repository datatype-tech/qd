# serve.py - local dev server for Quantum Garden web build.
# Unlike "python -m http.server", every response carries
# Cache-Control: no-store, so the browser never replays a stale
# index.js / index.wasm / index.data after rebuild_w.bat runs.
# Usage: python serve.py [port]
import sys
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer


class NoCacheHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-store, must-revalidate")
        super().end_headers()


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    ThreadingHTTPServer(("127.0.0.1", port), NoCacheHandler).serve_forever()
