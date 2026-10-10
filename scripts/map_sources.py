#!/usr/bin/env python3
"""Map every .text function of default.xbe to its original source file.

Run:  python3 -I scripts/map_sources.py [export_dir]
In:   export/functions.jsonl (scripts/ExportAll.java) + export/functions_extra.jsonl (ExportAt.java)
Out:  export/source_map.json, plus a coverage summary on stdout.

Method
1. A function is "tagged" when it references a string that holds a source path
   (assert / debug / operator-new file arguments such as
   C:\\BlitzParty\\Game\\Src\\Core\\core.cpp).
2. Statically linked library code is classified by fixed address regions
   (LIBRARY_REGIONS below). Evidence for each region is in its "evidence" text.
3. MSVC links each object file contiguously. An untagged run of functions
   between two tagged functions of the SAME file belongs to that file
   ("bracketed"). A run between two DIFFERENT files is a "gap": it belongs to
   one of the two neighbours or to an object file that has no tagged function.
   Gaps keep both candidates; when both neighbours share a directory the gap
   gets that directory.
4. Runs before the first / after the last anchor of an area are "leading" /
   "trailing" and get the one neighbour as candidate.
"""
import json
import os
import re
import sys
from collections import OrderedDict, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
EXPORT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "..", "export")

PATH_RE = re.compile(r"([A-Za-z]:\\[^\s(\"]+?\.(?:cpp|c|h|hpp|inl))", re.I)
ROOTS = [("C:\\BlitzParty\\Game\\Src\\", "game"), ("C:\\Babel\\Xbox\\Src\\", "engine")]

# [lo, hi) virtual addresses. Found by hand; see docs/ARCHITECTURE.md "Address map".
LIBRARY_REGIONS = [
    ("xapi", 0x0003055A, 0x00034730, "lib",
     "XDK XAPILIB + CRT startup (entry, mainXapiStartup @0x30f10, CreateThread, _cinit). "
     "Unaligned addresses, XbSymbolDatabase names."),
    ("bink", 0x00057980, 0x000703A0, "lib",
     "RAD Bink video library (statically linked). 'Not a Bink file.' in 0x591c0 (BinkOpen); "
     "its own DirectSound mixer 0x69df0+. Only 0x591c0/0x59f80/0x5a090 are called from game code."),
    ("libc", 0x001DE951, 0x001EA360, "lib",
     "MSVC LIBC (+ C++ EH runtime, _ftol, 64-bit math helpers, stdio, malloc). Unaligned."),
    ("engine_tail", 0x001EA360, 0x001EC550, "untagged",
     "16-byte aligned code linked after LIBC. No source strings. Called from game code "
     "(0x8xxxx, 0x19xxxx, 0x1cxxxx); 0x1eaec0 binds BINPUT controls 'LeftRight','UpDown'. "
     "Probably a late-linked Babel or ClassLib object (camera / collision helpers)."),
    ("libc", 0x001EC550, 0x001ECC20, "lib",
     "MSVC LIBC string helpers (strnicmp, itoa, strncpy, memset, ...)."),
    ("eh_funclets", 0x001ECC20, 0x001F29F0, "lib",
     "Compiler-generated C++ EH unwind funclets (mov ecx,[ebp-x]; jmp dtor). Not real functions."),
    ("crt_init", 0x001F29F0, 0x001F4E20, "game",
     "Compiler-generated static constructors ($E functions), listed in the .CRT$XCA..XCZ table at "
     "0x281750-0x2818a0 that _cinit (0x340f8) runs. Ghidra's analysis missed most of them; "
     "scripts/ExportAt.java exports them to export/functions_extra.jsonl."),
    ("crt_exit", 0x001F4E20, 0x001F6000, "game",
     "Static destructors: the constructors register them with atexit (e.g. 0x1f2a10 -> 0x1f4e20)."),
]


def short_path(full):
    for root, kind in ROOTS:
        if full.lower().startswith(root.lower()):
            return full[len(root):].replace("\\", "/"), kind
    return full.replace("\\", "/"), "other"


def region_of(addr):
    for name, lo, hi, kind, _ in LIBRARY_REGIONS:
        if lo <= addr < hi:
            return name, kind
    return None, None


def read_xbe_sections(xbe_path):
    """Return {name: (va, bytes)} for the XBE sections (raw data only)."""
    import struct
    d = open(xbe_path, "rb").read()
    base, = struct.unpack_from("<I", d, 0x104)
    nsec, secaddr = struct.unpack_from("<II", d, 0x11C)
    out = {}
    for i in range(nsec):
        _, va, vs, ra, rs, na = struct.unpack_from("<IIIIII", d, secaddr - base + i * 56)
        name = d[na - base:d.index(b"\0", na - base)].decode()
        out[name] = (va, d[ra:ra + rs])
    return out


def refine_with_vtables(funcs, out_funcs):
    """A vtable is a run of >=2 dwords in .rdata/.data that are .text function starts.
    The methods of one class live in one .cpp file. When an unresolved function sits in a
    vtable whose other members resolve to exactly one file, and that file is one of the
    function's gap candidates, assign it (confidence "vtable"). Inherited base-class methods
    appear in derived vtables, so the candidate check is required."""
    xbe = os.path.join(HERE, "..", "orig", "default.xbe")
    if not os.path.exists(xbe):
        print("note: orig/default.xbe missing, vtable refinement skipped")
        return
    import struct
    starts = {f["a"]: f["addr"] for f in funcs}
    vtables = []
    for sec in (".rdata", ".data"):
        va, data = read_xbe_sections(xbe)[sec]
        run = []
        for off in range(0, len(data) - 3, 4):
            v, = struct.unpack_from("<I", data, off)
            if v in starts:
                run.append(starts[v])
            else:
                if len(run) >= 2:
                    vtables.append(run)
                run = []
    fkind = {e["file"]: e["kind"] for e in out_funcs.values() if e.get("file")}
    changed = True
    while changed:
        changed = False
        for vt in vtables:
            files = {out_funcs[a]["file"] for a in vt if out_funcs[a].get("file")}
            for a in vt:
                e = out_funcs[a]
                if e.get("file") or e["region"] != "code":
                    continue
                hits = files & set(e.get("candidates", []))
                if len(hits) == 1:
                    fn = hits.pop()
                    e.update(file=fn, confidence="vtable", kind=fkind[fn])
                    e.pop("candidates", None)
                    e.pop("dir", None)
                    changed = True


def rebracket(funcs, out_funcs):
    """After new anchors appear, re-run the bracketing rule over each code area and
    tighten the candidates of what stays unresolved to the nearest resolved neighbours."""
    fkind = {e["file"]: e["kind"] for e in out_funcs.values() if e.get("file")}
    n = len(funcs)
    for i, f in enumerate(funcs):
        e = out_funcs[f["addr"]]
        if e["region"] != "code" or e.get("file"):
            continue
        def scan(step):
            j = i + step
            while 0 <= j < n:
                o = out_funcs[funcs[j]["addr"]]
                if o["region"] != "code":
                    return None
                if o.get("file"):
                    return o["file"]
                j += step
            return None
        pf, nf = scan(-1), scan(1)
        if pf and pf == nf:
            e.update(file=pf, kind=fkind[pf], confidence="bracketed")
            e.pop("candidates", None)
            e.pop("dir", None)
        else:
            cands = [x for x in (pf, nf) if x]
            dirs = {c.rsplit("/", 1)[0] for c in cands}
            kinds = {fkind[c] for c in cands}
            e.update(candidates=cands, dir=dirs.pop() if len(dirs) == 1 else None,
                     kind=kinds.pop() if len(kinds) == 1 else "game/engine",
                     confidence="gap" if pf and nf else ("trailing" if pf else "leading"))


def main():
    funcs = []
    seen = set()
    for name in ("functions.jsonl", "functions_extra.jsonl"):
        path = os.path.join(EXPORT, name)
        if not os.path.exists(path):
            continue
        with open(path) as fh:
            for line in fh:
                f = json.loads(line)
                if f["section"] == ".text" and f["addr"] not in seen:
                    seen.add(f["addr"])
                    f["a"] = int(f["addr"], 16)
                    funcs.append(f)
    funcs.sort(key=lambda f: f["a"])

    canon = {}  # lower-case short path -> first spelling seen
    for f in funcs:
        paths = sorted({m.group(1) for s in f["strings"] for m in PATH_RE.finditer(s)})
        f["tags"] = []
        for p in paths:
            sp, kind = short_path(p)
            sp = canon.setdefault(sp.lower(), sp)
            f["tags"].append((sp, kind, p))

    out_funcs = OrderedDict()
    multi = []
    # Split the function list into areas: library regions and the code between them.
    areas = []
    cur = None
    for f in funcs:
        reg, _ = region_of(f["a"])
        key = reg or "code"
        if cur is None or cur[0] != key:
            cur = [key, []]
            areas.append(cur)
        cur[1].append(f)

    for key, fl in areas:
        if key != "code":
            kind = region_of(fl[0]["a"])[1]
            for f in fl:
                out_funcs[f["addr"]] = {"name": f["name"], "size": f["size"], "region": key,
                                        "kind": kind, "file": None, "confidence": "region"}
            continue
        anchors = []
        for i, f in enumerate(fl):
            if f["tags"]:
                if len(f["tags"]) > 1:
                    multi.append((f["addr"], [t[0] for t in f["tags"]]))
                anchors.append(i)
        for i, f in enumerate(fl):
            ent = {"name": f["name"], "size": f["size"], "region": "code"}
            if f["tags"]:
                sp, kind, _ = f["tags"][0]
                ent.update(file=sp, kind=kind, confidence="tagged")
            else:
                prev = max((j for j in anchors if j < i), default=None)
                nxt = min((j for j in anchors if j > i), default=None)
                pf = fl[prev]["tags"][0] if prev is not None else None
                nf = fl[nxt]["tags"][0] if nxt is not None else None
                if pf and nf and pf[0] == nf[0]:
                    ent.update(file=pf[0], kind=pf[1], confidence="bracketed")
                else:
                    cands = [x[0] for x in (pf, nf) if x]
                    conf = "gap" if pf and nf else ("trailing" if pf else "leading")
                    dirs = {c.rsplit("/", 1)[0] for c in cands}
                    kinds = {x[1] for x in (pf, nf) if x}
                    ent.update(file=None, candidates=cands, confidence=conf,
                               dir=dirs.pop() if len(dirs) == 1 else None,
                               kind=kinds.pop() if len(kinds) == 1 else "game/engine")
            out_funcs[f["addr"]] = ent

    refine_with_vtables(funcs, out_funcs)
    rebracket(funcs, out_funcs)

    # Per-file contiguous ranges (tagged + bracketed + vtable functions).
    files = defaultdict(lambda: {"ranges": [], "functions": 0, "tagged": 0, "bytes": 0})
    order = []
    last_file = None
    for f in funcs:
        e = out_funcs[f["addr"]]
        fn = e.get("file")
        if not fn:
            last_file = None
            continue
        rec = files[fn]
        if fn not in order:
            order.append(fn)
        rec["kind"] = e["kind"]
        rec["functions"] += 1
        rec["bytes"] += f["size"]
        rec["tagged"] += e["confidence"] == "tagged"
        end = f["a"] + max(f["size"], 1)
        if last_file == fn and rec["ranges"]:
            rec["ranges"][-1][1] = end
        else:
            rec["ranges"].append([f["a"], end])
        last_file = fn
    files_out = OrderedDict()
    for fn in order:
        r = files[fn]
        r["ranges"] = [["%08x" % lo, "%08x" % hi] for lo, hi in r["ranges"]]
        files_out[fn] = r

    # Gaps: maximal runs of unresolved functions.
    gaps = []
    for f in funcs:
        e = out_funcs[f["addr"]]
        if e["region"] != "code" or e.get("file"):
            continue
        key = (tuple(e["candidates"]), e["confidence"])
        if gaps and gaps[-1]["_key"] == key and gaps[-1]["_end"] == f["addr"]:
            g = gaps[-1]
        else:
            g = {"_key": key, "lo": f["addr"], "functions": 0, "bytes": 0,
                 "candidates": e["candidates"], "dir": e["dir"], "confidence": e["confidence"]}
            gaps.append(g)
        g["functions"] += 1
        g["bytes"] += f["size"]
        g["hi"] = "%08x" % (f["a"] + f["size"])
        idx = funcs.index(f)
        g["_end"] = funcs[idx + 1]["addr"] if idx + 1 < len(funcs) else None
    for g in gaps:
        del g["_key"], g["_end"]

    # Coverage summary.
    cov = defaultdict(lambda: [0, 0])
    for addr, e in out_funcs.items():
        k = e["confidence"] if e["region"] == "code" else "lib:" + e["region"]
        cov[k][0] += 1
        cov[k][1] += e["size"]
    dir_cov = defaultdict(lambda: [0, 0])
    for addr, e in out_funcs.items():
        if e["region"] != "code":
            continue
        d = e["file"].rsplit("/", 1)[0] if e.get("file") else (e.get("dir") or "?unresolved")
        dir_cov[d][0] += 1
        dir_cov[d][1] += e["size"]

    regions = [{"name": n, "lo": "%08x" % lo, "hi": "%08x" % hi, "kind": k, "evidence": ev}
               for n, lo, hi, k, ev in LIBRARY_REGIONS]
    result = OrderedDict(
        description="Source-file map of default.xbe .text. Generated by scripts/map_sources.py.",
        regions=regions,
        coverage={k: {"functions": v[0], "bytes": v[1]} for k, v in sorted(cov.items())},
        directories={k: {"functions": v[0], "bytes": v[1]} for k, v in sorted(dir_cov.items())},
        files=files_out,
        gaps=gaps,
        multi_tagged=multi,
        functions=out_funcs,
    )
    with open(os.path.join(EXPORT, "source_map.json"), "w") as fh:
        json.dump(result, fh, indent=1)

    total = len(funcs)
    print("functions in .text: %d" % total)
    for k, v in sorted(cov.items()):
        print("  %-18s %5d funcs %8d bytes" % (k, v[0], v[1]))
    print("source files with code: %d" % len(files_out))
    print("gaps (unresolved runs): %d, %d funcs" % (len(gaps), sum(g["functions"] for g in gaps)))
    print("functions with >1 source path: %d" % len(multi))
    print("per directory (resolved file or same-directory gap):")
    for k, v in sorted(dir_cov.items()):
        print("  %-22s %5d funcs %8d bytes" % (k, v[0], v[1]))


if __name__ == "__main__":
    main()
