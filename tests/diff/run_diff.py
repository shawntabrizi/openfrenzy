#!/usr/bin/env python3
"""Per-function differential test: original x86 (orig/default.xbe in Unicorn) against the
port's compiled C (the game build's objects, run in the host arena). See tests/diff/README.md.

  python3 -I tests/diff/run_diff.py --build build-<id> [--scope leaf|l1|all]
                                    [--list tests/diff/regression.txt] [--only ADDR,...]
                                    [--cases N] [--report FILE]

Exit codes: 0 every tested function matches (float-only differences allowed), 1 a real
mismatch, 2 a harness error, 77 skipped (no orig/default.xbe, or no Unicorn).
"""

import argparse
import bisect
import math
import os
import platform
import random
import re
import shlex
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
VENV_PYTHON = os.path.join(ROOT, "tools", "diff-venv", "bin", "python")
SKIP = 77

SCRATCH_BASE = 0x87000000      # inside the Xbox window, where the guest can map a fixed address
SCRATCH_SIZE = 0x40000
IMAGES = 4
TEXT_OFF = 0x38000             # ASCII text from here to the end of the scratch window
PTR_SLOT = 0x8000              # pointer argument j points into [0x1000 + j * PTR_SLOT, ...)
FLOAT_REL = 1e-4               # float-only: relative difference at most this
FLOAT_ABS = 1e-5               # ... or absolute difference at most this
XBE_LO, XBE_HI = 0x00010000, 0x00330000   # the XBE image as Unicorn maps it
STALL = 5.0                    # seconds without port output: the running case hangs


def skip(message):
    print("SKIP: " + message)
    sys.exit(SKIP)


# ---------------------------------------------------------------- environment

def import_unicorn(work):
    """Load the macOS shim, then Unicorn; re-run under tools/diff-venv when needed."""
    try:
        import unicorn  # noqa: F401  (only a presence test; the shim must load first)
        have = True
    except ImportError:
        have = False
    if not have:
        if os.path.exists(VENV_PYTHON) and os.path.realpath(sys.executable) != os.path.realpath(VENV_PYTHON):
            os.execv(VENV_PYTHON, [VENV_PYTHON, "-I", os.path.abspath(__file__)] + sys.argv[1:])
        skip("Unicorn is not installed: run tests/diff/setup_venv.sh (makes tools/diff-venv)")
    if platform.system() == "Darwin" and platform.machine() == "arm64":
        import ctypes
        src = os.path.join(HERE, "ucshim.c")
        lib = os.path.join(work, "ucshim.dylib")
        if not os.path.exists(lib) or os.path.getmtime(lib) < os.path.getmtime(src):
            subprocess.run(["cc", "-shared", "-O2", "-o", lib, src], check=True, timeout=120)
        ctypes.CDLL(lib).ucshim_install()
    sys.path.insert(0, HERE)
    import orig_x86
    return orig_x86


# ---------------------------------------------------------------- prototypes

TYPE_KINDS = {
    "void": "void", "float": "f32", "double": "f64", "float10": "f64",
    "int": "i32", "signed int": "i32", "signed": "i32", "long": "i32", "long int": "i32",
    "int32_t": "i32", "BOOL": "i32",
    "unsigned int": "u32", "unsigned": "u32", "uint": "u32", "undefined4": "u32", "uint32_t": "u32",
    "unsigned long": "u32", "ulong": "u32", "dword": "u32", "undefined3": "u32",
    "short": "i16", "short int": "i16", "signed short": "i16", "int16_t": "i16",
    "unsigned short": "u16", "ushort": "u16", "undefined2": "u16", "word": "u16", "uint16_t": "u16",
    "char": "i8", "signed char": "i8", "int8_t": "i8",
    "unsigned char": "u8", "byte": "u8", "uchar": "u8", "undefined1": "u8", "undefined": "u8",
    "uint8_t": "u8", "_Bool": "b", "bool": "b",
    "long long": "i64", "longlong": "i64", "int64_t": "i64", "long long int": "i64",
    "unsigned long long": "u64", "ulonglong": "u64", "undefined8": "u64", "uint64_t": "u64",
    "qword": "u64",
}
TYPE_WORDS = set(w for k in TYPE_KINDS for w in k.split()) | {"const", "volatile", "struct"}
C_TYPES = {"i8": "int8_t", "u8": "uint8_t", "b": "_Bool", "i16": "int16_t", "u16": "uint16_t",
           "i32": "int32_t", "u32": "uint32_t", "i64": "int64_t", "u64": "uint64_t",
           "f32": "float", "f64": "double", "ptr": "void *", "str": "void *", "void": "void"}
WIDE = ("i64", "u64", "f64")
INT_KINDS = ("i8", "u8", "i16", "u16", "i32", "u32")


def type_kind(text):
    text = text.strip()
    if "(" not in text and "[" not in text and text.count("*") == 1:
        base = [w for w in text.split("*")[0].split() if w not in ("const", "volatile")]
        if base == ["char"]:
            return "str"              # a string: points into the text area of the scratch
    if "(" in text or "*" in text or "[" in text:
        return "ptr"
    words = [w for w in text.split() if w not in ("const", "volatile", "register")]
    if len(words) > 1 and words[-1] not in TYPE_WORDS:
        words = words[:-1]            # parameter name
    t = " ".join(words)
    if t in TYPE_KINDS:
        return TYPE_KINDS[t]
    return None                        # struct by value, varargs, unknown typedef


def split_params(text):
    parts, depth, cur = [], 0, ""
    for ch in text:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += ch
    parts.append(cur)
    return [p.strip() for p in parts]


def parse_decls(path):
    """{addr: dict(module, name, ret, params, sig)} from the generated of_decls.h."""
    text = open(path).read()
    funcs = {}
    for m in re.finditer(r"/\* 0x([0-9a-f]{8}) (\S+) [^*]*\*/\n([^;]*);", text):
        addr = int(m.group(1), 16)
        sig = " ".join(m.group(3).split())
        open_at = sig.index("(")
        head = sig[:open_at]
        name = re.search(r"(\w+)\s*$", head).group(1)
        ret_text = head[:head.rstrip().rfind(name)]
        args = sig[open_at + 1:sig.rindex(")")].strip()
        params = [] if args in ("", "void") else split_params(args)
        kinds = [type_kind(p) if p != "..." else None for p in params]
        funcs[addr] = {"module": m.group(2), "name": name, "ret": type_kind(ret_text),
                       "params": kinds, "sig": sig}
    return funcs


def read_function_table():
    table = {}
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if line.startswith("#"):
            continue
        cols = line.rstrip("\n").split("\t")
        table[int(cols[0], 16)] = {"module": cols[1], "size": int(cols[5])}
    return table


# ---------------------------------------------------------------- guest ELF symbols

def read_elf(path):
    data = open(path, "rb").read()
    shoff, = struct.unpack_from("<Q", data, 0x28)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x3A)
    sections = []
    for i in range(shnum):
        sh = struct.unpack_from("<IIQQQQIIQQ", data, shoff + i * shentsize)
        sections.append(sh)
    names = sections[shstrndx]
    def sec_name(off):
        start = names[4] + off
        return data[start:data.index(b"\0", start)].decode()
    secs = []
    for sh in sections:
        secs.append({"name": sec_name(sh[0]), "type": sh[1], "flags": sh[2], "addr": sh[3],
                     "offset": sh[4], "size": sh[5], "link": sh[6], "entsize": sh[9]})
    symbols = []
    for s in secs:
        if s["type"] != 2:
            continue
        strtab = secs[s["link"]]
        for off in range(s["offset"], s["offset"] + s["size"], 24):
            name_off, info, other, shndx, value, size = struct.unpack_from("<IBBHQQ", data, off)
            start = strtab["offset"] + name_off
            name = data[start:data.index(b"\0", start)].decode()
            symbols.append((name, value, size, info & 0xf, shndx))
    return data, secs, symbols


class PortImage:
    """Address maps between the guest image and the XBE (from symbol names)."""

    def __init__(self, elf_path, decls, xbe_data_ranges):
        self.data, self.secs, symbols = read_elf(elf_path)
        self.writable = [(s["addr"], s["addr"] + s["size"], s) for s in self.secs
                         if s["flags"] & 2 and s["flags"] & 1 and s["addr"]]
        self.rodata = [(s["addr"], s["addr"] + s["size"]) for s in self.secs
                       if s["name"] == ".rodata"]
        orig_names = {}
        for addr, f in decls.items():
            orig_names[f["name"]] = addr
            orig_names["FUN_%08x" % addr] = addr
        maps = []
        self.duplicates = []
        self.func_port_to_orig = {}
        # asm_convert.py drops .type/.size: most symbols are NOTYPE with size 0. A data
        # symbol's size is the gap to the next symbol of its section (cut below at the next
        # mapped original address).
        text_index = [i for i, s in enumerate(self.secs) if s["name"] == ".text"]
        data_index = set(i for i, s in enumerate(self.secs)
                         if s["name"] in (".data", ".bss", ".rodata", ".guest_init"))
        starts = {}
        for name, value, size, kind, shndx in symbols:
            if shndx in data_index and kind in (0, 1):
                starts.setdefault(shndx, set()).add(value)
        sorted_starts = {k: sorted(v) for k, v in starts.items()}
        for name, value, size, kind, shndx in symbols:
            if shndx in text_index and name in orig_names:
                self.func_port_to_orig[value] = orig_names[name]
            if shndx not in data_index or kind not in (0, 1):
                continue
            m = re.search(r"(?:^|_)(00[0-9a-f]{6})$", name)
            if not m:
                continue
            orig = int(m.group(1), 16)
            if not any(lo <= orig < hi for lo, hi in xbe_data_ranges):
                continue
            if size == 0:
                s = self.secs[shndx]
                ss = sorted_starts[shndx]
                i = bisect.bisect_right(ss, value)
                end = ss[i] if i < len(ss) else s["addr"] + s["size"]
                size = end - value
            if size:
                maps.append((orig, orig + size, value, name))
        by_name = {name: value for name, value, size, kind, shndx in symbols if shndx in data_index}
        blobs = []
        for line in open(os.path.join(HERE, "blobs.tsv")):
            if line.startswith("#") or not line.strip():
                continue
            name, lo, hi = line.split()
            if name in by_name:
                blobs.append((int(lo, 16), int(hi, 16), by_name[name], name))
        maps.sort()
        # an inferred size that runs into the next original address is cut there, unless
        # that next symbol is an alias at the same distance (a block with aliases)
        j = 0
        for i in range(len(maps)):
            orig, end, port, name = maps[i]
            j = max(j, i + 1)
            while j < len(maps) and maps[j][0] <= orig:
                j += 1
            k = j
            while k < len(maps) and maps[k][0] < end:
                if maps[k][2] - port != maps[k][0] - orig:
                    maps[i] = (orig, maps[k][0], port, name)
                    break
                k += 1
        # one port location per original byte: keep the first symbol; two symbols that
        # name the same original address at two port places are two storages (a bug)
        named = []
        for orig, end, port, name in maps:
            if named and orig < named[-1][1]:
                prev = named[-1]
                if port - orig != prev[2] - prev[0]:
                    self.duplicates.append((name, prev[3]))
                continue
            named.append((orig, end, port, name))
        # Inside a declared block the block wins: a named symbol of another module there
        # keeps only its first word (its size is inferred, the block's is declared), and a
        # named symbol that starts before the block ends at the block's start.
        for lo, hi, port, name in blobs:
            for i, (n_lo, n_hi, n_port, n_name) in enumerate(named):
                if n_hi <= lo or n_lo >= hi or n_port - n_lo == port - lo:
                    continue
                if n_lo < lo:
                    named[i] = (n_lo, lo, n_port, n_name)
                else:
                    named[i] = (n_lo, min(n_hi, n_lo + 4), n_port, n_name)
        # the declared blocks fill what the named symbols leave free
        keys = [m[0] for m in named]
        pieces = []
        for lo, hi, port, name in blobs:
            cur = lo
            i = max(bisect.bisect_right(keys, lo) - 1, 0)
            while i < len(named) and named[i][0] < hi:
                n_lo, n_hi = named[i][0], named[i][1]
                if n_hi > cur:
                    if n_lo > cur:
                        pieces.append((cur, n_lo, port + cur - lo, name))
                    cur = max(cur, n_hi)
                i += 1
            if cur < hi:
                pieces.append((cur, hi, port + cur - lo, name))
        self.o2p = sorted(named + pieces)
        self.o2p_keys = [m[0] for m in self.o2p]
        self.p2o = sorted((port, port + (end - orig), orig, name)
                          for orig, end, port, name in self.o2p)
        self.p2o_keys = [m[0] for m in self.p2o]

    def orig_to_port(self, address):
        i = bisect.bisect_right(self.o2p_keys, address) - 1
        if i >= 0 and address < self.o2p[i][1]:
            return self.o2p[i][2] + address - self.o2p[i][0]
        return None

    def port_to_orig(self, address):
        i = bisect.bisect_right(self.p2o_keys, address) - 1
        if i >= 0 and address < self.p2o[i][1]:
            return self.p2o[i][2] + address - self.p2o[i][0]
        return None

    def initial_byte(self, address):
        for s in self.secs:
            if s["addr"] <= address < s["addr"] + s["size"] and s["flags"] & 2:
                if s["type"] == 8:      # NOBITS
                    return 0
                return self.data[s["offset"] + address - s["addr"]]
        return None

    def diff_ranges(self):
        """Writable mapped globals: the driver compares and restores these."""
        out = []
        for port, end, orig, name in self.p2o:
            if any(lo <= port and end <= hi for lo, hi, _ in self.writable):
                out.append((port, end - port))
        return out

    def outside_image(self, address):
        """A guest address beyond every section of the image, outside the low 16 MB."""
        end = max(s["addr"] + s["size"] for s in self.secs if s["addr"])
        # also the XBE range: x86 has its image mapped there, so a bad pointer that x86
        # follows into its own image faults only in the port
        return address >= end or 0x01000000 <= address < 0x88000000 or \
            XBE_LO <= address < XBE_HI

    def is_unmapped_data(self, value):
        """A pointer into the port's data whose original address is not known."""
        in_data = any(lo <= value < hi for lo, hi in self.rodata) or any(
            lo <= value < hi for lo, hi, _ in self.writable)
        return in_data and self.port_to_orig(value) is None


# ---------------------------------------------------------------- static x86 facts

def scan_function(cs, xbe_bytes_at, addr, size):
    """ret immediates, direct call targets, indirect calls (linear sweep of the body)."""
    code = xbe_bytes_at(addr, size)
    rets, calls, indirect = set(), set(), False
    for insn in cs.disasm(code, addr):
        if insn.mnemonic == "ret":
            rets.add(int(insn.op_str, 16) if insn.op_str else 0)
        elif insn.mnemonic == "call":
            try:
                calls.add(int(insn.op_str, 16))
            except ValueError:
                indirect = True
        elif insn.mnemonic == "jmp" and not insn.op_str.startswith("0x"):
            if "[" in insn.op_str and "*" not in insn.op_str:
                indirect = True        # tail call through a pointer (not a jump table)
    return rets, calls, indirect


def classify_levels(funcs, table, scans, orig):
    """leaf: no calls but C library ones; l1: calls only leaf functions and the C library."""
    def is_libc(t):
        return any(lo <= t < hi for lo, hi in orig.LIBC_REGIONS) and t not in orig.LIBC_DENY
    level = {}
    for addr in funcs:
        rets, calls, indirect = scans[addr]
        if not indirect and all(is_libc(t) for t in calls):
            level[addr] = "leaf"
    for addr in funcs:
        if addr in level:
            continue
        rets, calls, indirect = scans[addr]
        if not indirect and all(is_libc(t) or level.get(t) == "leaf" for t in calls):
            level[addr] = "l1"
        else:
            level[addr] = "deep"
    return level


# ---------------------------------------------------------------- inputs

INT_POOL = [0, 1, 2, 3, 4, 7, 15, 16, 31, 100, 255, 1000, -1, -2, -100, 0x7fffffff,
            -0x80000000, 0x10000, 0x12345678]
FLOAT_POOL = [0.0, -0.0, 1.0, -1.0, 0.5, 2.0, 3.14159265, -2.5, 10.0, 100.0, -1000.0,
              0.001, 1e6, 0.75, -0.25]


def f32_bits(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


def f64_words(x):
    lo, hi = struct.unpack("<II", struct.pack("<d", x))
    return [lo, hi]


def make_images():
    """Scratch contents: a word mix (pointers into the scratch window, small integers,
    floats in [-100, 100], zeros, a few raw random words), and ASCII text with NUL
    terminators from TEXT_OFF on (where string arguments point)."""
    images = []
    for k in range(IMAGES):
        rng = random.Random(0x5EED0000 + k)
        words = []
        for _ in range(TEXT_OFF // 4):
            r = rng.random()
            if r < 0.25:
                words.append(SCRATCH_BASE + rng.randrange(SCRATCH_SIZE // 16) * 16)
            elif r < 0.45:
                words.append(rng.randrange(16))
            elif r < 0.75:
                words.append(f32_bits(rng.uniform(-100.0, 100.0)))
            elif r < 0.90:
                words.append(0)
            elif r < 0.97:
                words.append(f32_bits(rng.choice([1.0, 0.5, -1.0, 2.0])))
            else:
                words.append(rng.getrandbits(32))
        text = bytearray()
        while len(text) < SCRATCH_SIZE - TEXT_OFF:
            text += bytes(rng.choice(b"abcdefghijklmnopqrstuvwxyz0123456789 _.ABCDEF")
                          for _ in range(rng.randrange(1, 24))) + b"\0"
        images.append(struct.pack("<%dI" % len(words), *words) + bytes(text[:SCRATCH_SIZE - TEXT_OFF]))
    return images


def gen_value(kind, j, case, rng):
    """One parameter value: (port words, description)."""
    typical = case == 0
    zero = case == 1
    if kind == "str":
        off = 0 if typical else rng.randrange(0, 0x400)
        return [SCRATCH_BASE + TEXT_OFF + j * 0x800 + off], "s%d+%x" % (j, off)
    if kind == "ptr":
        if case == 2 and j == 0:
            return [0], "NULL"
        off = 0 if typical or zero else rng.choice([0, 0, 0, 0x40, 0x400])
        v = SCRATCH_BASE + 0x1000 + j * PTR_SLOT + off
        return [v], "p%d+%x" % (j, off)
    if kind in ("f32", "f64"):
        x = 1.5 if typical else 0.0 if zero else (
            rng.choice(FLOAT_POOL) if rng.random() < 0.6 else rng.uniform(-1000.0, 1000.0))
        return (f64_words(x) if kind == "f64" else [f32_bits(x)]), repr(x)
    if kind == "b":
        v = 1 if typical else 0 if zero else rng.randrange(2)
        return [v], str(v)
    if typical:
        v = 2
    elif zero:
        v = 0
    else:
        v = rng.choice(INT_POOL) if rng.random() < 0.7 else rng.getrandbits(32)
    if kind in ("i64", "u64"):
        v64 = v if rng.random() < 0.5 else (v | (rng.getrandbits(32) << 32))
        v64 &= (1 << 64) - 1
        return [v64 & 0xffffffff, v64 >> 32], hex(v64)
    bits = {"i8": 8, "u8": 8, "i16": 16, "u16": 16}.get(kind, 32)
    v &= (1 << bits) - 1
    if kind[0] == "i" and v >= 1 << (bits - 1):
        v -= 1 << bits
    return [v & 0xffffffff], str(v)


# ---------------------------------------------------------------- port driver build

def ninja_commands(ninja, guest_dir):
    out = subprocess.run([ninja, "-C", guest_dir, "-t", "commands", "openfrenzy_guest"],
                         check=True, capture_output=True, text=True, timeout=120).stdout
    lines = out.splitlines()
    compile_cmd = [l for l in lines if "/src/core/Core/core.c" in l and " -c " in l]
    link_cmd = [l for l in lines if "ld.lld" in l]
    if not compile_cmd or not link_cmd:
        raise SystemExit("run_diff: no game build commands in %s (build the project first)" % guest_dir)
    return compile_cmd[-1], link_cmd[-1]


def gen_dispatch(tested, decls, path):
    lines = ["/* generated by tests/diff/run_diff.py; do not edit */",
             "#include <stdint.h>", "#include <string.h>",
             '#include "%s"' % os.path.join(HERE, "diff_driver.c"),
             "static float F(uint32_t x) { float f; memcpy(&f, &x, 4); return f; }",
             "static double D(uint32_t lo, uint32_t hi) { uint64_t v = ((uint64_t)hi << 32) | lo;"
             " double d; memcpy(&d, &v, 8); return d; }",
             "static uint64_t Q(uint32_t lo, uint32_t hi) { return ((uint64_t)hi << 32) | lo; }",
             "int of_diff_call(uint32_t function, const uint32_t *a, struct of_diff_result *r)",
             "{", "\tswitch (function)", "\t{"]
    for index, addr in enumerate(tested):
        f = decls[addr]
        types, args, w = [], [], 0
        for kind in f["params"]:
            types.append(C_TYPES[kind])
            if kind in ("ptr", "str"):
                args.append("(void *)(uintptr_t)a[%d]" % w)
            elif kind == "f32":
                args.append("F(a[%d])" % w)
            elif kind == "f64":
                args.append("D(a[%d], a[%d])" % (w, w + 1))
            elif kind in ("i64", "u64"):
                args.append("(%s)Q(a[%d], a[%d])" % (C_TYPES[kind], w, w + 1))
            else:
                args.append("(%s)a[%d]" % (C_TYPES[kind], w))
            w += 2 if kind in WIDE else 1
        ret = f["ret"]
        call = "((%s (*)(%s))FUN_%08x)(%s)" % (C_TYPES[ret], ", ".join(types) or "void", addr,
                                               ", ".join(args))
        if ret == "void":
            body = "%s;" % call
        elif ret in ("f32", "f64"):
            body = "r->f = %s;" % call
        elif ret in ("ptr", "str"):
            body = "r->w[0] = (uint32_t)(uintptr_t)%s;" % call
        elif ret in ("i64", "u64"):
            body = "{ uint64_t v = (uint64_t)%s; r->w[0] = (uint32_t)v; r->w[1] = (uint32_t)(v >> 32); }" % call
        else:
            body = "r->w[0] = (uint32_t)(int32_t)%s;" % call
        lines.append("\tcase %d: %s return 0; /* %s */" % (index, body, f["name"]))
    lines += ["\tdefault: return 1;", "\t}", "}", ""]
    open(path, "w").write("\n".join(lines))


def build_driver(ninja, build, work, tested, decls):
    guest_dir = os.path.join(build, "guest-main")
    compile_cmd, link_cmd = ninja_commands(ninja, guest_dir)
    src = os.path.join(work, "diff_cases.c")
    obj = os.path.join(work, "diff_cases.o")
    elf = os.path.join(work, "diff_driver.elf")
    gen_dispatch(tested, decls, src)
    cmd = re.sub(r" -MD -MT \S+ -MF \S+", "", compile_cmd)
    cmd = re.sub(r" -o \S+ ", " -o %s " % shlex.quote(obj), cmd)
    cmd = re.sub(r" -c \S+$", " -c %s" % shlex.quote(src), cmd)
    r = subprocess.run(cmd, shell=True, cwd=guest_dir, capture_output=True, text=True, timeout=600)
    if r.returncode != 0:
        raise SystemExit("run_diff: driver compile failed:\n" + r.stderr[-4000:])
    # the game image's link line, without of_boot (main) and with the driver
    link = link_cmd.split("&&")[1] if "&&" in link_cmd else link_cmd
    parts = shlex.split(link)
    out = []
    skip_next = False
    for i, p in enumerate(parts):
        if skip_next:
            skip_next = False
            continue
        if p in ("-o", "-Map"):
            skip_next = True
            continue
        if p.endswith(".obj") and "/runtime/of_boot.c" in p:
            continue
        out.append(p)
    out += [obj, "-o", elf]
    r = subprocess.run(out, cwd=guest_dir, capture_output=True, text=True, timeout=600)
    if r.returncode != 0:
        raise SystemExit("run_diff: driver link failed:\n" + r.stderr[-4000:])
    return elf


# ---------------------------------------------------------------- port run

def write_cases(path, images, ranges, cases):
    with open(path, "wb") as f:
        f.write(b"OFDF" + struct.pack("<IIIIII", 1, SCRATCH_BASE, SCRATCH_SIZE, len(images),
                                      len(ranges), len(cases)))
        for img in images:
            f.write(img)
        for addr, size in ranges:
            f.write(struct.pack("<II", addr, size))
        for i, (index, image, words) in enumerate(cases):
            f.write(struct.pack("<IIII", i, index, image, len(words)))
            f.write(struct.pack("<%dI" % len(words), *[w & 0xffffffff for w in words]))


def parse_output(path):
    results, current, order = {}, None, []
    if not os.path.exists(path):
        return results, None, False
    done = False
    for line in open(path):
        parts = line.split()
        if not parts:
            continue
        if parts[0] == "B":
            current = int(parts[1])
            order.append(current)
            results[current] = {"ret": None, "changed": {}, "complete": False}
        elif parts[0] == "R":
            r = results[int(parts[1])]
            r["ret"] = (int(parts[2], 16), int(parts[3], 16),
                        struct.unpack("<d", struct.pack("<Q", int(parts[4], 16)))[0])
        elif parts[0] == "M":
            address = int(parts[1], 16)
            raw = bytes.fromhex(parts[2])
            for i, b in enumerate(raw):
                results[current]["changed"][address + i] = b
        elif parts[0] == "E":
            results[int(parts[1])]["complete"] = True
        elif parts[0] == "DONE":
            done = True
    last = order[-1] if order else None
    return results, last, done


def run_port(host, elf, work, ncases, timeout, log):
    """Run every case; restart after a fault or hang. {case: result or status}."""
    cases_path = os.path.join(work, "cases.bin")
    out_path = os.path.join(work, "port_out.txt")
    env = dict(os.environ)
    env.update({"OF_DIFF_CASES": cases_path, "OF_DIFF_OUT": out_path,
                "SDL_VIDEO_DRIVER": "dummy", "SDL_AUDIO_DRIVER": "dummy",
                "OPENFRENZY_AUDIO_ENABLED": "false", "OPENFRENZY_WINDOWED": "1",
                "OPENFRENZY_DATA_ROOT": os.path.join(work, "data"),
                "OPENFRENZY_SAVE_ROOT": os.path.join(work, "saves")})
    os.makedirs(env["OPENFRENZY_DATA_ROOT"], exist_ok=True)
    os.makedirs(env["OPENFRENZY_SAVE_ROOT"], exist_ok=True)
    statuses = {}
    first = 0
    if os.path.exists(out_path):
        os.remove(out_path)
    restarts = 0
    with open(log, "w") as logf:
        while first < ncases:
            env["OF_DIFF_FIRST"] = str(first)
            logf.write("--- run from case %d\n" % first)
            logf.flush()
            log_start = logf.tell()
            # the child has its own hard limit (perl alarm), and is killed when its output
            # stops growing for STALL seconds (one case that hangs)
            proc = subprocess.Popen(["perl", "-e", "alarm shift; exec @ARGV", str(timeout), host, elf],
                                    env=env, stdout=logf, stderr=subprocess.STDOUT)
            hung = False
            last_size, last_change = -1, time.time()
            while proc.poll() is None:
                time.sleep(0.05)
                size = os.path.getsize(out_path) if os.path.exists(out_path) else 0
                if size != last_size:
                    last_size, last_change = size, time.time()
                elif time.time() - last_change > STALL:
                    hung = True
                    proc.kill()
                    proc.wait()
            code = proc.returncode
            results, last, done = parse_output(out_path)
            if done:
                break
            if last is None or results[last]["complete"]:
                # died between cases (or before the first): the harness cannot go on
                raise SystemExit("run_diff: the port driver stopped outside a case (exit %s, log %s)"
                                 % (code, log))
            if hung:
                statuses[last] = ("hang", None)
            else:
                with open(log) as lf:
                    lf.seek(log_start)
                    faults = re.findall(r"OpenFrenzy fault \d+ at 0x([0-9a-f]+)", lf.read())
                address = int(faults[-1], 16) & 0xffffffff if faults else None
                statuses[last] = ("fault", address)
            first = last + 1
            restarts += 1
    results, _, _ = parse_output(out_path)
    return results, statuses, restarts


# ---------------------------------------------------------------- compare

def is_finite32(bits):
    return (bits >> 23) & 0xff != 0xff


def is_nan32(bits):
    return (bits >> 23) & 0xff == 0xff and bits & 0x7fffff != 0


def to_f32(x):
    """Round a Python float to float precision (overflow to infinity, as a store does)."""
    try:
        return struct.unpack("<f", struct.pack("<f", x))[0]
    except OverflowError:
        return math.copysign(math.inf, x)


def is_normal_or_zero(bits):
    e = (bits >> 23) & 0xff
    return e != 0xff and (e != 0 or bits & 0x7fffffff == 0)


def floats_close(a, b):
    if math.isnan(a) or math.isnan(b) or math.isinf(a) or math.isinf(b):
        return a == b or (math.isnan(a) and math.isnan(b))
    d = abs(a - b)
    return d <= FLOAT_ABS or d <= FLOAT_REL * max(abs(a), abs(b))


def f32(bits):
    return struct.unpack("<f", struct.pack("<I", bits & 0xffffffff))[0]


class Comparer:
    def __init__(self, port, orig_runner, images):
        self.port = port
        self.orig = orig_runner
        self.images = images
        self.xbe_ro = [(va, va + vs) for name, va, vs, _ in orig_runner.sections]

    def orig_initial(self, a, image):
        if SCRATCH_BASE <= a < SCRATCH_BASE + SCRATCH_SIZE:
            return self.images[image][a - SCRATCH_BASE]
        return self.orig.initial_byte(a)

    def port_initial(self, a, image):
        """a is an original-space address."""
        if SCRATCH_BASE <= a < SCRATCH_BASE + SCRATCH_SIZE:
            return self.images[image][a - SCRATCH_BASE]
        p = self.port.orig_to_port(a)
        return None if p is None else self.port.initial_byte(p)

    def normalize_port_word(self, value):
        o = self.port.port_to_orig(value)
        if o is not None:
            return o, "global"
        if value in self.port.func_port_to_orig:
            return self.port.func_port_to_orig[value], "function"
        return value, None

    def words_equal(self, ow, pw, is_float=False):
        """'same', 'ptr' (pointer to unmapped constant data), 'float', 'range' or None
        (different). The float rules apply only to a word that x86 stored with an x87 or
        SSE instruction (is_float)."""
        if ow == pw:
            return "same"
        n, how = self.normalize_port_word(pw)
        if n == ow:
            return "same"
        if self.port.is_unmapped_data(pw) and any(lo <= ow < hi for lo, hi in self.xbe_ro):
            return "ptr"
        if not is_float:
            # a float constant that x86 stores with an integer move: a few ulps apart, both
            # normal floats of a usual size (integers almost never look like this)
            if (ow ^ pw) & 0x80000000 == 0 and abs(ow - pw) <= 4 and is_normal_or_zero(ow) \
                    and 1e-10 <= abs(f32(ow)) <= 1e10:
                return "float"
            return None
        if is_nan32(ow) and is_nan32(pw):
            return "float"            # x87 makes negative NaNs, ARM positive ones
        if (ow ^ pw) & 0x80000000 == 0 and (ow >> 23) & 0xff == 0 and (pw >> 23) & 0xff == 0:
            return "float"            # both denormal: x87 rounds them from 64-bit mantissas
        if not (is_finite32(ow) and is_finite32(pw)):
            return "range"            # x87's wider exponent did not overflow (or did)
        if is_normal_or_zero(ow) and is_normal_or_zero(pw) and floats_close(f32(ow), f32(pw)):
            return "float"
        return None

    def compare_memory(self, ochanged, pchanged_port, image, float_bytes=(), float_values=()):
        """List of (orig word address, x86 word, port word, verdict) for differing words,
        and the number of x86-changed bytes that have no port location."""
        pchanged = {}
        for pa, b in pchanged_port.items():
            if SCRATCH_BASE <= pa < SCRATCH_BASE + SCRATCH_SIZE:
                pchanged[pa] = b
            else:
                oa = self.port.port_to_orig(pa)
                pchanged[oa if oa is not None else ("port", pa)] = b
        unmapped = 0
        words = set()
        for a in set(ochanged) | set(pchanged):
            if isinstance(a, tuple):
                words.add(a)
                continue
            if self.port_initial(a, image) is None and a not in pchanged:
                unmapped += 1
                continue
            words.add(a & ~3)
        out = []
        for w in sorted(words, key=lambda x: (1, x[1]) if isinstance(x, tuple) else (0, x)):
            if isinstance(w, tuple):
                out.append((w, None, pchanged[w], None))
                continue
            ob, pb = [], []
            skip_word = False
            for a in range(w, w + 4):
                o = ochanged.get(a, self.orig_initial(a, image))
                p = pchanged.get(a, self.port_initial(a, image))
                if o is None or p is None:
                    skip_word = True
                    break
                ob.append(o)
                pb.append(p)
            if skip_word:
                # partially mapped word: compare the bytes one by one
                for a in range(w, w + 4):
                    if a in ochanged or a in pchanged:
                        o = ochanged.get(a, self.orig_initial(a, image))
                        p = pchanged.get(a, self.port_initial(a, image))
                        if o is not None and p is not None and o != p:
                            out.append((a, o, p, None))
                continue
            ow = struct.unpack("<I", bytes(ob))[0]
            pw = struct.unpack("<I", bytes(pb))[0]
            # a float word: x86 stored it with an x87/SSE store, or copied a value that such a
            # store made (a structure copy with integer moves)
            verdict = self.words_equal(ow, pw, ow in float_values or
                                       any(a in float_bytes for a in range(w, w + 4)))
            if verdict != "same":
                out.append((w, ow, pw, verdict))
        return out, unmapped


def compare_return(kind, o, pret, comparer):
    """None if equal, 'float' if only float-close, else a description."""
    w0, w1, fv = pret
    if kind == "void":
        return None
    if kind in ("f32", "f64"):
        st0 = o["st0"]
        if st0 is None:
            return "x86 leaves no ST0"
        if kind == "f32":
            st0 = to_f32(st0)
        if st0 == fv or (math.isnan(st0) and math.isnan(fv)):
            return None
        if floats_close(st0, fv) or math.isfinite(st0) != math.isfinite(fv):
            return "float"
        return "ret %r != x86 %r" % (fv, o["st0"])
    eax, edx = o["eax"], o["edx"]
    if kind in ("i64", "u64"):
        return None if (eax, edx) == (w0, w1) else "ret %08x%08x != x86 %08x%08x" % (w1, w0, edx, eax)
    mask = {"i8": 0xff, "u8": 0xff, "b": 0xff, "i16": 0xffff, "u16": 0xffff}.get(kind, 0xffffffff)
    if (eax & mask) == (w0 & mask):
        return None
    if mask == 0xffffffff and w0 in (0, 1) and eax & 0xff == w0:
        return None                   # x86 returns a bool in AL; the rest of EAX is left over
    if kind in ("ptr", "str"):
        v = comparer.words_equal(eax, w0)
        if v in ("same", "ptr"):
            return None
    return "ret %08x != x86 %08x" % (w0 & mask, eax & mask)


def case_problems(f, o, pr, v, comparer):
    """(problems, float diffs, range diffs, pointer diffs, unmapped bytes) of one case."""
    problems, floaty, ranges, ptrish = [], 0, 0, 0
    rdiff = compare_return(f["ret"], o, pr["ret"], comparer)
    if rdiff == "float":
        floaty += 1
    elif rdiff:
        problems.append(rdiff)
    mem, unmapped = comparer.compare_memory(o["changed"], pr["changed"], v["image"],
                                            o.get("float_bytes", ()), o.get("float_values", ()))
    for w, ow, pw, verdict in mem:
        if verdict == "float":
            floaty += 1
        elif verdict == "range":
            floaty += 1
            ranges += 1
        elif verdict == "ptr":
            ptrish += 1
        else:
            where = ("port-only global 0x%08x" % w[1]) if isinstance(w, tuple) else "0x%08x" % w
            problems.append("mem %s: port %s x86 %s" % (
                where, "%08x" % pw if pw is not None else "-", "%08x" % ow if ow is not None else "-"))
    return problems, floaty, ranges, ptrish, unmapped


# ---------------------------------------------------------------- main

def x86_layout(kinds, words_per, rets):
    """Return (ecx index, edx index, stack word indexes, expected pop) or a skip reason."""
    total = sum(words_per)
    first_ok = len(kinds) >= 1 and words_per[0] == 1
    second_ok = len(kinds) >= 2 and words_per[1] == 1
    nonzero = [r for r in rets if r]
    if len(nonzero) > 1 or (nonzero and 0 in rets):
        return "abi: several ret forms %s" % sorted(rets)
    if not nonzero:
        return (0 if total >= 1 else None, 1 if total >= 2 else None, list(range(total)), 0)
    k = nonzero[0] // 4
    if k == total:
        return (None, None, list(range(total)), k * 4)
    if k == total - 1 and first_ok:
        return (0, None, list(range(1, total)), k * 4)
    if k == total - 2 and first_ok and second_ok:
        return (0, 1, list(range(2, total)), k * 4)
    return "abi: the port passes %d argument words, x86 pops %d" % (total, k)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", default=os.path.join(ROOT, "build"))
    ap.add_argument("--xbe", default=os.path.join(ROOT, "orig", "default.xbe"))
    ap.add_argument("--ninja", default="ninja")
    ap.add_argument("--scope", choices=("leaf", "l1", "all"), default="l1")
    ap.add_argument("--list", help="file of addresses to test (regression list)")
    ap.add_argument("--only", help="comma-separated addresses")
    ap.add_argument("--module", help="only functions of these modules (comma-separated)")
    ap.add_argument("--cases", type=int, default=12)
    ap.add_argument("--report", help="write a TSV line per function")
    ap.add_argument("--details", type=int, default=3, help="differing cases shown per function")
    ap.add_argument("--timeout", type=int, default=120, help="seconds per port process")
    ap.add_argument("--work")
    args = ap.parse_args()

    started = time.time()
    if not os.path.exists(args.xbe):
        skip("no %s (the harness needs the original XBE)" % args.xbe)
    build = os.path.abspath(args.build)
    work = os.path.abspath(args.work or os.path.join(build, "test-diff"))
    os.makedirs(work, exist_ok=True)
    orig = import_unicorn(work)
    import capstone

    decls_path = os.path.join(build, "guest-main", "gen", "of_decls.h")
    guest_elf = os.path.join(build, "openfrenzy_guest.elf")
    host = os.path.join(build, "openfrenzy")
    for p in (decls_path, guest_elf, host):
        if not os.path.exists(p):
            print("run_diff: %s is missing (build the project first)" % p)
            return 2
    decls = parse_decls(decls_path)
    table = read_function_table()

    runner = orig.X86Runner(args.xbe, SCRATCH_BASE, SCRATCH_SIZE, set(table))
    images = make_images()
    cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

    def xbe_at(addr, size):
        lo = addr - runner.image_lo
        return bytes(runner.pristine[lo:lo + size])

    known = {}
    for line in open(os.path.join(HERE, "known.txt")):
        line = line.strip()
        if line and not line.startswith("#"):
            a, _, reason = line.partition(" ")
            known[int(a, 16)] = reason.strip()
    scans = {a: scan_function(cs, xbe_at, a, table[a]["size"]) for a in decls if a in table}
    level = classify_levels(scans, table, scans, orig)

    if args.list or args.only:
        wanted = []
        if args.list:
            for line in open(args.list):
                line = line.split("#")[0].strip()
                if line:
                    wanted.append(int(line.split()[0], 16))
        if args.only:
            wanted += [int(x, 16) for x in args.only.split(",")]
        candidates = [a for a in wanted if a in decls]
        missing = [a for a in wanted if a not in decls]
        if missing:
            print("run_diff: unknown addresses: %s" % " ".join("%08x" % a for a in missing))
            return 2
    else:
        allowed = {"leaf": ("leaf",), "l1": ("leaf", "l1"), "all": ("leaf", "l1", "deep")}[args.scope]
        candidates = sorted(a for a in decls if a in scans and level[a] in allowed)
        if args.module:
            mods = set(args.module.split(","))
            candidates = [a for a in candidates if decls[a]["module"] in mods]

    # ---- x86 side
    funcs = {}
    for addr in candidates:
        f = decls[addr]
        info = {"addr": addr, "name": f["name"], "module": f["module"],
                "level": level.get(addr, "?"), "status": None, "note": "", "cases": []}
        funcs[addr] = info
        if f["ret"] is None or any(k is None for k in f["params"]):
            info["status"] = "skip"
            info["note"] = "type: unsupported parameter or return type (%s)" % f["sig"]
            continue
        words_per = [2 if k in WIDE else 1 for k in f["params"]]
        layout = x86_layout(f["params"], words_per, scans[addr][0])
        if isinstance(layout, str):
            info["status"] = "skip"
            info["note"] = layout
            continue
        ecx_i, edx_i, stack_idx, expect_pop = layout
        rng = random.Random(addr)
        valid = []
        stops = {}
        abi_note = None
        for c in range(args.cases):
            words, desc, wide = [], [], False
            for j, kind in enumerate(f["params"]):
                w, d = gen_value(kind, j, c, rng)
                words += w
                desc.append(d)
                if kind in INT_KINDS:
                    value = int(d, 0)
                    wide |= not 0 <= value <= 15
            image = c % IMAGES
            ecx = words[ecx_i] if ecx_i is not None else 0
            edx = words[edx_i] if edx_i is not None else 0
            stack = [words[i] for i in stack_idx]
            o = runner.run(addr, images[image], ecx, edx, stack)
            if o["status"] != "ok":
                key = o["status"].split("@")[0] if o["status"].startswith("fault") else o["status"].split(":")[0]
                stops[key] = stops.get(key, 0) + 1
                if o["status"].split(":")[0] in ("xdk", "xapi", "bink", "kernel", "libc", "time"):
                    info.setdefault("stop", o["status"])
                continue
            if o["popped"] != expect_pop:
                abi_note = "abi: x86 popped %d argument bytes, expected %d" % (o["popped"], expect_pop)
                break
            # the same case with other values in the registers that are not arguments
            o2 = runner.run(addr, images[image], ecx, edx, stack, poison=1)
            if o2["status"] != o["status"] or o2["changed"] != o["changed"] or (
                    f["ret"] in ("f32", "f64") and o2["st0"] != o["st0"]) or (
                    f["ret"] not in ("void", "f32", "f64") and (o2["eax"], o2["edx"]) != (o["eax"], o["edx"])):
                abi_note = "abi: the result depends on registers that are not arguments"
                break
            fd = o["fdepth"]
            if fd > 1:
                # the x87 stack grew by more than one value: an error path of the C library
                # (pow of a negative number, ...); the case is outside the domain
                stops["x87 depth %d" % fd] = stops.get("x87 depth %d" % fd, 0) + 1
                continue
            if f["ret"] in ("f32", "f64") and fd != 1:
                abi_note = "ABI note: the port returns %s, x86 leaves nothing in ST0" % f["ret"]
                break
            if f["ret"] not in ("f32", "f64") and fd != 0:
                abi_note = "ABI note: x86 returns a value in ST0, the port returns %s" % f["ret"]
                break
            valid.append({"case": c, "image": image, "words": words, "desc": desc, "orig": o,
                          "wide": wide, "ecx": ecx, "edx": edx, "stack": stack})
        if abi_note:
            # "ABI note": the prototypes differ in where the result goes. It matters only if
            # a caller reads that result; reported for review, not a failure.
            info["status"] = "abi-note" if abi_note.startswith("ABI note") else "skip"
            info["kind"] = "abi"
            info["note"] = abi_note
            continue
        if not valid:
            info["status"] = "skip"
            if "stop" in info:
                info["note"] = "calls library/kernel code: %s" % info["stop"]
            else:
                info["note"] = "no case completes on x86 (%s)" % ", ".join(
                    "%s x%d" % kv for kv in sorted(stops.items()))
            continue
        info["cases"] = valid
        info["x86_dropped"] = stops

    tested = [a for a in candidates if funcs[a]["status"] is None]
    t_orig = time.time() - started

    # ---- port side
    port_statuses, port_results, restarts = {}, {}, 0
    port = None
    if tested:
        elf = build_driver(args.ninja, build, work, tested, decls)
        port = PortImage(elf, decls, runner.data_ranges)
        comparer = Comparer(port, runner, images)
        flat = []
        for index, addr in enumerate(tested):
            for v in funcs[addr]["cases"]:
                v["id"] = len(flat)
                flat.append((index, v["image"], v["words"]))
        write_cases(os.path.join(work, "cases.bin"), images, port.diff_ranges(), flat)
        port_results, port_statuses, restarts = run_port(
            host, elf, work, len(flat), args.timeout, os.path.join(work, "port.log"))
    t_port = time.time() - started - t_orig

    # ---- compare
    for addr in tested:
        info = funcs[addr]
        f = decls[addr]
        real, oob, floaty, ptrish, unmapped_total, ranges, precision = [], [], 0, 0, 0, 0, 0
        for v in info["cases"]:
            cid = v["id"]
            o = v["orig"]
            if cid in port_statuses:
                what, fault_at = port_statuses[cid]
                text = "port %s%s, x86 returned" % (
                    what, "" if fault_at is None else " at 0x%08x" % fault_at)
                if fault_at is not None and port.outside_image(fault_at):
                    oob.append((v, [text + " (out of range on both sides?)"]))
                else:
                    real.append((v, [text]))
                continue
            pr = port_results.get(cid)
            if pr is None or not pr["complete"] or pr["ret"] is None:
                real.append((v, ["port did not run the case"]))
                continue
            problems, fl, rg, pt, um = case_problems(f, o, pr, v, comparer)
            if problems:
                # the precision oracle: x86 again with 24-bit x87 precision (each result
                # rounded to a float mantissa, as the port's float code does)
                o24 = runner.run(addr, images[v["image"]], v["ecx"], v["edx"], v["stack"], fpcw=0x007f)
                if o24["status"] == "ok" and not case_problems(f, o24, pr, v, comparer)[0]:
                    precision += 1
                    problems = []
            floaty += fl
            ranges += rg
            ptrish += pt
            unmapped_total += um
            if problems:
                real.append((v, problems))
        info["tested_cases"] = len(info["cases"])
        info["float_diffs"] = floaty
        info["unmapped"] = unmapped_total
        if real and addr in known:
            info["status"] = "known"
            info["kind"] = "known"
            info["note"] = "%d/%d cases differ; known: %s" % (len(real), len(info["cases"]), known[addr])
            info["real"] = real
        elif real and all(v["wide"] for v, _ in real):
            # differs only with an integer argument outside [0, 15] (an index past a
            # table, a huge count): both sides read beyond the data; reported, not counted
            info["status"] = "unclear"
            info["kind"] = "wide-int"
            info["note"] = "%d/%d cases differ, all with an integer argument outside [0, 15]" % (
                len(real), len(info["cases"]))
            info["real"] = real
        elif real:
            info["status"] = "mismatch"
            info["kind"] = "real"
            info["note"] = "%d/%d cases differ" % (len(real), len(info["cases"]))
            info["real"] = real
        elif oob:
            # the port faulted outside its image (past a global array, or in the window)
            # where x86 read other data of its own image: an input out of range, not a
            # difference of the C. Reported, not counted as a mismatch.
            info["status"] = "oob"
            info["kind"] = "oob"
            info["note"] = "%d/%d cases: port faults outside its image, x86 reads its own data" % (
                len(oob), len(info["cases"]))
            info["real"] = oob
        else:
            info["status"] = "match"
            info["kind"] = "float" if floaty or precision else "exact"
            notes = []
            if floaty:
                notes.append("%d float-only differences" % floaty)
            if ranges:
                notes.append("%d of them finite on one side only (x87 exponent range)" % ranges)
            if precision:
                notes.append("%d cases equal only with 24-bit x87 precision" % precision)
            if ptrish:
                notes.append("%d pointers to unmapped constants" % ptrish)
            if unmapped_total:
                notes.append("%d x86 bytes in unmapped globals" % unmapped_total)
            info["note"] = "; ".join(notes)

    # ---- report
    counts = {}
    for info in funcs.values():
        key = info["status"] if info["status"] != "match" else "match-" + info["kind"]
        if info["status"] == "mismatch":
            key = "mismatch-" + info.get("kind", "real")
        counts[key] = counts.get(key, 0) + 1
    skip_reasons = {}
    for info in funcs.values():
        if info["status"] == "skip":
            r = info["note"].split(":")[0]
            skip_reasons[r] = skip_reasons.get(r, 0) + 1

    if args.report:
        with open(args.report, "w") as rep:
            rep.write("#addr\tname\tmodule\tlevel\tstatus\tkind\tcases\tnote\n")
            for addr in candidates:
                i = funcs[addr]
                rep.write("%08x\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" % (
                    addr, i["name"], i["module"], i["level"], i["status"], i.get("kind", "-"),
                    i.get("tested_cases", 0), i["note"]))

    for addr in candidates:
        i = funcs[addr]
        if i["status"] not in ("mismatch", "oob", "unclear", "known", "abi-note"):
            continue
        print("%s 0x%08x %s (%s, %s): %s" % (i["status"].upper(), addr,
                                             i["name"], i["module"], i["level"], i["note"]))
        for v, problems in i.get("real", [])[:args.details]:
            print("    case %d args (%s) image %d" % (v["case"], ", ".join(v["desc"]), v["image"]))
            o = v["orig"]
            print("      x86 eax %08x st0 %r" % (o["eax"], o["st0"]))
            for p in problems[:6]:
                print("      " + p)
            if len(problems) > 6:
                print("      ... %d more" % (len(problems) - 6))

    elapsed = time.time() - started
    print("functions: %d selected; %s" % (len(candidates), ", ".join(
        "%s %d" % kv for kv in sorted(counts.items()))))
    if skip_reasons:
        print("skipped by reason: %s" % ", ".join("%s %d" % kv for kv in sorted(skip_reasons.items())))
    print("cases: %d run on both sides; port restarts after a fault or hang: %d" % (
        sum(len(funcs[a]["cases"]) for a in tested), restarts))
    dups = []
    if port:
        dups = sorted(set(d for d in port.duplicates if not any(n.startswith("str_") for n in d)))
        for a_name, b_name in dups:
            print("TWO-STORAGE %s and %s: one original address, two port locations" % (a_name, b_name))
    print("time: %.1f s (x86 %.1f s, port build and run %.1f s)" % (elapsed, t_orig, t_port))
    failed = False
    if any(i["status"] == "mismatch" for i in funcs.values()):
        failed = True
    if port and dups:
        failed = True
    if args.list or args.only:
        # a listed function must stay tested: a skip hides a regression
        untested = [a for a in candidates if funcs[a]["status"] not in ("match", "known")]
        for a in untested:
            print("FAIL listed function 0x%08x %s is %s: %s" % (a, funcs[a]["name"], funcs[a]["status"],
                                                            funcs[a]["note"]))
        failed |= bool(untested)
    print("RESULT: %s" % ("FAIL" if failed else "PASS"))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
