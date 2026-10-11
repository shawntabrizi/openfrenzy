#!/usr/bin/env python3
"""Write docs/LIBRARY_CALLS.md: every XBE library function that game/engine code calls, and
what the port uses instead.

Run:  python3 -I scripts/gen_library_calls.py   (after gen_function_table.py)
In:   export/functions.jsonl (+ functions_extra.jsonl), export/source_map.json,
      src/include/of_functions.tsv, src/runtime/entry/xdk_entry_points.inc

Decompiled code never calls these addresses. It calls the replacement: the C library (musl),
or the XDK function of the same name, which the platform layer implements (HLE).
"""
import collections
import json
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
EXPORT = os.path.join(ROOT, "export")

# Unnamed library functions identified by hand from their decompiled bodies (verify before
# relying on an argument order). Key: address; value: replacement.
IDENTIFIED = {
    "001de998": "_ftol: float (x87 ST0) to 64-bit integer, truncating. Write a C cast: (int)x or (longlong)x",
    "001dee6e": "sprintf(buffer, format, ...)",
    "001e0516": "_snprintf(buffer, count, format, ...)",
    "001df984": "sscanf(text, format, ...)",
    "0003417a": "CloseHandle(handle)",
    "0003096f": "Sleep(milliseconds) (SleepEx(ms, FALSE))",
    "00030943": "WaitForSingleObject(handle, milliseconds) (WaitForSingleObjectEx(h, ms, FALSE))",
    "0003080d": "ReleaseMutex(handle)",
    "000310c1": "lstrlenA(text)",
    "0003099f": "wsprintfA(buffer, format, ...)",
    "00030ade": "GlobalMemoryStatus(status)",
    "000591c0": "BinkOpen (RAD Bink; 'Not a Bink file.'): bink_host.c, decoded by the host with FFmpeg (host_bink.c)",
    "00059f80": "BinkClose (RAD Bink): bink_host.c",
    "0005a090": "RADSetMemory (RAD Bink; XbSymbolDatabase says D3D_SetPushBufferSize): bink_host.c",
    "001def24": "C++ 'eh vector constructor iterator' (??_M): construct each element of an array; write a loop",
    "001def84": "C++ 'eh vector destructor iterator' (??_L): destroy each element of an array; write a loop",
    "001df950": "__allmul: 64-bit multiply; write a * b on 64-bit types",
    "001df8a0": "__alldiv: 64-bit divide; write a / b",
    "001e0a80": "__aulldiv: unsigned 64-bit divide; write a / b",
    "001e0b10": "__allrem: 64-bit remainder; write a % b",
    "001ec6f0": "__allshr: 64-bit shift right; write a >> n",
    # The rest were named by the xkernel module (src/xkernel/xkernel_libcalls.h) and checked
    # again against the bodies, the x86 and the callers (docs/INTEGRATION_NOTES.md, "Library
    # functions named"). Each port call site uses the replacement given here.
    # xapi
    "000309d4": "XGetLanguage() (ExQueryNonVolatileSetting(XC_LANGUAGE))",
    "000309f9": "XGetVideoStandard() (XC_FACTORY_AV_REGION >> 8 & 0xff)",
    "00030a22": "XGetVideoFlags() (XC_VIDEO_FLAGS >> 16 & 0x5f)",
    "00030ab6": "XGetGameRegion() (XC_FACTORY_GAME_REGION)",
    "00030b2d": "GetDiskFreeSpaceExA(path, free, total, total free) (NtQueryVolumeInformationFile)",
    "0003101e": "lstrcpyA(destination, source) (strcpy in an SEH frame)",
    "0003105c": "lstrcpynA(destination, source, count) (at most count - 1 characters, always terminated)",
    "0003463a": "XLoadSection(name) (XeLoadSection of the named XBE section; returns its address)",
    "00034655": "XFreeSection(name) (XeUnloadSection of the named XBE section)",
    # libc: C library and compiler helpers
    "001df03e": "atoi (Ghidra's FID says _atol; the same code on ILP32)",
    "001df043": "strtok(text, delimiters) (MSVC keeps the position per thread, musl globally)",
    "001df0f0": "strstr(text, pattern)",
    "001df170": "_chkstk / _alloca_probe (stack probe, size in EAX): no C call; delete it",
    "001df180": "swprintf(buffer, format, ...) (MSVC form: no count, 16-bit wchar_t; msvc_wide.c)",
    "001df260": "qsort(base, count, size, compare) (MSVC's algorithm in xbox_crt.c: same order of equal keys)",
    "001df4f0": "_snwprintf(buffer, count, format, ...) (16-bit wchar_t; msvc_wide.c)",
    "001df688": "pow(x, y) (_CIpow: x87 intrinsic, x = ST1, y = ST0)",
    "001df9c0": "memmove(destination, source, size) (MSVC memcpy/memmove: safe for overlap)",
    "001dfded": "fread(buffer, size, count, file)",
    "001e01ad": "fopen(name, mode) (_fsopen(name, mode, _SH_DENYNO))",
    "001e02d7": "fwrite(buffer, size, count, file) (Ghidra's FID says _fread)",
    "001e0340": "strchr(text, character)",
    "001e03fc": "_strlwr(text) (A-Z only; no caller uses the result)",
    "001e041f": "vsprintf(buffer, format, va_list)",
    "001e05b0": "acos(x) (_CIacos: x87 intrinsic, x = ST0)",
    "001e067c": "asin(x) (_CIasin: x87 intrinsic, x = ST0)",
    "001e0873": "fputc(character, file)",
    "001e0a24": "_clearfp() (fnstsw, fnclex; returns the abstract status word)",
    "001e0af2": "fmod(x, y) (_CIfmod: x87 intrinsic, x = ST1, y = ST0)",
    "001e0bc2": "floor(x)",
    "001e0c90": "fprintf(file, format, ...) (Ghidra's FID says _fwprintf; it calls the narrow _output)",
    "001e0d53": "free(memory) (small-block heap or HeapFree)",
    "001e0dab": "realloc(memory, size)",
    "001e0fa8": "atof(text)",
    "001e105c": "sin(x) (cdecl, x a double on the stack)",
    "001e10f8": "atan2(y, x) (cdecl, two doubles)",
    "001e1120": "cos(x) (cdecl, x a double on the stack)",
    # libc region: XAPI functions linked after the C library
    "001e87e0": "XPhysicalAlloc(size, address, alignment, protect) (MmAllocateContiguousMemoryEx)",
    "001e8823": "XPhysicalProtect(address, size, protect) (MmSetAddressProtect when size != 0)",
    "001e8833": "XPhysicalFree(address) (a jump to MmFreeContiguousMemory)",
    "001e893c": "GetTickCount() (KeTickCount)",
    "001e8de0": "GetLocalTime(time) (system time less the time zone bias)",
    "001e8e5f": "QueryPerformanceCounter(counter)",
    "001e8e74": "QueryPerformanceFrequency(frequency)",
    "001e904c": "SetEndOfFile(file)",
    "001e90d0": "SetFilePointer(file, distance, high distance, method)",
    "001e91ca": "GetFileInformationByHandle(file, information)",
    "001e9307": "ReadFileEx(file, buffer, size, overlapped, completion) (completion by APC)",
    "001e9358": "WriteFileEx(file, buffer, size, overlapped, completion) (completion by APC)",
    "001e93a9": "GetFileSize(file, high size)",
    "001e93e4": "CreateFileA(name, access, share, security, disposition, flags, template)",
    "001e95db": "FindFirstFileA(pattern, data)",
    "001e96e8": "FindNextFileA(find, data)",
    "001e97cf": "DeleteFileA(name)",
    "001e997f": "XFindNextSoundtrack(find, data) (no soundtracks in the port)",
    "001e999e": "XFindFirstSoundtrack(data) (no soundtracks in the port)",
    "001e9a4d": "XGetSoundtrackSongInfo(soundtrack, index, song, length, name, name size)",
    "001e9c76": "XOpenSoundtrackSong(song, asynchronous) (...\\TDATA\\FFFE0000\\MUSIC\\%04x\\%08x.WMA)",
    "001e9d06": "XFindClose(find) (soundtrack and nickname find handles)",
}


def load():
    fs = {}
    for name in ("functions.jsonl", "functions_extra.jsonl"):
        path = os.path.join(EXPORT, name)
        if os.path.exists(path):
            for line in open(path):
                f = json.loads(line)
                fs.setdefault(f["addr"], f)
    smap = json.load(open(os.path.join(EXPORT, "source_map.json")))["functions"]
    game = {}
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if not line.startswith("#"):
            cols = line.split("\t")
            game[cols[0]] = cols[1]
    hle = {}
    inc = os.path.join(ROOT, "src", "runtime", "entry", "xdk_entry_points.inc")
    for m in re.finditer(r'XDK_ENTRY\(0x([0-9a-f]{8}), "(\w+)", "([^"]*)", (\w+), (\w+)\)', open(inc).read()):
        hle[m.group(1)] = (m.group(4), m.group(5))
    return fs, smap, game, hle


def main():
    fs, smap, game, hle = load()
    callers = collections.defaultdict(set)
    for a, module in game.items():
        for c in fs[a]["callees"]:
            if c not in game and not c.startswith("EXTERNAL"):
                callers[c].add(module)
    groups = collections.defaultdict(list)
    for c, mods in callers.items():
        f = fs.get(c)
        if not f:
            continue
        region = smap[c]["region"] if f["section"] == ".text" and c in smap else f["section"]
        name = f["name"]
        if c in IDENTIFIED:
            repl = IDENTIFIED[c]
        elif c in hle and hle[c][0] != "NULL":
            repl = "%s (HLE, %s)" % (hle[c][0], hle[c][1])
        elif c in hle:
            repl = "no HLE yet (%s): the owning platform module adds it" % hle[c][1]
        elif not name.startswith("FUN_"):
            clean = re.sub(r"^.*::", "", name)
            repl = ("%s (C library)" % clean.lstrip("_")) if region == "libc" else ("%s (XDK, HLE)" % clean)
        else:
            repl = "unidentified: read export/funcs/%s.c and add it to IDENTIFIED" % c
        groups[region].append((len(mods), c, name, repl, ", ".join(sorted(mods))))
    lines = ["# Library calls", "",
             "Generated by `scripts/gen_library_calls.py` (do not edit). Every XBE library function",
             "that game or engine code calls. Decompiled code calls the replacement, never the address.",
             "Owners: `libc`/`xapi` -> xkernel (and musl), `D3D`/`XGRPH` -> d3d8, `DSOUND`/`WMADEC*`/`bink`",
             "-> dsound, `XPP` -> input. Hand identifications are marked in the script; verify them.", ""]
    for region in sorted(groups):
        rows = sorted(groups[region], key=lambda r: (-r[0], r[1]))
        lines += ["## %s (%d functions)" % (region, len(rows)), "",
                  "| address | XBE name | replacement | called from modules |", "|---|---|---|---|"]
        for n, c, name, repl, mods in rows:
            lines.append("| 0x%s | `%s` | %s | %s |" % (c, name.replace("|", "\\|"), repl.replace("|", "\\|"), mods))
        lines.append("")
    with open(os.path.join(ROOT, "docs", "LIBRARY_CALLS.md"), "w") as fh:
        fh.write("\n".join(lines))
    print("docs/LIBRARY_CALLS.md: %d functions in %d regions; %d unidentified" % (
        sum(len(v) for v in groups.values()), len(groups),
        sum(1 for v in groups.values() for r in v if r[3].startswith("unidentified"))))


if __name__ == "__main__":
    main()
