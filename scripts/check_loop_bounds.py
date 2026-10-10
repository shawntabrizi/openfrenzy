#!/usr/bin/env python3
"""Find counted loops in src/ whose trip count differs from the same loop in the XBE.

Integration round 5 found CDemolition_Player_CreateActor (0x000f9370) looping 47 times,
while the XBE loop `iVar1 = 0x264; do { ... iVar1 = iVar1 + 4; } while (iVar1 < 0x310);`
runs 43 times. The extra 4 iterations read past a name table and wrote past the
actor's animation slots. This script finds more of these.

For each function, it finds the loops with a constant start, a constant step and a
constant limit, and computes their trip count:
  - in export/funcs/<addr>.c (Ghidra): do/while, while and for loops; stride loops
    (iVar += 4 while iVar < 0x310), pointer loops (puVar1 = &DAT_x; puVar1 + 1 while
    (int)puVar1 < 0x...), and count-down loops (iVar3 = 6; ... iVar3 + -1 while != 0);
  - in src/ (the function after a `/* 0x<addr>` comment): the same shapes, plus
    `for (i = 0; i < 47; i++)`, `0x264 + i * 4 < 800` and #define limits.
A start or limit that uses the same plain identifier on both sides (param_1 + 4 ..
param_1 + 0x24) is accepted as a symbolic base; it cancels out.

It reports:
  MISMATCH  a src loop whose trip count is not a trip count of any XBE loop of the
            function (the XBE has at least one countable loop).
  XBE-ONLY  (with --all) an XBE trip count that no src loop of the function has.
Not every hit is a bug: the compiler unrolls short loops, src may use memset or a
helper, and a loop with `break` runs at most its trip count. Check each hit against
the Ghidra output (and the disassembly, scripts/DecompileAt.java) before you change it.

Usage: python3 -I scripts/check_loop_bounds.py [--all] [--addr 000f9370] [-v]
       (run from the repository root)
"""
import argparse
import collections
import glob
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
MAX_TRIPS = 1 << 20

TYPE_SIZES = {}
for _n, _s in (
    ("char uchar byte undefined undefined1 bool _Bool int8_t uint8_t u8 s8 BYTE CHAR", 1),
    ("short ushort undefined2 word wchar_t int16_t uint16_t u16 s16 WORD WCHAR", 2),
    ("int uint undefined4 dword long ulong float int32_t uint32_t u32 s32 DWORD"
     " LONG ULONG UINT INT BOOL FLOAT size_t uintptr_t intptr_t uint3 code", 4),
    ("double longlong ulonglong undefined8 qword int64_t uint64_t __int64 u64 s64", 8),
    ("float10", 10),
):
    for _t in _n.split():
        TYPE_SIZES[_t] = _s

KEYWORDS = {"if", "for", "while", "do", "switch", "return", "sizeof", "else", "case",
            "goto", "break", "continue", "default"}

ADDR_SYM = re.compile(r"&\s*(?:DAT|PTR|UNK|LAB|FUN|s|u)_\w*?([0-9a-fA-F]{8})\b")
STR_SYM = re.compile(r"\b(?:s|u)_\w*?_([0-9a-fA-F]{8})\b")
CAST = re.compile(r"\(\s*(?:const\s+|volatile\s+)*(?:unsigned\s+|signed\s+|struct\s+|enum\s+)?"
                  r"(\w+)(?:\s+(?:int|long|char|short))?\s*(\*+)?\s*\)")


def clean(text):
    """Blank comments and string/char literal contents, keeping every offset."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                j += 2 if text[j] == "\\" else 1
            for k in range(i + 1, min(j, n)):
                if out[k] != "\n":
                    out[k] = " "
            i = j + 1
        elif c == "'":
            j = i + 1
            while j < n and text[j] != "'":
                j += 2 if text[j] == "\\" else 1
            lit = text[i + 1:j]
            val = {"\\0": 0, "\\n": 10, "\\t": 9, "\\r": 13, "\\\\": 92, "\\'": 39}.get(
                lit, ord(lit[0]) if len(lit) == 1 else None)
            if val is not None and j - i + 1 >= 3:
                rep = str(val).ljust(j - i + 1)
                if len(rep) == j - i + 1:
                    out[i:j + 1] = list(rep)
            i = j + 1
        else:
            i += 1
    return "".join(out)


def match_close(s, i, open_c, close_c):
    """s[i] == open_c; return the index of the matching close_c, or -1."""
    depth = 0
    for j in range(i, len(s)):
        if s[j] == open_c:
            depth += 1
        elif s[j] == close_c:
            depth -= 1
            if depth == 0:
                return j
    return -1


def skip_ws(s, i):
    while i < len(s) and s[i].isspace():
        i += 1
    return i


def stmt_end(s, i):
    """End (exclusive) of the statement that starts at i: a block or up to ';'."""
    i = skip_ws(s, i)
    if i < len(s) and s[i] == "{":
        j = match_close(s, i, "{", "}")
        return -1 if j < 0 else j + 1
    m = re.match(r"(if|for|while|switch)\b", s[i:])
    if m:
        p = skip_ws(s, i + len(m.group(1)))
        if p >= len(s) or s[p] != "(":
            return -1
        q = match_close(s, p, "(", ")")
        if q < 0:
            return -1
        e = stmt_end(s, q + 1)
        if m.group(1) == "if" and e > 0:
            k = skip_ws(s, e)
            if s.startswith("else", k) and not (s[k + 4:k + 5].isalnum() or s[k + 4:k + 5] == "_"):
                return stmt_end(s, k + 4)
        return e
    if s.startswith("do", i) and not (s[i + 2:i + 3].isalnum() or s[i + 2:i + 3] == "_"):
        e = stmt_end(s, i + 2)
        if e < 0:
            return -1
        j = s.find(";", e)
        return -1 if j < 0 else j + 1
    depth = 0
    for j in range(i, len(s)):
        if s[j] in "([{":
            depth += 1
        elif s[j] in ")]}":
            depth -= 1
        elif s[j] == ";" and depth == 0:
            return j + 1
    return -1


def split_top(s, sep):
    parts, depth, cur = [], 0, []
    for c in s:
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        if c == sep and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(c)
    parts.append("".join(cur))
    return parts


def find_loops(s, base=0):
    """Yield dicts {kind, pos, init, cond, step, body, body_start} for loops in cleaned text s."""
    consumed = set()
    for m in re.finditer(r"\b(for|while|do)\b", s):
        kw, p = m.group(1), m.start()
        if p in consumed:
            continue
        q = skip_ws(s, m.end())
        if kw == "do":
            e = stmt_end(s, q)
            if e < 0:
                continue
            w = skip_ws(s, e)
            if not s.startswith("while", w):
                continue
            consumed.add(w)
            pp = skip_ws(s, w + 5)
            if pp >= len(s) or s[pp] != "(":
                continue
            pe = match_close(s, pp, "(", ")")
            if pe < 0:
                continue
            yield dict(kind="do", pos=p, init=None, cond=s[pp + 1:pe], step=None,
                       body=s[q:e], body_start=q, end=pe + 1)
            continue
        if q >= len(s) or s[q] != "(":
            continue
        pe = match_close(s, q, "(", ")")
        if pe < 0:
            continue
        hdr = s[q + 1:pe]
        e = stmt_end(s, pe + 1)
        if e < 0:
            continue
        b0 = skip_ws(s, pe + 1)
        if kw == "for":
            parts = split_top(hdr, ";")
            if len(parts) != 3:
                continue
            yield dict(kind="for", pos=p, init=parts[0], cond=parts[1], step=parts[2],
                       body=s[b0:e], body_start=b0, end=e)
        else:
            if s[b0:b0 + 1] == ";":
                continue  # tail of a do/while we did not parse
            yield dict(kind="while", pos=p, init=None, cond=hdr, step=None,
                       body=s[b0:e], body_start=b0, end=e)


def assign_re(v):
    """Assignments to plain variable v (not through *v, v[...], x.v, x->v)."""
    ev = re.escape(v)
    return re.compile(
        r"(?:(?<=[;{}(,])|^)\s*(?:(\+\+|--)\s*" + ev + r"\b|" + ev +
        r"\s*(\+\+|--|\+=|-=|\*=|/=|<<=|>>=|&=|\|=|\^=|=(?!=)))", re.M)


def assignments(text, v):
    """[(op, rhs_text, pos)] for statements that assign v."""
    out = []
    for m in assign_re(v).finditer(text):
        op = m.group(1) or m.group(2)
        rhs = ""
        if op not in ("++", "--"):
            rest = text[m.end():]
            # take rhs up to the end of the statement (balanced)
            depth, k = 0, 0
            while k < len(rest):
                c = rest[k]
                if c in "([":
                    depth += 1
                elif c in ")]":
                    if depth == 0:
                        break
                    depth -= 1
                elif c in ";," and depth == 0:
                    break
                k += 1
            rhs = rest[:k]
        out.append((op, rhs.strip(), m.start()))
    return out


def var_decl_size(func_text, v):
    """Pointee size if v is declared as a pointer, 1 if an integer, None if unknown."""
    m = re.search(r"(?:^|[;{(,])\s*((?:const\s+|unsigned\s+|signed\s+|struct\s+)*\w+)\s*(\*+)\s*"
                  + re.escape(v) + r"\s*[;=,\[)]", func_text, re.M)
    if m:
        base = m.group(1).split()[-1]
        if len(m.group(2)) > 1:
            return 4
        return TYPE_SIZES.get(base)
    return 1


class Expr:
    """A C expression over one variable, translated to Python."""

    def __init__(self, text, defines):
        self.ok, self.idents = False, set()
        t = text
        t = ADDR_SYM.sub(lambda m: str(int(m.group(1), 16)), t)
        t = STR_SYM.sub(lambda m: str(int(m.group(1), 16)), t)
        t = re.sub(r"\b(0[xX][0-9a-fA-F]+|\d+)[uUlL]+\b", r"\1", t)
        for _ in range(3):
            t2 = CAST.sub(lambda m: "" if (m.group(2) or m.group(1) in TYPE_SIZES
                                         or m.group(1) in ("unsigned", "signed", "void"))
                          else m.group(0), t)
            if t2 == t:
                break
            t = t2
        if re.search(r"->|\[|\.(?![0-9])|\?|\bsizeof\b|[^=!<>]=[^=]|\"", t):
            return
        if re.search(r"\b\w+\s*\(", t):
            m = re.search(r"\b(\w+)\s*\(", t)
            if m.group(1) not in ("and", "or", "not"):
                return
        for _ in range(4):
            t2 = re.sub(r"\b([A-Za-z_]\w*)\b",
                        lambda m: "(%s)" % defines[m.group(1)] if m.group(1) in defines else m.group(0), t)
            if t2 == t:
                break
            t = t2
        t = re.sub(r"\b0[xX]([0-9a-fA-F]+)\b", lambda m: str(int(m.group(1), 16)), t)
        if re.search(r"\b0\d+\b", t):
            t = re.sub(r"\b0(\d+)\b", lambda m: str(int(m.group(1), 8)), t)
        t = t.replace("&&", " and ").replace("||", " or ")
        t = re.sub(r"!(?!=)", " not ", t)
        t = re.sub(r"(?<![/])/(?![/])", "//", t)
        if re.search(r"\b\d+\.\d*|\b\d+[eE]", t) or "*/" in t:
            return
        self.idents = set(re.findall(r"\b[A-Za-z_]\w*\b", t)) - {"and", "or", "not"}
        if self.idents & KEYWORDS:
            return
        self.text = t.strip()
        try:
            self.code = compile(self.text or "0", "<loop>", "eval")
        except SyntaxError:
            return
        self.ok = True

    def eval(self, env):
        return eval(self.code, {"__builtins__": {}}, env)


def parse_update(op, rhs, v, defines):
    """Constant delta for v from (op, rhs), or None."""
    if op == "++":
        return 1
    if op == "--":
        return -1
    if op in ("+=", "-="):
        e = Expr(rhs, defines)
        if not e.ok or e.idents:
            return None
        d = e.eval({})
        return d if op == "+=" else -d
    if op == "=":
        m = re.fullmatch(r"\s*(?:\([^()]*\)\s*)?" + re.escape(v) + r"\s*([+-])\s*(.+)", rhs, re.S)
        if not m:
            m2 = re.fullmatch(r"\s*(.+?)\s*\+\s*(?:\([^()]*\)\s*)?" + re.escape(v) + r"\s*", rhs, re.S)
            if not m2:
                return None
            e = Expr(m2.group(1), defines)
            return e.eval({}) if e.ok and not e.idents else None
        e = Expr(m.group(2), defines)
        if not e.ok or e.idents:
            return None
        d = e.eval({})
        return d if m.group(1) == "+" else -d
    return None


SYM_BASE = 0x40000000


def analyze(loop, func_text, func_body_off, defines):
    """Return (trip_count, var, desc) or None for one loop."""
    cond = loop["cond"].strip()
    if cond in ("true", "1", "") and loop["kind"] != "for":
        # do { ... v = v + 4; if (0x283f47 < (int)v) return; } while (true): the last
        # top-level `if` that leaves the loop is the bound (Ghidra writes these often)
        b = loop["body"]
        if not b.startswith("{"):
            return None
        exits = []
        for m in re.finditer(r"\bif\s*\(", b):
            p = m.end() - 1
            q = match_close(b, p, "(", ")")
            if q < 0:
                continue
            e = stmt_end(b, q + 1)
            if e < 0 or b[:m.start()].count("{") - b[:m.start()].count("}") != 1:
                continue
            if re.search(r"\b(return|break)\b", b[q + 1:e]) and not re.search(r"\b(for|while|do)\b", b[q + 1:e]):
                exits.append((m.start(), b[p + 1:q]))
        if not exits:
            return None
        loop = dict(loop, kind="do", cond="!(%s)" % exits[-1][1])
        cond = loop["cond"]
    if not cond:
        return None
    pre, post = 0, 0
    parts = split_top(cond, ",")
    test = parts[-1].strip()
    var = None
    # updates inside the condition: (v = v + -1, v != 0), --v, v--
    cond_updates = parts[:-1]
    m_pre = re.match(r"\s*(\+\+|--)\s*([A-Za-z_]\w*)\b", test)
    m_post = re.search(r"\b([A-Za-z_]\w*)\s*(\+\+|--)(?!\s*[A-Za-z_(])", test)
    if m_pre:
        var = m_pre.group(2)
        pre = 1 if m_pre.group(1) == "++" else -1
        test = test[:m_pre.start()] + " " + var + test[m_pre.end():]
    elif m_post:
        var = m_post.group(1)
        post = 1 if m_post.group(2) == "++" else -1
        test = test[:m_post.start()] + var + test[m_post.end():]
    e = Expr(test, defines)
    if not e.ok:
        return None
    if var is None:
        cands = [i for i in e.idents if i not in defines]
        # the loop variable is the identifier that the loop updates
        upd = []
        for c in cands:
            hits = assignments(loop["body"], c) + (assignments(loop["step"], c) if loop["step"] else [])
            hits += [h for u in cond_updates for h in assignments(u.strip(), c)]
            if hits:
                upd.append(c)
        if len(upd) != 1:
            return None
        var = upd[0]
    for u in cond_updates:
        a = assignments(u.strip(), var)
        if len(a) != 1:
            return None
        d = parse_update(a[0][0], a[0][1], var, defines)
        if d is None:
            return None
        pre += d
    # step in body (and for-header step)
    body_asg = assignments(loop["body"], var)
    step = 0
    if loop["kind"] == "for":
        sa = assignments(loop["step"].strip(), var) if loop["step"].strip() else []
        if len(sa) != 1 or body_asg:
            if not (len(sa) == 0 and len(body_asg) == 1):
                return None
            sa = body_asg
    else:
        sa = body_asg
        if len(sa) > 1:
            return None
    if sa:
        d = parse_update(sa[0][0], sa[0][1], var, defines)
        if d is None:
            return None
        step = d
    if step == 0 and pre == 0 and post == 0:
        return None
    # start value
    init = None
    if loop["kind"] == "for" and loop["init"] and loop["init"].strip():
        for piece in split_top(loop["init"], ","):
            a = assignments(piece.strip(), var)
            if a and a[0][0] == "=":
                init = a[0][1]
        if init is None:
            init_txt = func_text[:func_body_off + loop["pos"]]
        else:
            init_txt = None
    else:
        init_txt = func_text[:func_body_off + loop["pos"]]
    if init is None:
        # last assignment to var before the loop; strip declarations like `int i = 0;`
        a = assignments(init_txt, var)
        a += [("=", mm.group(1), mm.start()) for mm in re.finditer(
            r"\b(?:int|unsigned|uint|long|short|char|undefined4)\s+" + re.escape(var) + r"\s*=\s*([^;,]+)",
            init_txt)]
        if not a:
            return None
        a.sort(key=lambda x: x[2])
        op, rhs, _ = a[-1]
        if op != "=":
            return None
        init = rhs
    ie = Expr(init, defines)
    if not ie.ok:
        return None
    sym = (ie.idents - {var}) | (e.idents - {var} - set(defines))
    if ie.idents & {var}:
        return None
    if sym:
        # only plain identifiers used both in the start and in the limit cancel out
        if not (sym <= ie.idents and sym <= e.idents):
            return None
        if any(re.match(r"(DAT|PTR|_DAT|UNK)_", s) for s in sym):
            return None
        # and that the function never assigns (a parameter or a fixed local)
        if any(assignments(func_text[func_body_off:], s) for s in sym):
            return None
    env = {s: SYM_BASE for s in sym}
    size = var_decl_size(func_text, var)
    if size != 1 and re.search(r"[A-Za-z_]\w*\s*\+\s*(?:0[xX][0-9a-fA-F]+|\d+)\s*$", test):
        # pointer compared with base + n elements: pointer arithmetic on both sides, so
        # the element size cancels; count in elements
        size = 1
    if size is None:
        return None
    try:
        v = ie.eval(dict(env))
        n = 0
        if loop["kind"] == "do":
            while True:
                n += 1
                v += step * size + pre * size
                env[var] = v
                t = e.eval(env)
                v += post * size
                if not t or n >= MAX_TRIPS:
                    break
        else:
            while True:
                v += pre * size
                env[var] = v
                t = e.eval(env)
                v += post * size
                if not t:
                    break
                n += 1
                if n >= MAX_TRIPS:
                    break
                v += step * size
    except Exception:
        return None
    if n >= MAX_TRIPS:
        return None
    desc = re.sub(r"\s+", " ", "%s %s=%s step %+d cond (%s)" % (
        loop["kind"], var, init.strip(), step + pre + post, test.strip()))
    return n, var, desc


def function_loops(text, body_off, defines, start=0):
    """Loops of the function whose signature starts at text[start] and body at text[body_off]."""
    first_line = text.count("\n", 0, start)
    text, body_off = text[start:], body_off - start
    s = text[body_off:]
    out = []
    for lp in find_loops(s):
        r = analyze(lp, text, body_off, defines)
        if r:
            line = first_line + text.count("\n", 0, body_off + lp["pos"]) + 1
            out.append((r[0], r[1], r[2], line, "break" in lp["body"]))
    return out


def collect_defines():
    vals = collections.defaultdict(set)
    per_file = {}
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.[ch]"), recursive=True) + \
            glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True):
        try:
            txt = open(path, encoding="utf-8", errors="replace").read()
        except OSError:
            continue
        d = {}
        for m in re.finditer(r"^\s*#\s*define\s+([A-Za-z_]\w*)\s+(\(?\s*-?(?:0[xX][0-9a-fA-F]+|\d+)[uUlL]*\s*\)?)\s*(?://.*|/\*.*)?$",
                             txt, re.M):
            d[m.group(1)] = re.sub(r"[uUlL]+(?=\s*\)?$)", "", m.group(2).strip())
            vals[m.group(1)].add(d[m.group(1)])
        per_file[path] = d
    glob_d = {k: next(iter(v)) for k, v in vals.items() if len(v) == 1}
    return glob_d, per_file


def src_functions():
    """Yield (addr, path, text, body_off, name, sig_start) for src functions after a `/* 0x<addr>` comment."""
    files = glob.glob(os.path.join(ROOT, "src", "**", "*.c"), recursive=True) + \
        glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)
    for path in sorted(files):
        raw = open(path, encoding="utf-8", errors="replace").read()
        cl = clean(raw)
        for m in re.finditer(r"/\*\s*0x([0-9a-fA-F]{8})", raw):
            if cl[m.start()] != " ":
                continue  # inside a string
            cend = raw.find("*/", m.end())
            if cend < 0:
                continue
            b = cl.find("{", cend + 2)
            if b < 0:
                continue
            sig = cl[cend + 2:b]
            if ";" in sig or "(" not in sig or "=" in sig or "/* 0x" in raw[cend + 2:b]:
                continue
            e = match_close(cl, b, "{", "}")
            if e < 0:
                continue
            nm = re.search(r"(\w+)\s*\([^()]*(?:\([^()]*\)[^()]*)*\)\s*$", sig)
            yield m.group(1).lower(), path, cl[:e + 1], b, nm.group(1) if nm else "", cend + 2


def ghidra_function(addr):
    path = os.path.join(ROOT, "export", "funcs", addr + ".c")
    if not os.path.exists(path):
        return None
    raw = open(path, encoding="utf-8", errors="replace").read()
    cl = clean(raw)
    m = re.search(r"^\{", cl, re.M)
    if not m:
        return None
    return cl, m.start()


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--all", action="store_true", help="also list XBE-only trip counts")
    ap.add_argument("--addr", help="only this function (8 hex digits)")
    ap.add_argument("-v", "--verbose", action="store_true", help="print every parsed loop")
    args = ap.parse_args()

    names = {}
    fj = os.path.join(ROOT, "export", "functions.jsonl")
    if os.path.exists(fj):
        import json
        for line in open(fj):
            o = json.loads(line)
            names[o["addr"].lower()] = o["name"]

    glob_d, per_file = collect_defines()
    by_addr = collections.defaultdict(list)
    for addr, path, text, off, fname, start in src_functions():
        if fname:
            names[addr] = fname
        if args.addr and addr != args.addr.lower():
            continue
        defs = dict(glob_d)
        defs.update(per_file.get(path, {}))
        loops = function_loops(text, off, defs, start)
        if loops:
            by_addr[addr].append((path, text.count("\n", 0, off) + 1, loops))

    n_funcs = n_src = n_x = 0
    mism, xonly, nox = [], [], 0
    for addr in sorted(by_addr):
        g = ghidra_function(addr)
        if not g:
            continue
        xl = function_loops(g[0], g[1], {})
        src_loops = [(p, l) for p, _, ls in by_addr[addr] for l in ls]
        n_funcs += 1
        n_src += len(src_loops)
        n_x += len(xl)
        xc = collections.Counter(l[0] for l in xl)
        sc = collections.Counter(l[0] for _, l in src_loops)
        name = names.get(addr, "")
        if args.verbose:
            print("0x%s %s" % (addr, name))
            for l in xl:
                print("   XBE %5d  %s (line %d)" % (l[0], l[2], l[3]))
            for p, l in src_loops:
                print("   src %5d  %s (%s:%d)" % (l[0], l[2], os.path.relpath(p, ROOT), l[3]))
        if not xl:
            nox += 1
            continue
        for p, l in src_loops:
            if l[0] not in xc:
                mism.append((addr, name, p, l, sorted(xc)))
        if args.all:
            for l in xl:
                if l[0] not in sc:
                    xonly.append((addr, name, l, sorted(sc)))

    print("check_loop_bounds: %d functions with countable src loops (%d src loops, %d XBE loops); "
          "%d have no countable XBE loop (not compared)" % (n_funcs, n_src, n_x, nox))
    print("\nMISMATCH (src trip count not among the XBE loops of the function): %d" % len(mism))
    for addr, name, p, l, xs in mism:
        print("  0x%s %-40s src %s:%d  %d trips  [%s]%s\n      XBE trip counts: %s" % (
            addr, name, os.path.relpath(p, ROOT), l[3], l[0], l[2],
            "  (has break)" if l[4] else "", ", ".join(map(str, xs))))
    if args.all:
        print("\nXBE-ONLY (XBE trip count no src loop has): %d" % len(xonly))
        for addr, name, l, ss in xonly:
            print("  0x%s %-40s XBE line %d  %d trips  [%s]\n      src trip counts: %s" % (
                addr, name, l[3], l[0], l[2], ", ".join(map(str, ss))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
