# Differential test (tests/diff)

This test runs one function two times with the same inputs: the original x86 code from
`orig/default.xbe` in the Unicorn emulator, and the port's compiled C in the host arena. Then it
compares the results. It finds behaviour differences that do not crash.

## Use

One-time setup (makes `tools/diff-venv`, git-ignored; never the system Python):

```sh
sh tests/diff/setup_venv.sh
```

Build the project first (`cmake --build build-<id>`). Then:

```sh
ctest --test-dir build-<id> -L diff                     # the regression list, about 2 s
python3 -I tests/diff/run_diff.py --build build-<id>     # leaf and level-1 functions, about 30 s
python3 -I tests/diff/run_diff.py --build build-<id> --scope all --report out.tsv
python3 -I tests/diff/run_diff.py --build build-<id> --only 0015d260,00188870
```

Options: `--scope leaf|l1|all`, `--module a,b`, `--list FILE`, `--only ADDR,...`,
`--cases N` (default 12), `--report FILE` (a TSV line for each function), `--details N`.

Exit codes: 0 pass, 1 a real difference (or a listed function that is not tested), 2 a
harness error, 77 skip. The test skips when `orig/default.xbe` or `tools/diff-venv` is
missing. It needs no other game data.

## How it works

### x86 side (`orig_x86.py`)

- The XBE sections are loaded at their virtual addresses. Each case starts from the same
  state: the pristine data image (the harness puts back every byte that the last case
  wrote), a fresh copy of the scratch image, 64 KB of zero stack, and a saved CPU state. The x87 control word is
  0x027F (53-bit precision, the MSVC CRT value). FS points to a small TIB.
- Arguments: the port's prototype (from the generated `of_decls.h`) gives the types. The
  harness finds the x86 locations from the code: a `ret N` gives the number of stack words
  that the function pops. With `ret N`, the first one or two words go to ECX and EDX when
  the port has more argument words than N/4 (thiscall, fastcall). With a plain `ret`, the
  harness puts the first two words in ECX and EDX and also all words on the stack (this
  works for cdecl and for a thiscall with only `this`). After the call the harness checks
  how many bytes the function popped.
- A case stops (and is not compared) when the code enters an XDK section, the XAPI or Bink
  region, a C library function with outside state (rand, malloc, files), a kernel import,
  or `bkTimerRead` (rdtsc). It also stops on a fault, or after 400 000 instructions.
- Recorded: EAX, EDX, ST0, the x87 stack depth, the bytes popped, and every byte that the
  function changed in the scratch window and in the data sections (stack writes are not
  compared). The write hook also records which bytes an x87 or SSE store wrote.
- Every case runs a second time with other values in EAX, EBX, ESI, EDI and EBP. If the
  result changes, the function reads a register that is not an argument (a custom calling
  convention) and the harness skips it.

### Port side (`diff_driver.c`)

- `run_diff.py` writes a C file with one call for each tested function, through its real
  symbol, with the argument words cast to the parameter types. It is compiled with the game
  build's command and linked with the game build's objects, without `of_boot.c` (whose
  `main` starts the game). Static constructors do not run on either side.
- The driver maps the scratch window at the same guest address as Unicorn (0x87000000,
  inside the Xbox window, where the guest can map a fixed address). So pointers in the
  scratch data are the same values on both sides.
- Before each case the driver copies the scratch image and clears 64 KB of its stack (as
  the x86 side does, so that a local read before it is written is 0 on both sides). After the call it writes the
  return value and every changed byte of the scratch window and of the mapped globals, and
  it puts the globals back. The output is flushed after each case. When the port faults or
  hangs (no output for 5 s), the harness records it for that case and starts the driver
  again from the next case. Every child process has a hard time limit.

### Globals

The port's globals are at other addresses. The harness maps them through the guest ELF's
symbols: a name that ends in the original address (`DAT_00285630`, `vtable_002537d8`,
`str_00262b20`, and the `.set` aliases inside data blocks). The assembler step drops symbol
sizes, so a size is the gap to the next symbol, cut at the next mapped original address.
Data blocks without such names are in `blobs.tsv`, with the range from their header. Inside
a declared block the block wins over a named symbol of another module (that symbol keeps
only its first word).

Two symbols that name one original address at two port locations are two storages: code in
one module does not see writes from the other. The harness prints `TWO-STORAGE` and fails.

### Inputs

- Scratch: four images of 256 KB from fixed seeds. Words are pointers into the scratch
  window (25 %), small integers, floats in [-100, 100], zeros, and a few raw random words.
  The last 32 KB are ASCII text with NUL terminators.
- Pointer argument j points into the scratch window at 0x1000 + j * 0x8000 (plus a small
  offset). `char *` arguments point into the text. Case 2 passes NULL for the first pointer.
- Integers: 2 (case 0), 0 (case 1), then values from a pool of edge values (0, 1, -1,
  0x7fffffff, -0x80000000, ...) or random 32-bit values. Floats: 1.5, 0.0, then edge values
  (0, -0, 1, -1, 0.5, 1e6, ...) or random values in [-1000, 1000]. No NaN or infinity input.
- The seed of each function is its address, so a run is the same each time.

### Compare and classes

- Return values: integers masked to their size; a port result of 0 or 1 equals an x86 bool
  in AL. Floats: ST0, rounded to float for a float return.
- Memory: byte by byte, in 32-bit words. A port pointer to a global or a function is mapped
  back to its original address. A port pointer into data without a known original address
  equals an x86 pointer into its own data ("pointer to unmapped constant").
- Float tolerance applies only to words that x86 wrote with an x87 or SSE store, or that hold
  a value which such a store made in the same run (a copy with integer moves): relative
  1e-4 or absolute 1e-5; any NaN equals any NaN; two denormals are equal; a value that is
  not finite on one side is an x87 range difference. Other words: two normal floats of
  magnitude 1e-10 to 1e10 at most 4 ulps apart are equal (a float constant that x86 stores
  with a mov).
- The precision check: when a case differs, x86 runs again with the x87 precision at 24
  bits (control word 0x007F). If the results then agree, the difference is precision only.

Function classes in the report:

| Class | Meaning |
|---|---|
| `match-exact` | every case agrees |
| `match-float` | agrees within the float rules (or only with 24-bit precision) |
| `mismatch-real` | a case differs: a port bug, or a new harness limit to check by hand |
| `unclear` | differs only in cases with an integer argument outside [0, 15] (an index past a table) |
| `oob` | the port faults outside its image (or in the XBE range) where x86 reads its own data |
| `abi-note` | x86 returns a value in ST0 and the port returns none (or the reverse) |
| `known` | listed in `known.txt` with a reason (a port choice, or checked by hand) |
| `skip` | not tested: unsupported type, calls into library or kernel code, no x86 case completes, unclear ABI |

## Files

| File | What |
|---|---|
| `run_diff.py` | the harness: selection, inputs, driver build, port run, compare, report |
| `orig_x86.py` | the x86 side (Unicorn) |
| `diff_driver.c` | the port side's `main` (included by the generated C file) |
| `ucshim.c` | makes the Unicorn wheel run on Apple Silicon macOS 26 (see the file) |
| `regression.txt` | the functions that `ctest -L diff` runs |
| `known.txt` | known differences with their reasons |
| `blobs.tsv` | data blocks without address names, with their original ranges |
| `setup_venv.sh` | makes `tools/diff-venv` |

## Add a function

- A fixed bug: add its address to `regression.txt` with a comment.
- A difference that is not a bug: add it to `known.txt` with the reason that you checked.

## Limits

- Inputs are random. A function that needs a valid object graph (vtable calls, lists) often
  faults on x86 with every case and is skipped. Functions that call XDK, kernel, heap or
  file code are not tested.
- Tiny floats in the scratch (pointer values read as floats) cause x87 range differences in
  vector maths. They are in `known.txt` after a check. A real bug in such a function can hide
  among them.
- Port globals without an address name and outside `blobs.tsv` are not compared and not
  restored between cases.
- A port symbol's size is a guess (the gap to the next symbol).
- Port code that reads memory before x86 does (an eager load) can fault where x86 does not.
