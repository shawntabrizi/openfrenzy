#!/usr/bin/env python3
"""Tests of the web build's two tools (docs/WEB.md "Tests"; ctest web_tools).

    python3 -I port/web/tests/test_web_tools.py --binaryen DIR --node NODE

wasm_icall_adapt.py: the decompiled game calls through function pointers whose type is not the
callee's, which works on the Xbox and natively on AArch64 (arguments are registers) and traps
in WebAssembly. The adapter must give the callee what an AArch64 call would: integer arguments
fill its integer parameters in order, float arguments its float parameters, missing ones are
0, extra ones are dropped, and a void callee called for a value gives the first integer
argument (eax = this for MSVC constructors). Each case below is a small module with one such
call; it must trap before the adapter and give the AArch64 result after it.

wasm_inline_asm.py: the data files' assembly names symbols as Mach-O does (one leading
underscore more than C); the converted copy must name the C symbols.
"""

import argparse
import os
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.join(HERE, "..", "tools")
sys.path.insert(0, TOOLS)

import wasm_inline_asm  # noqa: E402

ARGS = None

# (name, callee, caller type, call operands, expected result as JavaScript prints it)
CASES = [
    ("extra argument dropped",
     "(func $callee (param i32) (result i32) (local.get 0))",
     "(param i32 i32) (result i32)", "(i32.const 7) (i32.const 9)", "7"),
    ("missing argument is 0",
     "(func $callee (param i32 i32) (result i32) (i32.add (local.get 0) (local.get 1)))",
     "(param i32) (result i32)", "(i32.const 5)", "5"),
    ("float class kept apart from integers",
     "(func $callee (param i32 f32) (result f32) (f32.add (f32.convert_i32_s (local.get 0)) (local.get 1)))",
     "(param f32 i32) (result f32)", "(f32.const 0.5) (i32.const 2)", "2.5"),
    ("double passed to a float parameter is converted",
     "(func $callee (param f32) (result f32) (local.get 0))",
     "(param f64) (result f64)", "(f64.const 1.25)", "1.25"),
    ("void callee called for a value gives the first integer argument",
     "(func $callee (param i32))",
     "(param i32) (result i32)", "(i32.const 1234)", "1234"),
]


def module_text(callee, caller_type, operands):
    return f"""(module
 (type $caller (func {caller_type}))
 (table $0 2 2 funcref)
 (elem $0 (i32.const 1) $callee)
 {callee}
 (func $run (export "run") {run_result(caller_type)}
  (call_indirect $0 (type $caller) {operands} (i32.const 1)))
)
"""


def run_result(caller_type):
    return caller_type[caller_type.index("(result"):] if "(result" in caller_type else ""


def run_module(path):
    script = ("const fs = require('fs');"
              "WebAssembly.instantiate(fs.readFileSync(process.argv[1])).then(({instance}) => {"
              " try { console.log(String(instance.exports.run())); } catch (e) { console.log('trap: ' + e.message); } });")
    result = subprocess.run([ARGS.node, "-e", script, path], capture_output=True, text=True, timeout=60)
    return result.stdout.strip() or result.stderr.strip()


class CallAdapter(unittest.TestCase):
    def test_cases(self):
        binaryen = ARGS.binaryen
        for name, callee, caller_type, operands, expected in CASES:
            with self.subTest(name), tempfile.TemporaryDirectory() as work:
                wat = os.path.join(work, "case.wat")
                plain = os.path.join(work, "plain.wasm")
                adapted = os.path.join(work, "adapted.wasm")
                with open(wat, "w") as f:
                    f.write(module_text(callee, caller_type, operands))
                subprocess.run([os.path.join(binaryen, "wasm-as"), wat, "-o", plain, "--all-features"], check=True)
                self.assertTrue(run_module(plain).startswith("trap"), "the mismatched call must trap unadapted")
                subprocess.run([sys.executable, "-I", os.path.join(TOOLS, "wasm_icall_adapt.py"), "--binaryen", binaryen,
                                "--opt=", plain, adapted], check=True, capture_output=True)
                self.assertEqual(run_module(adapted), expected)


class InlineAssembly(unittest.TestCase):
    def test_mach_o_names_become_c_names(self):
        source = ('int x;\n__asm__(".globl _DAT_1\\n.set _DAT_1, _block + 0x4\\n"\n'
                  '\t".weak_definition __DAT_2\\n.set __DAT_2, _block + 0x8\\n");\n')
        converted = wasm_inline_asm.convert(source)
        self.assertIn('".globl DAT_1\\n.set DAT_1, block + 0x4\\n"', converted)
        self.assertIn('".weak _DAT_2\\n.set _DAT_2, block + 0x8\\n"', converted)

    def test_stringified_macro_and_code_outside_asm(self):
        source = ('#define ALIAS(n, o) __asm__(".globl _" #n "\\n\\t.set _" #n ", _g + " #o "\\n")\n'
                  'const char *s = "_keep";\n')
        converted = wasm_inline_asm.convert(source)
        self.assertIn('__asm__(".globl " #n "\\n\\t.set " #n ", g + " #o "\\n")', converted)
        self.assertIn('"_keep"', converted, "strings outside __asm__ stay as written")


def main():
    global ARGS
    parser = argparse.ArgumentParser()
    parser.add_argument("--binaryen", required=True)
    parser.add_argument("--node", required=True)
    ARGS, rest = parser.parse_known_args()
    unittest.main(argv=[sys.argv[0]] + rest, verbosity=2)


if __name__ == "__main__":
    main()
