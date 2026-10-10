#!/usr/bin/env python3
"""Assign every .text function of default.xbe to exactly one work module.

Run:  python3 -I scripts/modules.py   (after scripts/map_sources.py)
In:   export/source_map.json, export/functions.jsonl
Out:  export/modules.json  {module: {kind, title, dir, functions: [addr...], bytes}}
      and a table on stdout. Exits non-zero if a function has no module or two.

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
    ("runtime", "platform", "Guest memory, XBE loader, function table, stubs, main()", []),
    ("vfs", "platform", "XDVDFS ISO reader, drive letters, host save dir", []),
    ("xapi", "platform", "Kernel + XAPI + LIBC replacements, threads, timing, guest heap",
     [("LIB:xapi", 0, 1 << 32), ("LIB:libc", 0, 1 << 32)]),
    ("d3d8", "platform", "D3D8 + XGRAPHICS HLE on OpenGL 4.1", []),
    ("dsound", "platform", "DirectSound HLE on SDL3 audio, WMA stub", []),
    ("xinput", "platform", "XInput (XPP) on SDL3 gamepad, Bink video stub",
     [("LIB:bink", 0, 1 << 32)]),
    ("bkernel", "engine", "Babel bKernel: memory, files, packages, cache, events, bkg load, debug",
     [("bKernel", 0, 1 << 32)]),
    ("bdisplay", "engine", "Babel BDISPLAY: display, render, textures, fonts, drawprim",
     [("BDISPLAY", 0, 1 << 32)]),
    ("core", "game", "Game Core: core.cpp main loop, CGameMode_Root/Scores, tweak menu, static init",
     [("Core", 0x27540, 0x30500), ("LIB:crt_init", 0, 1 << 32)]),
    ("binput_bgui", "engine", "Babel BINPUT + BGUI", [("BINPUT", 0, 1 << 32), ("BGUI", 0, 1 << 32)]),
    ("bactor", "engine", "Babel BACTOR: actors, animation, lights, camera, shader selection",
     [("BACTOR", 0, 1 << 32)]),
    ("bmaths_bsound", "engine", "Babel BMATHS + BSOUND (streams, playlists) + late engine tail",
     [("BMATHS", 0, 1 << 32), ("BSOUND", 0, 1 << 32), ("LIB:engine_tail", 0, 1 << 32)]),
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
    ("clusteroids_mousewhack", "game", "Minigames Clusteroids + MouseWhack",
     [("Clusteroids", 0, 1 << 32), ("MouseWhack", 0, 1 << 32)]),
    ("demolition", "game", "Minigame Demolition", [("Demolition", 0, 1 << 32)]),
    ("drillers_foxhunt", "game", "Minigames Drillers + FoxHunt",
     [("Drillers", 0, 1 << 32), ("FoxHunt", 0, 1 << 32)]),
    ("hopper_scratchmatch", "game", "Minigames Hopper + ScratchMatch",
     [("Hopper", 0, 1 << 32), ("ScratchMatch", 0, 1 << 32)]),
    ("rollerball", "game", "Minigame RollerBall", [("RollerBall", 0, 1 << 32)]),
    ("rowing", "game", "Minigame Rowing", [("Rowing", 0, 1 << 32)]),
    ("tankedup", "game", "Minigame TankedUp", [("TankedUp", 0, 1 << 32)]),
]


def main():
    smap = json.load(open(os.path.join(EXPORT, "source_map.json")))
    meta = {}
    for line in open(os.path.join(EXPORT, "functions.jsonl")):
        f = json.loads(line)
        if f["section"] == ".text":
            meta[f["addr"]] = f
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
    for mid, kind, title, rules in MODULES:
        fl = []
        for a in addrs:
            ai = int(a, 16)
            if any(resolved[a] == d and lo <= ai < hi for d, lo, hi in rules):
                if a in owner:
                    print("ERROR %s in %s and %s" % (a, owner[a], mid))
                    sys.exit(1)
                owner[a] = mid
                fl.append(a)
        out[mid] = {"kind": kind, "title": title, "dir": "src/%s" % mid,
                    "function_count": len(fl), "bytes": sum(funcs[a]["size"] for a in fl),
                    "functions": fl}
    missing = [a for a in addrs if a not in owner]
    for mid, rec in out.items():
        print("%-24s %-8s %5d funcs %8d bytes" % (mid, rec["kind"], rec["function_count"], rec["bytes"]))
    print("total assigned %d of %d" % (len(owner), len(addrs)))
    if missing:
        print("UNASSIGNED:", len(missing), missing[:20])
        sys.exit(1)
    with open(os.path.join(EXPORT, "modules.json"), "w") as fh:
        json.dump(out, fh, indent=1)


if __name__ == "__main__":
    main()
