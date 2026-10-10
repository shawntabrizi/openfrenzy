#!/usr/bin/env python3
"""Assign every .text function of default.xbe to exactly one work module.

Run:  python3 -I scripts/modules.py   (after scripts/map_sources.py)
In:   export/source_map.json, export/functions.jsonl
Out:  export/modules.json  {module: {kind, title, dir, functions: [addr...], bytes}}
      and a table on stdout. Exits non-zero if a function has no module or two.
      Also records each function's resolved directory in export/modules.json["_resolved"].

Gap resolution: a run of unresolved functions between directory A and directory B is
split at ONE point k (MSVC links object files contiguously, so the run is A..A X.. B..B).
k maximises the call-graph votes: each function votes for A or B by the number of call
edges to functions already resolved in A or in B. Ties go to A (the earlier neighbour).
"""
import json
import os
import sys
from collections import OrderedDict, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
EXPORT = os.path.join(HERE, "..", "export")

# id, kind, title, rules. A rule is (dir_or_region, lo, hi): a function matches when its
# resolved directory (or library region "LIB:<name>") equals dir_or_region and lo <= addr < hi.
# Order = boot priority. Platform modules have no .text functions of their own except the
# library regions they replace.
MODULES = [
    ("runtime", "platform", "Boot (of_boot.c), stub runtime, sdl_platform, shared XDK headers, "
     "msvc_crt, XDK entry-point table; links the game image", []),
    ("vfs", "platform", "D: drive on the disc image (xiso_fs), xbox_files (CreateFile/ReadFile...), "
     "save drive, utility drive", []),
    ("xkernel", "platform", "Kernel + XAPI HLE (threads, events, timing, memory, "
     "MmAllocateContiguousMemory); replaces XBE xapi + libc regions (not decompiled)",
     [("LIB:xapi", 0, 1 << 32), ("LIB:libc", 0, 1 << 32)]),
    ("d3d8", "platform", "D3D8 + XGRAPHICS HLE on GLES 3 (ANGLE/Metal), NV2A shaders, swizzle", []),
    ("dsound", "platform", "DirectSound HLE on SDL3 audio, WMA decoder (music), Bink null; "
     "replaces XBE bink region (not decompiled)", [("LIB:bink", 0, 1 << 32)]),
    ("input", "platform", "XInput (XPP) on SDL3 gamepad + keyboard", []),
    ("core", "game", "Game Core: core.cpp main loop, CGameMode_Root/Scores, tweak menu, static init ($E)",
     [("Core", 0x27540, 0x30500), ("LIB:crt_init", 0, 1 << 32), ("LIB:crt_exit", 0, 1 << 32)]),
    ("bkernel", "engine", "Babel bKernel: memory, files, packages (.xbp), cache, events, bkg load, debug",
     [("bKernel", 0, 1 << 32)]),
    ("bdisplay", "engine", "Babel BDISPLAY: display, render, textures, fonts, drawprim",
     [("BDISPLAY", 0, 1 << 32)]),
    ("engine_misc", "engine", "Babel BINPUT, BGUI, BMATHS, BSOUND (streams, playlists) + late engine tail",
     [("BINPUT", 0, 1 << 32), ("BGUI", 0, 1 << 32), ("BMATHS", 0, 1 << 32), ("BSOUND", 0, 1 << 32),
      ("LIB:engine_tail", 0, 1 << 32)]),
    ("bactor", "engine", "Babel BACTOR: actors, animation, lights, camera, shader selection",
     [("BACTOR", 0, 1 << 32)]),
    ("core_modes", "game", "Game Core: front-end CGameMode_* screens (menus, select, intro, map)",
     [("Core", 0x11000, 0x27540)]),
    ("classlib_ui", "game", "ClassLib block 1: samples, overlay, HUD, intro cam, ParseMap, fireworks, rumble",
     [("ClassLib", 0x703A0, 0x82950)]),
    ("classlib_fx", "game", "ClassLib rest: sprites, particles, projector, debris, ray, token, water, seagulls",
     [("ClassLib", 0x82950, 1 << 32)]),
    ("charlib", "game", "CharLib: character actors, AI, routes, collision, platforms, shadows",
     [("CharLib", 0, 1 << 32)]),
    ("meta", "game", "Meta: the board game between minigames (puzzles, tiles, level load, effects)",
     [("Meta", 0, 1 << 32)]),
    ("bumpercars", "game", "Minigame BumperCars", [("BumperCars", 0, 1 << 32)]),
    ("carryon", "game", "Minigame CarryOn (fireworks)", [("CarryOn", 0, 1 << 32)]),
    ("clusteroids", "game", "Minigame Clusteroids", [("Clusteroids", 0, 1 << 32)]),
    ("mousewhack", "game", "Minigame MouseWhack (BugBash)", [("MouseWhack", 0, 1 << 32)]),
    ("demolition", "game", "Minigame Demolition", [("Demolition", 0, 1 << 32)]),
    ("drillers", "game", "Minigame Drillers", [("Drillers", 0, 1 << 32)]),
    ("foxhunt", "game", "Minigame FoxHunt (Tag)", [("FoxHunt", 0, 1 << 32)]),
    ("hopper", "game", "Minigame Hopper", [("Hopper", 0, 1 << 32)]),
    ("scratchmatch", "game", "Minigame ScratchMatch", [("ScratchMatch", 0, 1 << 32)]),
    ("rollerball", "game", "Minigame RollerBall", [("RollerBall", 0, 1 << 32)]),
    ("rowing", "game", "Minigame Rowing (jet boats)", [("Rowing", 0, 1 << 32)]),
    ("tankedup", "game", "Minigame TankedUp", [("TankedUp", 0, 1 << 32)]),
]

# Modules larger than one agent's task are split into N parts of about equal bytes. A cut is
# only made where the source file changes, so one original file stays in one module.
SPLIT = {"demolition": 4, "bumpercars": 2, "rollerball": 2, "carryon": 2, "meta": 2,
         "rowing": 2, "tankedup": 2, "core_modes": 2}
# A split balances this weight: bytes plus a fixed cost per function (each function is a
# unit of work however small it is).
FUNCTION_WEIGHT = 250

# Regions that belong to no module: compiler-generated C++ exception-unwind funclets (the port
# has no C++ exceptions; the destructors they call are decompiled with their classes).
IGNORED = {"LIB:eh_funclets"}


def file_key(smap_entry, resolved_dir):
    """The source file of a function, or its gap run, for split decisions."""
    if smap_entry.get("file"):
        return smap_entry["file"]
    return "gap:%s:%s" % (resolved_dir, ",".join(smap_entry.get("candidates", [])))


def split_module(fl, n, funcs, resolved):
    """Partition the address-sorted list fl into n parts at file boundaries, by bytes."""
    total = sum(funcs[a]["size"] + FUNCTION_WEIGHT for a in fl)
    parts, cur, acc = [], [], 0
    target = total / n
    for i, a in enumerate(fl):
        cur.append(a)
        acc += funcs[a]["size"] + FUNCTION_WEIGHT
        boundary = i + 1 < len(fl) and file_key(funcs[fl[i + 1]], resolved[fl[i + 1]]) != \
            file_key(funcs[a], resolved[a])
        if boundary and acc >= target * (len(parts) + 1) and len(parts) < n - 1:
            parts.append(cur)
            cur = []
    parts.append(cur)
    return parts


# What each platform module owns in src/ (it replaces XDK libraries; nothing to decompile).
PLATFORM_FILES = {
    "runtime": "src/runtime/ (of_boot.c, of_unimplemented.c, of_config.c, sdl_platform.c, msvc_*.c, "
               "platform.h, include/ XDK headers, compat/, entry/ XDK entry-point table), "
               "src/include/ (shared; architect only)",
    "vfs": "src/vfs/ (xiso_fs.c, xbox_files.c, xiso.c, posix.h)",
    "xkernel": "src/xkernel/ (xbox_kernel.c, xbox_xapi.c, xbox_memory.c)",
    "d3d8": "src/d3d8/ (d3d8_gl.c, d3d8_resources.c, nv2a_*.c, xbox_textures.c, gl.h, xgpu*.{c,h})",
    "dsound": "src/dsound/ (dsound_sdl.c; bink_host.c; WMA decoder to add)",
    "input": "src/input/ (xinput_sdl.c)",
}


def write_docs(out):
    """docs/modules.tsv (machine-readable) and docs/MODULES.md (for the agents)."""
    docs = os.path.join(HERE, "..", "docs")
    with open(os.path.join(docs, "modules.tsv"), "w") as fh:
        fh.write("order\tid\tkind\tfunctions\tbytes\tlo\thi\ttitle\n")
        for i, (mid, rec) in enumerate(out.items()):
            fh.write("%d\t%s\t%s\t%d\t%d\t%s\t%s\t%s\n" % (
                i + 1, mid, rec["kind"], rec["function_count"], rec["bytes"], rec.get("lo") or "-",
                rec.get("hi") or "-", rec["title"]))
    lines = ["# Work modules", "",
             "Generated by `scripts/modules.py` (do not edit). One module = one agent task = one",
             "directory `src/<id>/`. Order = boot priority. See docs/ARCHITECTURE.md for the rules.", "",
             "| # | id | kind | functions | bytes | address range |", "|---:|---|---|---:|---:|---|"]
    for i, (mid, rec) in enumerate(out.items()):
        rng = "0x%s-0x%s" % (rec["lo"], rec["hi"]) if rec.get("lo") else "-"
        lines.append("| %d | `%s` | %s | %d | %d | %s |" % (i + 1, mid, rec["kind"], rec["function_count"],
                                                         rec["bytes"], rng))
    for i, (mid, rec) in enumerate(out.items()):
        lines += ["", "## %d. `%s` (%s)" % (i + 1, mid, rec["kind"]), "", rec["title"] + ".", ""]
        if rec["kind"] == "platform":
            lines.append("Owns: " + PLATFORM_FILES[mid] + ".")
            if rec["function_count"]:
                lines.append("Replaces %d XBE library functions (%s): do not decompile them."
                             % (rec["function_count"], ", ".join(sorted(set(f for f in rec["files"])))))
            continue
        lines.append("Original source files (from `export/source_map.json`; `(unplaced)` = gap functions "
                     "of that directory):")
        lines.append("")
        lines.append(", ".join("`%s`" % f for f in rec["files"]))
    with open(os.path.join(docs, "MODULES.md"), "w") as fh:
        fh.write("\n".join(lines) + "\n")


def main():
    smap = json.load(open(os.path.join(EXPORT, "source_map.json")))
    meta = {}
    for name in ("functions.jsonl", "functions_extra.jsonl"):
        if not os.path.exists(os.path.join(EXPORT, name)):
            continue
        for line in open(os.path.join(EXPORT, name)):
            f = json.loads(line)
            if f["section"] == ".text":
                meta.setdefault(f["addr"], f)
    funcs = smap["functions"]
    addrs = sorted(funcs, key=lambda a: int(a, 16))

    def dir_of(e):
        if e["region"] != "code":
            return "LIB:" + e["region"]
        if e.get("file"):
            return e["file"].rsplit("/", 1)[0]
        return e.get("dir")

    resolved = {a: dir_of(funcs[a]) for a in addrs}

    # Split mixed-directory gap runs.
    i = 0
    n = len(addrs)
    while i < n:
        a = addrs[i]
        if resolved[a] is not None:
            i += 1
            continue
        j = i
        while j < n and resolved[addrs[j]] is None:
            j += 1
        run = addrs[i:j]
        cands = funcs[a]["candidates"]
        da, db = (cands[0].rsplit("/", 1)[0], cands[-1].rsplit("/", 1)[0])
        votes = []
        for x in run:
            m = meta[x]
            va = vb = 0
            for y in m["callers"] + m["callees"]:
                d = resolved.get(y)
                va += d == da
                vb += d == db
            votes.append(va - vb)  # >0 prefers A
        best_k, best = len(run), sum(votes)  # k = first index that goes to B
        acc_a = 0
        for k in range(len(run) + 1):
            # score = votes for A before k  +  votes for B from k on
            score = acc_a - sum(votes[k:])
            if score > best:
                best_k, best = k, score
            if k < len(run):
                acc_a += votes[k]
        for idx, x in enumerate(run):
            resolved[x] = da if idx < best_k else db
        i = j

    out = OrderedDict()
    owner = {}
    seen = set()
    for mid, kind, title, rules in MODULES:
        fl = []
        for a in addrs:
            ai = int(a, 16)
            if any(resolved[a] == d and lo <= ai < hi for d, lo, hi in rules):
                if a in owner or a in seen:
                    print("ERROR %s in two modules (%s)" % (a, mid))
                    sys.exit(1)
                seen.add(a)
                fl.append(a)
        parts = split_module(fl, SPLIT[mid], funcs, resolved) if mid in SPLIT else [fl]
        for pi, pl in enumerate(parts):
            pid = mid if len(parts) == 1 else "%s_%s" % (mid, "abcdefgh"[pi])
            ptitle = title if len(parts) == 1 else "%s (part %d of %d, 0x%s-0x%s)" % (
                title, pi + 1, len(parts), pl[0], pl[-1])
            files = []
            for a in pl:
                fk = funcs[a].get("file") or ("%s/(unplaced)" % resolved[a]
                                              if not resolved[a].startswith("LIB:") else resolved[a])
                if fk not in files:
                    files.append(fk)
            out[pid] = {"kind": kind, "title": ptitle, "dir": "src/%s" % pid,
                        "function_count": len(pl), "bytes": sum(funcs[a]["size"] for a in pl),
                        "lo": pl[0] if pl else None, "hi": pl[-1] if pl else None,
                        "files": files, "functions": pl}
            for a in pl:
                owner[a] = pid
    ignored = [a for a in addrs if resolved[a] in IGNORED]
    missing = [a for a in addrs if a not in owner and resolved[a] not in IGNORED]
    for mid, rec in out.items():
        if mid.startswith("_"):
            continue
        print("%-24s %-8s %5d funcs %8d bytes" % (mid, rec["kind"], rec["function_count"], rec["bytes"]))
    print("total assigned %d of %d (%d ignored: %s)" % (len(owner), len(addrs), len(ignored),
                                                     ", ".join(sorted(IGNORED))))
    if missing:
        print("UNASSIGNED:", len(missing), missing[:20])
        sys.exit(1)
    write_docs(out)
    out["_resolved"] = {a: resolved[a] for a in addrs}
    with open(os.path.join(EXPORT, "modules.json"), "w") as fh:
        json.dump(out, fh, indent=1)


if __name__ == "__main__":
    main()
