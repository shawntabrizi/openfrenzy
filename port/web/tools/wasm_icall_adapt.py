#!/usr/bin/env python3
"""Make the web build's indirect calls tolerate a callee of another type (docs/WEB.md).

    python3 -I port/web/tools/wasm_icall_adapt.py --binaryen BIN_DIR IN.wasm OUT.wasm [--opt -O2]
            [--map OUT.icall.txt]

Why. The decompiled game calls through function pointers whose declared type is not always
the callee's: a vtable slot declared int (*)(void *) holding a void function, a callback
called with one argument more than it takes, Ghidra's unprototyped `code *`. On the Xbox and
on the native AArch64 build that works: arguments are registers, extra ones are ignored,
missing ones hold whatever was there, and an unused return register costs nothing.
WebAssembly checks the type of every call_indirect and traps on a mismatch. Emscripten's
EMULATE_FUNCTION_POINTER_CASTS would change every table entry, which JSPI (the frame yield,
port/web/src/web_frame.c) does not allow.

What. After the link, every `call_indirect (type $T)` becomes a call of `$__of_icall_T`, a
small function with the same parameters plus the table index. It looks up the callee's real
type ($__of_fn_type: a br_table over the static function table) and, when it is $T (the
usual case), makes the same call_indirect. Otherwise it calls `$__of_icall_generic`, which
passes the arguments as the AArch64 procedure call standard would: integer arguments (i32,
i64) fill the callee's integer parameters in order, float arguments (f32, f64) fill its
float parameters in order, a missing one is 0, a surplus one is dropped; f32 and f64 are
converted to each other, i32 and i64 are wrapped or zero-extended. The callee's result goes
back the same way (0 if it returns nothing). The function table itself is not changed, so
JavaScript's calls into it are unaffected.

The first mismatched call of each table entry is reported to of_web_icall_mismatch (C,
port/web/src/web_icall.c) when the module has it. --map writes "index type name" for every
table entry, to read those reports.

The module goes through Binaryen's text format (wasm-dis, then wasm-opt to assemble and
optimise it again).
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile

NAME = r'\$(?:"(?:[^"\\]|\\.)*"|[^\s()]+)'
INT_SLOTS = 12
FLOAT_SLOTS = 8


def parse_signature(text):
    """params and results of '(param ...) (result ...)' text: (tuple, tuple)"""
    params = []
    results = []
    for kind, body in re.findall(r"\((param|result)((?:\s+(?:\$\S+\s+)?[a-z0-9]+)*)\s*\)", text):
        tokens = [t for t in body.split() if not t.startswith("$")]
        (params if kind == "param" else results).extend(tokens)
    return tuple(params), tuple(results)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binaryen", required=True, help="the directory of wasm-dis and wasm-opt")
    parser.add_argument("--opt", default="-O2")
    parser.add_argument("--map")
    parser.add_argument("input")
    parser.add_argument("output")
    args = parser.parse_args()
    dis = os.path.join(args.binaryen, "wasm-dis")
    opt = os.path.join(args.binaryen, "wasm-opt")

    with tempfile.TemporaryDirectory() as work:
        wat_in = os.path.join(work, "in.wat")
        wat_out = os.path.join(work, "out.wat")
        subprocess.run([dis, args.input, "-o", wat_in], check=True)
        with open(wat_in, encoding="utf-8", errors="surrogateescape") as f:
            lines = f.read().split("\n")

        # ---------- the types and the functions' signatures
        types = {}          # index -> (params, results)
        by_signature = {}   # (params, results) -> index
        signature_of = {}   # function name -> (params, results)
        elem = None         # [name, ...] for table indices 1..
        elem_base = None
        for line in lines:
            m = re.match(r"^ \(type \$(\d+) \(func(.*)\)\)$", line)
            if m:
                signature = parse_signature(m.group(2))
                types[int(m.group(1))] = signature
                by_signature.setdefault(signature, int(m.group(1)))
                continue
            m = re.match(r"^ \(import \"[^\"]*\" \"[^\"]*\" \(func (" + NAME + r")(.*)\)\)$", line)
            if m:
                signature_of[m.group(1)] = parse_signature(m.group(2))
                continue
            m = re.match(r"^ \(func (" + NAME + r")(.*)$", line)
            if m:
                signature_of[m.group(1)] = parse_signature(m.group(2))
                continue
            m = re.match(r"^ \(elem \$\S+ \(i32\.const (\d+)\)(.*)\)$", line)
            if m:
                if elem is not None:
                    sys.exit("wasm_icall_adapt: more than one element segment")
                elem_base = int(m.group(1))
                elem = re.findall(NAME, m.group(2))
        if elem is None:
            sys.exit("wasm_icall_adapt: no element segment")
        table_line = [line for line in lines if re.match(r"^ \(table \$0 (\d+) (\d+) funcref\)$", line)]
        if len(table_line) != 1:
            sys.exit("wasm_icall_adapt: the function table is not one fixed-size table $0")

        entry_type = {}
        for offset, name in enumerate(elem):
            signature = signature_of.get(name)
            if signature is None:
                sys.exit(f"wasm_icall_adapt: no signature for table entry {name}")
            if signature not in by_signature:
                sys.exit(f"wasm_icall_adapt: no type for the signature of {name}")
            entry_type[elem_base + offset] = by_signature[signature]
        table_size = elem_base + len(elem)

        # ---------- the call sites
        text = "\n".join(lines)
        call_types = sorted({int(t) for t in re.findall(r"\(call_indirect \$0 \(type \$(\d+)\)", text)})
        if "return_call_indirect" in text or "call_ref" in text:
            sys.exit("wasm_icall_adapt: tail calls or call_ref are not handled")

        def adaptable(signature):
            params, results = signature
            return (all(p in ("i32", "i64", "f32", "f64") for p in params) and len(results) <= 1 and
                    all(r in ("i32", "i64", "f32", "f64") for r in results) and
                    sum(p in ("i32", "i64") for p in params) <= INT_SLOTS and
                    sum(p in ("f32", "f64") for p in params) <= FLOAT_SLOTS)

        adapted = [t for t in call_types if adaptable(types[t])]
        callee_types = sorted({t for t in entry_type.values() if adaptable(types[t])})
        text = re.sub(r"\(call_indirect \$0 \(type \$(\d+)\)",
                      lambda m: f"(call $__of_icall_{m.group(1)}" if int(m.group(1)) in adapted else m.group(0),
                      text)
        has_report = "(func $of_web_icall_mismatch " in text

        # ---------- direct calls through a cast that LLVM could not fix (FixFunctionBitcasts makes
        # "<callee>_bitcast_invalid" stubs that trap, for example an int passed where the callee
        # takes a char): the same rule as the generic indirect call, with both types known here
        def convert(value, source, target):
            if source == target:
                return value
            if source in ("i32", "i64") and target in ("i32", "i64"):
                return f"(i32.wrap_i64 {value})" if target == "i32" else f"(i64.extend_i32_u {value})"
            if source in ("f32", "f64") and target in ("f32", "f64"):
                return f"(f32.demote_f64 {value})" if target == "f32" else f"(f64.promote_f32 {value})"
            return {"i32": "(i32.const 0)", "i64": "(i64.const 0)", "f32": "(f32.const 0)", "f64": "(f64.const 0)"}[target]

        def adapt_arguments(source_params, target_params, get):
            ints = [(get(i), p) for i, p in enumerate(source_params) if p in ("i32", "i64")]
            floats = [(get(i), p) for i, p in enumerate(source_params) if p in ("f32", "f64")]
            operands = []
            for p in target_params:
                pool = ints if p in ("i32", "i64") else floats
                if pool:
                    value, kind = pool.pop(0)
                    operands.append(convert(value, kind, p))
                else:
                    operands.append(convert(None, "none", p))
            return operands

        bitcast_fixed = []
        def fix_bitcast(match):
            stub, target, header = match.group(1), match.group(2), match.group(3)
            if target not in signature_of or ("$" + stub) not in signature_of:
                return match.group(0)
            source_params, source_results = signature_of["$" + stub]
            target_params, target_results = signature_of[target]
            if not adaptable((source_params, source_results)) or not adaptable((target_params, target_results)):
                return match.group(0)
            call = f"(call {target} {' '.join(adapt_arguments(source_params, target_params, lambda i: f'(local.get {i})'))})"
            if source_results and target_results:
                body = convert(call, target_results[0], source_results[0])
            elif source_results:
                body = f"(drop {call}) {convert(None, 'none', source_results[0])}" if target_results else \
                       f"{call} {convert(None, 'none', source_results[0])}"
            else:
                body = f"(drop {call})" if target_results else call
            bitcast_fixed.append(stub)
            return f" (func ${stub}{header}\n  {body}\n )"
        text = re.sub(r" \(func \$(\.L(\S+?)_bitcast_invalid(?:_\d+)?)((?: \(param [^\n]*?)?(?: \(result [^\n]*?)?)\n  \(unreachable\)\n \)",
                      lambda m: fix_bitcast(type("M", (), {"group": lambda self, i: {0: m.group(0), 1: m.group(1), 2: "$" + m.group(2), 3: m.group(3)}[i]})()),
                      text)

        left = re.findall(r" \(func \$(\.L\S+?_bitcast_invalid(?:_\d+)?)[^\n]*\n  \(unreachable\)\n \)", text)
        if left:
            sys.exit("wasm_icall_adapt: direct calls through a cast that trap, and whose callee is not in the "
                     "module to adapt them to (give the cast the callee's parameter types): " + ", ".join(left))

        # ---------- the new functions
        out = []
        slot_params = " ".join(["i64"] * INT_SLOTS + ["f64"] * FLOAT_SLOTS)

        # the callee's type, by table index (-1: not a static entry)
        out.append(" (func $__of_fn_type (param $index i32) (result i32)")
        out.append("  (if (i32.ge_u (local.get $index) (i32.const %d)) (then (return (i32.const -1))))" % table_size)
        labels = {}
        cases = sorted(set(entry_type.values()))
        for t in cases:
            labels[t] = f"$t{t}"
        targets = [labels[entry_type[i]] if i in entry_type else "$none" for i in range(table_size)]
        nested = "(block $none " + "".join(f"(block {labels[t]} " for t in cases)
        nested += "(br_table " + " ".join(targets) + " $none (local.get $index))"
        out.append("  " + nested)
        for t in reversed(cases):
            out.append(f"  ) (return (i32.const {t}))")
        out.append("  ) (i32.const -1))")

        # the float result of a generic call (a global is per thread: each worker has its own)
        out.append(" (global $__of_icall_float (mut f64) (f64.const 0))")

        # generic call: integer slots, float slots -> the callee's parameters
        out.append(f" (func $__of_icall_generic (param $index i32) (param $caller i32) (param $callee i32) "
                   f"(param {slot_params}) (result i64)")
        if has_report:
            out.append("  (call $of_web_icall_mismatch (local.get $index) (local.get $caller) (local.get $callee))")
        generic_labels = [f"$g{t}" for t in callee_types]
        nested = "(block $unknown " + "".join(f"(block {label} " for label in generic_labels)
        table_targets = []
        for t in range(max(types) + 1):
            table_targets.append(f"$g{t}" if t in callee_types else "$unknown")
        nested += "(br_table " + " ".join(table_targets) + " $unknown (local.get $callee))"
        out.append("  " + nested)
        for t in reversed(callee_types):
            params, results = types[t]
            operands = []
            int_slot = 0
            float_slot = 0
            for p in params:
                if p in ("i32", "i64"):
                    slot = f"(local.get {3 + int_slot})"
                    int_slot += 1
                    operands.append(slot if p == "i64" else f"(i32.wrap_i64 {slot})")
                else:
                    slot = f"(local.get {3 + INT_SLOTS + float_slot})"
                    float_slot += 1
                    operands.append(slot if p == "f64" else f"(f32.demote_f64 {slot})")
            call = f"(call_indirect $0 (type ${t}) {' '.join(operands)} (local.get $index))"
            result = results[0] if results else None
            if result is None:
                body = f"{call} (return (i64.const 0))"
            elif result == "i32":
                body = f"(return (i64.extend_i32_u {call}))"
            elif result == "i64":
                body = f"(return {call})"
            elif result == "f32":
                body = f"(global.set $__of_icall_float (f64.promote_f32 {call})) (return (i64.const 0))"
            else:
                body = f"(global.set $__of_icall_float {call}) (return (i64.const 0))"
            out.append(f"  ) {body}")
        out.append("  ) (unreachable))")

        # one entry per call-site type
        for t in adapted:
            params, results = types[t]
            result = results[0] if results else None
            names = " ".join(f"(param $p{i} {p})" for i, p in enumerate(params))
            result_text = f"(result {result})" if result else ""
            out.append(f" (func $__of_icall_{t} {names} (param $index i32) {result_text}")
            out.append("  (local $callee i32)")
            out.append("  (local.set $callee (call $__of_fn_type (local.get $index)))")
            direct = f"(call_indirect $0 (type ${t}) {' '.join(f'(local.get $p{i})' for i in range(len(params)))} (local.get $index))"
            out.append(f"  (if (i32.or (i32.eq (local.get $callee) (i32.const {t})) (i32.lt_s (local.get $callee) (i32.const 0)))")
            out.append(f"   (then (return {direct})))")
            ints = []
            floats = []
            for i, p in enumerate(params):
                if p == "i32":
                    ints.append(f"(i64.extend_i32_u (local.get $p{i}))")
                elif p == "i64":
                    ints.append(f"(local.get $p{i})")
                elif p == "f32":
                    floats.append(f"(f64.promote_f32 (local.get $p{i}))")
                else:
                    floats.append(f"(local.get $p{i})")
            ints += ["(i64.const 0)"] * (INT_SLOTS - len(ints))
            floats += ["(f64.const 0)"] * (FLOAT_SLOTS - len(floats))
            generic = (f"(call $__of_icall_generic (local.get $index) (i32.const {t}) (local.get $callee) "
                       f"{' '.join(ints)} {' '.join(floats)})")
            if result is None:
                out.append(f"  (drop {generic}))")
            elif result == "i32":
                out.append(f"  (i32.wrap_i64 {generic}))")
            elif result == "i64":
                out.append(f"  {generic})")
            elif result == "f32":
                out.append(f"  (drop {generic}) (f32.demote_f64 (global.get $__of_icall_float)))")
            else:
                out.append(f"  (drop {generic}) (global.get $__of_icall_float))")

        end = text.rstrip().rfind(")")
        text = text[:end] + "\n".join(out) + "\n)\n"
        with open(wat_out, "w", encoding="utf-8", errors="surrogateescape") as f:
            f.write(text)
        # the input's features (its target_features section), so that the output needs no other
        # (an optimised link has no target_features section: then the features Emscripten builds
        # this module with, the ones an unoptimised link of it declares)
        printed = subprocess.run([opt, args.input, "--print-features"], capture_output=True, text=True)
        features = printed.stdout.split() if printed.returncode == 0 else [
            "--enable-threads", "--enable-mutable-globals", "--enable-nontrapping-float-to-int",
            "--enable-simd", "--enable-bulk-memory", "--enable-sign-ext", "--enable-reference-types",
            "--enable-multivalue", "--enable-bulk-memory-opt", "--enable-call-indirect-overlong"]
        command = [opt, wat_out, "-o", args.output, "-g", "--emit-target-features", *[f for f in features if f.startswith("--enable-")]]
        if args.opt:
            command.append(args.opt)
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0:
            sys.stderr.write(result.stderr[-4000:])
            sys.exit("wasm_icall_adapt: wasm-opt failed")

        if args.map:
            with open(args.map, "w") as f:
                for index in range(elem_base, table_size):
                    f.write(f"{index} {entry_type[index]} {elem[index - elem_base]}\n")
        sites = len(re.findall(r"call \$__of_icall_\d+\b", text))
        print(f"wasm_icall_adapt: {sites} indirect call sites through "
              f"{len(adapted)} call types; {len(callee_types)} callee types in a table of {table_size}"
              + f"; {len(bitcast_fixed)} trapping direct-call stubs given bodies"
              + ("" if has_report else "; no of_web_icall_mismatch to report to"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
