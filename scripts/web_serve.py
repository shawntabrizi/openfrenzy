#!/usr/bin/env python3
"""Serve the web build's site on this computer (docs/WEB.md).

    python3 -I scripts/web_serve.py build-web/site              # http://127.0.0.1:8000/
    python3 -I scripts/web_serve.py build-web/site --port 8123 --timeout 600

The page needs SharedArrayBuffer (the game's threads), so every response has the
cross-origin isolation headers (COOP same-origin, COEP require-corp, CORP
same-origin). http://127.0.0.1 is a secure context, as https is. The server binds
to 127.0.0.1 only and sends no-cache, so a rebuild shows at the next reload.
--timeout stops it after that many seconds (the automated tests use it).

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

    def log_message(self, format, *args):
        if os.environ.get("WEB_SERVE_QUIET") != "1":
            super().log_message(format, *args)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("root", type=Path, help="the site's folder (build-web/site)")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--timeout", type=float, default=0, help="stop after this many seconds (0: never)")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    if not (root / "index.html").is_file():
        parser.error(f"{root} has no index.html: build the web target first (docs/WEB.md)")

    class Handler(SiteHandler):
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
