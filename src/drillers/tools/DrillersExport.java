// Analysis helper of the drillers module (not part of the build).
// Rebuilds the function boundaries of the Drillers code range in memory (run with -readOnly:
// nothing is saved) and writes the decompiled C of each true function to <out>/<addr>.c.
//
// Why: Ghidra's first analysis split the Drillers code badly. The CGameMode_Drillers vtable
// slots are thunks ("mov ecx, 0x2ff9f0; jmp body") whose bodies Ghidra merged into the thunk,
// several table entries are continuation chunks of the previous function (0x00110000,
// 0x00110840, 0x00115443, 0x00120000, 0x001214f0, 0x001246e0), and about 60 functions that are
// only reached by direct calls or pushed callbacks have no function at all.
//
// Usage: -postScript DrillersExport.java <out-dir> @<file with hex entry addresses>
//        (every function whose entry lies in [lo, hi) of the first two addresses is removed
//        first; then a function is created at each listed entry, in address order)
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.util.*;

public class DrillersExport extends GhidraScript {
	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		Path out = Paths.get(args[0]);
		Files.createDirectories(out);
		List<Long> entries = new ArrayList<>();
		for (int i = 1; i < args.length; i++) {
			if (args[i].startsWith("@")) {
				for (String s : Files.readString(Paths.get(args[i].substring(1))).trim().split("\\s+"))
					entries.add(Long.parseLong(s.replaceFirst("^0x", ""), 16));
			} else {
				entries.add(Long.parseLong(args[i].replaceFirst("^0x", ""), 16));
			}
		}
		Collections.sort(entries);
		long lo = 0x0010f830L, hi = 0x00125070L;
		FunctionManager fm = currentProgram.getFunctionManager();
		List<Function> old = new ArrayList<>();
		for (Function f : fm.getFunctions(toAddr(lo), true)) {
			if (f.getEntryPoint().getOffset() >= hi) break;
			old.add(f);
		}
		for (Function f : old) fm.removeFunction(f.getEntryPoint());
		println("removed " + old.size() + " functions");
		for (long e : entries) {
			Address a = toAddr(e);
			disassemble(a);
		}
		for (long e : entries) {
			Address a = toAddr(e);
			if (getFunctionAt(a) == null) new CreateFunctionCmd(a).applyTo(currentProgram);
		}
		// a second pass: bodies are recomputed now that every entry exists
		for (long e : entries) {
			Function f = getFunctionAt(toAddr(e));
			if (f != null) CreateFunctionCmd.fixupFunctionBody(currentProgram, f, monitor);
		}
		DecompInterface di = new DecompInterface();
		di.openProgram(currentProgram);
		StringBuilder globals = new StringBuilder();
		for (long e : entries) {
			Function f = getFunctionAt(toAddr(e));
			String name = String.format("%08x", e);
			if (f == null) { println("no function at " + name); continue; }
			DecompileResults r = di.decompileFunction(f, 120, monitor);
			String c = r.decompileCompleted() ? r.getDecompiledFunction().getC() : "/* decompile failed: " + r.getErrorMessage() + " */\n";
			// the type the decompiler gave each global it names in this function
			if (r.decompileCompleted() && r.getHighFunction() != null) {
				java.util.Iterator<ghidra.program.model.pcode.HighSymbol> it =
					r.getHighFunction().getGlobalSymbolMap().getSymbols();
				while (it.hasNext()) {
					ghidra.program.model.pcode.HighSymbol hs = it.next();
					ghidra.program.model.data.DataType dt = hs.getDataType();
					String addr = hs.getStorage() != null && hs.getStorage().getMinAddress() != null
						? hs.getStorage().getMinAddress().toString() : "?";
					globals.append(name).append('\t').append(hs.getName()).append('\t').append(addr)
						.append('\t').append(dt == null ? "?" : dt.getDisplayName())
						.append('\t').append(dt == null ? -1 : dt.getLength()).append('\n');
				}
			}
			StringBuilder hdr = new StringBuilder();
			hdr.append("/* body:");
			for (ghidra.program.model.address.AddressRange rg : f.getBody()) hdr.append(" ").append(rg.getMinAddress()).append("-").append(rg.getMaxAddress());
			hdr.append(" */\n");
			Files.writeString(out.resolve(name + ".c"), hdr + c);
		}
		Files.writeString(out.resolve("globals.tsv"), globals.toString());
		println("exported " + entries.size());
	}
}
