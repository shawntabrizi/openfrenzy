#!/usr/bin/env python3
"""Report which sounds started and were heard, from an OPENFRENZY_DSOUND_LOG=1 run log.

Usage: python3 -I scripts/sound_report.py <run.log> [image] [--misses]

Needs the log of a run with OPENFRENZY_DSOUND_LOG=1 (sample and stream starts, "sound: ..."
lines, and each DirectSound buffer's own mix peak, "dsound: buffer ... its mix peak") and
OPENFRENZY_BKLOG=1 (bkPrintf lines: package loads and "bLoadResource: Could not find").

- Sounds: one line per file name: starts, starts whose buffer added more than 0.001 (-60 dBFS)
  to the mix, the largest mix peak, the first start time (output seconds = position in the
  OPENFRENZY_DSOUND_CAPTURE WAV). Background-loaded packages have no filename table in the
  game (also on the Xbox), so names are resolved here from the disc's .xbp filename tables.
- --misses: the "Could not find '<name>' in memory" probes: how many, how many name a file
  that is on the disc, and which of those failed AFTER a package holding the file was loaded
  (a candidate port bug). Closing is known only for the commentator packages (com_*: one at
  a time); a miss after another holder was closed again is still listed.
Default image: orig/game.xiso.iso. Standard library only.
"""
import collections
import math
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import xiso_list  # noqa: E402


def disc_names(image):
    """{(package id | 0x80000000, crc): name}, {name lower: {package file}}"""
    fh, base, rs, rz = xiso_list.open_image(image)
    files = []
    xiso_list.walk(fh, base, rs, rz, "", files)
    by_crc, where = {}, collections.defaultdict(set)
    for path, sec, sz, is_dir in files:
        if is_dir or not path.lower().endswith(".xbp"):
            continue
        off = base + sec * xiso_list.SECTOR
        fh.seek(off)
        pid, bs, _, n, ent, _, _, _, _, _, noff, nsz = struct.unpack_from("<12I", fh.read(0x30))
        if n == 0 or noff == 0:
            continue
        fh.seek(off + noff * bs)
        names = fh.read(nsz)
        fh.seek(off + ent * bs)
        index = fh.read(n * 32)
        for i in range(n):
            _, crc, _, no = struct.unpack_from("<4I", index, i * 32)
            if no >= len(names):
                continue
            name = names[no:names.find(b"\0", no)].decode("latin-1")
            by_crc[("%08x" % (pid | 0x80000000), "%08x" % crc)] = name
            where[name.lower()].add(os.path.basename(path)[:-4].lower())
    return by_crc, where


START = re.compile(r"sound: (.+?) '(.*?)' \(package 0x(\w+) crc 0x(\w+)\) at ([\d.]+) s: buffer (\w+)")
LEVEL = re.compile(r"dsound: buffer (\w+) (ended|stopped|released|playing) at [\d.]+ s after [\d.]+ s: "
                   r"its mix peak ([\d.]+)")
LOADED = re.compile(r"(?:Loaded|Background loaded) package '(.*?)'")
MISS = re.compile(r"bLoadResource: Could not find '(.*?)' in memory")


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not args:
        sys.exit(__doc__)
    image = args[1] if len(args) > 1 else os.path.join(HERE, "..", "orig", "game.xiso.iso")
    by_crc, where = disc_names(image)
    stats = collections.OrderedDict()
    pending, loaded, misses = {}, [], []
    for number, line in enumerate(open(args[0], errors="replace")):
        m = START.search(line)
        if m:
            kind, name, package, crc, time, buffer = m.groups()
            if name == "?":
                name = by_crc.get((package, crc), "?%s/%s" % (package, crc))
            st = stats.setdefault(name, {"kind": kind, "starts": 0, "heard": 0, "peak": 0.0,
                                         "first": float(time)})
            st["starts"] += 1
            pending[buffer] = name
            continue
        m = LEVEL.search(line)
        if m and m.group(1) in pending:
            st = stats[pending[m.group(1)]]
            if m.group(2) != "playing":
                del pending[m.group(1)]
            peak = float(m.group(3))
            st["heard"] += peak >= 0.001
            st["peak"] = max(st["peak"], peak)
            continue
        m = LOADED.search(line)
        if m:
            loaded.append((number, m.group(1).replace("\\", "/").split("/")[-1].lower()))
            continue
        m = MISS.search(line)
        if m:
            misses.append((number, m.group(1).lower()))
    print("%-11s %-26s %6s %6s %18s %9s" % ("kind", "name", "starts", "heard", "max mix peak", "first s"))
    for name, st in stats.items():
        db = 20 * math.log10(st["peak"]) if st["peak"] > 0 else float("-inf")
        print("%-11s %-26s %6d %6d %8.4f %6.1f dB %9.2f" % (st["kind"], name, st["starts"], st["heard"],
                                                         st["peak"], db, st["first"]))
    if "--misses" in sys.argv:
        on_disc = [(n, x) for n, x in misses if x in where]
        def held(n, x):
            # packages loaded before line n; a commentator package (com_*) replaces the last
            # one (CLSamples_LoadCommentatorSet, 0x00071880, closes it first)
            have = set()
            for ln, p in loaded:
                if ln >= n:
                    break
                if p.startswith("com_"):
                    have = {q for q in have if not q.startswith("com_")}
                have.add(p)
            return have & where[x]

        late = [(n, x) for n, x in on_disc if held(n, x)]
        print("\nmisses: %d (%d distinct); name on the disc: %d; after a holder package loaded: %d"
              % (len(misses), len({x for _, x in misses}), len(on_disc), len(late)))
        for n, x in late:
            print("  line %d: %s (in %s)" % (n + 1, x, ", ".join(sorted(where[x]))))


if __name__ == "__main__":
    main()
