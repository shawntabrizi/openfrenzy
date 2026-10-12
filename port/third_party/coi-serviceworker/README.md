# coi-serviceworker

`coi-serviceworker.js` and `LICENSE` from coi-serviceworker 0.1.7 by Guido Zuidhof and
contributors (MIT licence), not changed:

- Source: <https://github.com/gzuidhof/coi-serviceworker>, the npm package
  `coi-serviceworker@0.1.7` (integrity `sha512-bjSUqEngCPOkErY2vbyWsaIGCNRODYzlNycaREVw5s12/C8SM+RnRUUeX6pZbTtov6C52ZLY/+tvHK+BDxuUuA==`)
- `coi-serviceworker.js` SHA-256: `d12bd536e27e39a773d7dc7adb1a1167d24002293e97ac81c995fb00cf8d4d5a`

The web build copies it into the site (`port/web/CMakeLists.txt`). A host that cannot send the
cross-origin isolation headers (GitHub Pages) gets them from this service worker: it adds COOP
and COEP to every response of the site and reloads the page once. Where the server sends the
headers (`scripts/web_serve.py`), the page is already isolated and the script does nothing.
See `docs/WEB.md` "Hosting".
