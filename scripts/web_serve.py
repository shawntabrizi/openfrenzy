#!/usr/bin/env python3
"""Serve the web build's site on this computer (docs/WEB.md).

    python3 -I scripts/web_serve.py build-web/site              # http://127.0.0.1:8000/
    python3 -I scripts/web_serve.py build-web/site --port 8123 --timeout 600

The page needs SharedArrayBuffer (the game's threads), so every response has the
cross-origin isolation headers (COOP same-origin, COEP require-corp, CORP
same-origin). http://127.0.0.1 is a secure context, as https is. The server binds
to 127.0.0.1 only and sends no-cache, so a rebuild shows at the next reload.
--timeout stops it after that many seconds (the automated tests use it).

For automated runs in a browser that cannot be driven (Safari): --disc PATH serves that file
(the player's own disc image) at /__disc, for the page's ?image-url=/__disc, and --report PATH
appends what the page posts to /__report (its log lines, with ?report) to that file. Both are
off unless given; the server still binds to 127.0.0.1 only.

After OpenCE's tools/web_serve.py (CC0, OpenCommunityEdition, commit e0b581b1).
"""

import argparse
import os
import sys
import threading
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


class SiteHandler(SimpleHTTPRequestHandler):
    extensions_map = {
        **SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript; charset=utf-8",
        ".mjs": "text/javascript; charset=utf-8",
        ".json": "application/json",
    }

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        self.send_header("Cache-Control", "no-cache")
        super().end_headers()

    disc = None
    report = None

    def do_GET(self):
        if self.disc and self.path.split("?")[0] == "/__disc":
            size = os.path.getsize(self.disc)
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(size))
            self.end_headers()
            with open(self.disc, "rb") as source:
                while True:
                    chunk = source.read(1 << 20)
                    if not chunk:
                        break
                    self.wfile.write(chunk)
            return
        super().do_GET()

    def do_POST(self):
        if not self.report or self.path.split("?")[0] != "/__report":
            self.send_error(404)
            return
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length).decode("utf-8", "replace")
        with open(self.report, "a", encoding="utf-8") as out:
            out.write(body if body.endswith("\n") else body + "\n")
        self.send_response(204)
        self.end_headers()

    def log_message(self, format, *args):
        if os.environ.get("WEB_SERVE_QUIET") != "1":
            super().log_message(format, *args)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("root", type=Path, help="the site's folder (build-web/site)")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--timeout", type=float, default=0, help="stop after this many seconds (0: never)")
    parser.add_argument("--disc", type=Path, help="serve this disc image at /__disc (automated runs)")
    parser.add_argument("--report", type=Path, help="append what the page posts to /__report here")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    if not (root / "index.html").is_file():
        parser.error(f"{root} has no index.html: build the web target first (docs/WEB.md)")

    class Handler(SiteHandler):
        disc = str(args.disc.resolve()) if args.disc else None
        report = str(args.report.resolve()) if args.report else None

        def __init__(self, *handler_args, **kwargs):
            super().__init__(*handler_args, directory=str(root), **kwargs)

    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    server.daemon_threads = True
    print(f"Serving {root} at http://127.0.0.1:{server.server_address[1]}/", flush=True)
    if args.timeout > 0:
        timer = threading.Timer(args.timeout, server.shutdown)
        timer.daemon = True
        timer.start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
