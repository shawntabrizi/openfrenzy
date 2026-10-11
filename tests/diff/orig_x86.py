"""Original side of the differential test: run one x86 function of orig/default.xbe in Unicorn.

The XBE sections are loaded at their virtual addresses. Each case starts from the same state:
the saved CPU context (x87 control word 0x027F, empty x87 stack), the pristine data image (every
byte that the previous case wrote is put back) and a fresh copy of the scratch image. The result
is the return registers (EAX, EDX, ST0), the x87 stack depth, how many argument bytes the
function popped, the bytes that it changed (scratch memory and data sections; stack writes are
ignored) and the game functions that it entered.

A case stops with a status other than "ok" when the code enters an XDK library section, the
XAPI or Bink region, a C library function with outside state (rand, malloc, files), or a kernel
import (an unmapped fetch below the scratch window), and when it faults or runs too long.
"""

import math
import struct

import capstone
from unicorn import (UC_ARCH_X86, UC_HOOK_BLOCK, UC_HOOK_MEM_WRITE, UC_MODE_32, UC_PROT_ALL,
                     UC_PROT_READ, UC_PROT_WRITE, Uc, UcError)
from unicorn import x86_const as X

# fixed addresses (Unicorn side); the scratch window is the same guest address in the port
# (all above 0x10000, so that NULL plus a field offset faults as in the port, whose low 16 MB
# are unmapped)
RET_ADDR = 0x00d00000          # return address: a page holding HLT; emulation stops here
GDT_ADDR = 0x00d01000
TIB_ADDR = 0x00d02000          # FS segment: SEH chain head, stack bounds
STACK_LO = 0x00e00000
STACK_HI = 0x01000000
STACK_TOP = 0x00ff0000         # ESP at the call (above it: arguments and spare room)

# Library code that a tested function must not enter (XBE sections and regions of the
# source map in export/source_map.json; addresses copied here so that the harness needs
# only the XBE).
FORBIDDEN = [
    (0x0003055a, 0x00034730, "xapi"),
    (0x00057980, 0x000703a0, "bink"),
    (0x001f5340, 0x00253360, "xdk"),       # D3D, DSOUND, XGRPH, WMADEC*, XPP sections
    (0x0031d980, 0x00326f20, "xdk"),       # DOLBY, $$XTIMAGE
]
# C library functions with state outside the data image, or that call the kernel
# (docs/LIBRARY_CALLS.md names)
LIBC_DENY = {
    0x001e133e: "rand", 0x001e0d41: "malloc", 0x001dfe85: "fclose", 0x001dfded: "fread",
    0x001e02d7: "fread", 0x001e0038: "ftell", 0x001e0108: "fseek", 0x001e0873: "fputc",
    0x001e0c90: "fwprintf", 0x001e9739: "MoveFileA", 0x001e8833: "MmFreeContiguousMemory",
    0x001df672: "atexit", 0x001e0a69: "controlfp",
}
LIBC_REGIONS = [(0x001de951, 0x001ea360), (0x001ec550, 0x001ecc20)]
# game functions whose result depends on the machine, not on the data image
NONDETERMINISTIC = {0x0004c5f0: "bkTimerRead (rdtsc)"}

POISON = [
    {X.UC_X86_REG_EAX: 0xA1A1A1A1, X.UC_X86_REG_EBX: 0xB1B1B1B1, X.UC_X86_REG_ESI: 0xC1C1C1C1,
     X.UC_X86_REG_EDI: 0xD1D1D1D1, X.UC_X86_REG_EBP: 0xE1E1E1E1},
    {X.UC_X86_REG_EAX: 0x00000007, X.UC_X86_REG_EBX: 0x00000000, X.UC_X86_REG_ESI: 0x00000001,
     X.UC_X86_REG_EDI: 0x00000040, X.UC_X86_REG_EBP: 0x00000003},
]


def load_xbe(path):
    """Return (header bytes, [(name, va, vsize, raw bytes)]) of an XBE file."""
    data = open(path, "rb").read()
    if data[:4] != b"XBEH":
        raise ValueError("%s is not an XBE file" % path)
    base, = struct.unpack_from("<I", data, 0x104)
    header_size, = struct.unpack_from("<I", data, 0x108)
    nsec, secaddr = struct.unpack_from("<II", data, 0x11C)
    sections = []
    for i in range(nsec):
        _, va, vs, ra, rs, na = struct.unpack_from("<IIIIII", data, secaddr - base + i * 56)
        name = data[na - base:data.index(b"\0", na - base)].decode()
        sections.append((name, va, vs, data[ra:ra + rs]))
    return base, data[:header_size], sections


def f80_to_float(mantissa, exponent):
    sign = -1.0 if exponent & 0x8000 else 1.0
    e = exponent & 0x7fff
    if e == 0x7fff:
        return sign * math.inf if (mantissa << 1) & 0xffffffffffffffff == 0 else math.nan
    if mantissa == 0:
        return sign * 0.0
    return sign * math.ldexp(float(mantissa), e - 16383 - 63)


class X86Runner:
    def __init__(self, xbe_path, scratch_base, scratch_size, func_entries, max_insns=400000):
        self.base, header, self.sections = load_xbe(xbe_path)
        self.scratch_base = scratch_base
        self.scratch_size = scratch_size
        self.func_entries = func_entries
        self.max_insns = max_insns
        lo = self.base
        hi = max(va + vs for _, va, vs, _ in self.sections)
        self.image_lo = lo
        self.image_hi = (hi + 0xfff) & ~0xfff
        self.pristine = bytearray(self.image_hi - self.image_lo)
        self.pristine[0:len(header)] = header
        for name, va, vs, raw in self.sections:
            off = va - lo
            self.pristine[off:off + len(raw)] = raw[:vs]
        self.data_ranges = [(va, va + vs) for name, va, vs, _ in self.sections
                            if name in (".data", ".rdata")]
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.uc = uc
        uc.mem_map(self.image_lo, self.image_hi - self.image_lo, UC_PROT_ALL)
        uc.mem_write(self.image_lo, bytes(self.pristine))
        uc.mem_map(RET_ADDR, 0x1000, UC_PROT_ALL)
        uc.mem_write(RET_ADDR, b"\xf4" * 16)
        uc.mem_map(GDT_ADDR, 0x1000, UC_PROT_ALL)
        uc.mem_map(TIB_ADDR, 0x1000, UC_PROT_READ | UC_PROT_WRITE)
        uc.mem_map(STACK_LO, STACK_HI - STACK_LO, UC_PROT_READ | UC_PROT_WRITE)
        # scratch: data only, so a call through a random pointer faults (FETCH_PROT)
        uc.mem_map(scratch_base, scratch_size, UC_PROT_READ | UC_PROT_WRITE)

        def seg(base, limit, access, flags):
            return struct.pack("<HHBBBB", limit & 0xffff, base & 0xffff, (base >> 16) & 0xff,
                               access, ((limit >> 16) & 0xf) | (flags << 4), (base >> 24) & 0xff)
        gdt = (b"\0" * 8 + seg(0, 0xfffff, 0x9a, 0xc) + seg(0, 0xfffff, 0x92, 0xc) +
               seg(TIB_ADDR, 0xfff, 0x92, 0x4))
        uc.mem_write(GDT_ADDR, gdt)
        uc.reg_write(X.UC_X86_REG_GDTR, (0, GDT_ADDR, len(gdt) - 1, 0))
        uc.reg_write(X.UC_X86_REG_SS, 2 << 3)
        uc.reg_write(X.UC_X86_REG_DS, 2 << 3)
        uc.reg_write(X.UC_X86_REG_ES, 2 << 3)
        uc.reg_write(X.UC_X86_REG_FS, 3 << 3)
        self.tib = struct.pack("<IIII", 0xffffffff, STACK_HI, STACK_LO, 0) + b"\0" * 8
        self.tib += struct.pack("<I", TIB_ADDR)  # fs:[0x18] self
        uc.mem_write(TIB_ADDR, self.tib)
        # SSE on (Pentium III), x87 control word of the MSVC CRT: 53-bit precision, all masked
        uc.reg_write(X.UC_X86_REG_CR0, (uc.reg_read(X.UC_X86_REG_CR0) & ~4) | 2)
        uc.reg_write(X.UC_X86_REG_CR4, uc.reg_read(X.UC_X86_REG_CR4) | (1 << 9) | (1 << 10))
        uc.reg_write(X.UC_X86_REG_FPCW, 0x027f)
        uc.reg_write(X.UC_X86_REG_MXCSR, 0x1f80)
        self.context = uc.context_save()

        self.writes = []
        self.float_bytes = set()
        self.float_store = {}
        self.cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.calls = set()
        self.libc = set()
        self.stop_reason = None
        self.entry = None
        uc.hook_add(UC_HOOK_MEM_WRITE, self._on_write)
        uc.hook_add(UC_HOOK_BLOCK, self._on_block)

    def _on_write(self, uc, access, address, size, value, user):
        self.writes.append((address, size))
        pc = uc.reg_read(X.UC_X86_REG_EIP)
        is_float = self.float_store.get(pc)
        if is_float is None:
            insn = next(self.cs.disasm(bytes(uc.mem_read(pc, 16)), pc), None)
            m = insn.mnemonic if insn else ""
            is_float = (m.startswith("f") and m not in ("fnstsw", "fstsw", "fnstcw", "fstcw",
                                                        "fnstenv", "fstenv", "fnsave", "fsave")) \
                or m in ("movss", "movsd", "movlps", "movhps", "movaps", "movups")
            self.float_store[pc] = is_float
        if is_float:
            self.float_bytes.update(range(address, address + size))

    def _on_block(self, uc, address, size, user):
        if address in LIBC_DENY:
            self.stop_reason = "libc:" + LIBC_DENY[address]
            uc.emu_stop()
            return
        if address in NONDETERMINISTIC:
            self.stop_reason = "time:" + NONDETERMINISTIC[address]
            uc.emu_stop()
            return
        for lo, hi, name in FORBIDDEN:
            if lo <= address < hi:
                self.stop_reason = "%s:0x%08x" % (name, address)
                uc.emu_stop()
                return
        if address != self.entry and address in self.func_entries:
            self.calls.add(address)
        elif address < 0x00253360:
            for lo, hi in LIBC_REGIONS:
                if lo <= address < hi:
                    self.libc.add(address)

    def run(self, entry, image, ecx, edx, stack_words, poison=0, fpcw=None):
        """fpcw: another x87 control word (0x007F: 24-bit precision, as float code on ARM)."""
        uc = self.uc
        uc.context_restore(self.context)
        if fpcw is not None:
            uc.reg_write(X.UC_X86_REG_FPCW, fpcw)
        self.image = image
        uc.mem_write(self.scratch_base, image)
        uc.mem_write(TIB_ADDR, self.tib)
        esp = STACK_TOP
        frame = struct.pack("<I", RET_ADDR) + b"".join(struct.pack("<I", w & 0xffffffff)
                                                      for w in stack_words)
        uc.mem_write(esp, frame + b"\0" * 64)
        for reg, value in POISON[poison].items():
            uc.reg_write(reg, value)
        uc.reg_write(X.UC_X86_REG_ECX, ecx & 0xffffffff)
        uc.reg_write(X.UC_X86_REG_EDX, edx & 0xffffffff)
        uc.reg_write(X.UC_X86_REG_ESP, esp)
        self.writes = []
        self.float_bytes = set()
        self.calls = set()
        self.libc = set()
        self.stop_reason = None
        self.entry = entry
        status = "ok"
        try:
            uc.emu_start(entry, RET_ADDR, count=self.max_insns)
        except UcError as error:
            status = "fault:%s@0x%08x" % (error, uc.reg_read(X.UC_X86_REG_EIP))
            pc = uc.reg_read(X.UC_X86_REG_EIP)
            if 0x80000000 <= pc < self.scratch_base:
                status = "kernel:0x%08x" % pc
        pc = uc.reg_read(X.UC_X86_REG_EIP)
        if self.stop_reason:
            status = self.stop_reason
        elif status == "ok" and pc != RET_ADDR:
            status = "timeout"
        result = {"status": status, "calls": self.calls, "libc": self.libc}
        if status == "ok":
            fpsw = uc.reg_read(X.UC_X86_REG_FPSW)
            top = (fpsw >> 11) & 7
            result["eax"] = uc.reg_read(X.UC_X86_REG_EAX)
            result["edx"] = uc.reg_read(X.UC_X86_REG_EDX)
            result["fdepth"] = (8 - top) & 7
            result["st0"] = f80_to_float(*uc.reg_read(X.UC_X86_REG_FP0 + top)) if top else None
            result["popped"] = uc.reg_read(X.UC_X86_REG_ESP) - esp - 4
        result["changed"] = self._collect_and_restore(status == "ok")
        result["float_bytes"] = self.float_bytes
        return result

    def _collect_and_restore(self, collect):
        """Bytes changed in scratch and data (address -> new byte); data bytes restored."""
        uc = self.uc
        spans = []
        for address, size in sorted(set(self.writes)):
            if spans and address <= spans[-1][1]:
                spans[-1][1] = max(spans[-1][1], address + size)
            else:
                spans.append([address, address + size])
        changed = {}
        sb, se = self.scratch_base, self.scratch_base + self.scratch_size
        for lo, hi in spans:
            if self.image_lo <= lo and hi <= self.image_hi:
                now = uc.mem_read(lo, hi - lo)
                old = self.pristine[lo - self.image_lo:hi - self.image_lo]
                if now != old:
                    if collect:
                        for i in range(hi - lo):
                            if now[i] != old[i]:
                                changed[lo + i] = now[i]
                    uc.mem_write(lo, bytes(old))
            elif collect and sb <= lo and hi <= se:
                now = uc.mem_read(lo, hi - lo)
                old = self.image[lo - sb:hi - sb]
                for i in range(hi - lo):
                    if now[i] != old[i]:
                        changed[lo + i] = now[i]
        return changed

    def initial_byte(self, address):
        if self.image_lo <= address < self.image_hi:
            return self.pristine[address - self.image_lo]
        return None
