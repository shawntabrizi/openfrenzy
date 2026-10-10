// Analysis helper of the tankedup_b module. Run with -readOnly: nothing is saved.
// For the given functions and their direct callees (and the callees' callees), decompiles each
// function whose signature Ghidra guessed (SourceType.DEFAULT) and commits the decompiler's
// parameters (not the return type) to the in-memory program (what the "Decompiler Parameter ID"
// analyzer does). Then decompiles the given functions again, so that their calls show the
// ECX/EDX and stack arguments that the first export dropped. Writes OUTDIR/<addr>.c.
// Usage: -postScript RedecompileWithParams.java OUTDIR @addrfile
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.pcode.HighFunction;
import ghidra.program.model.pcode.HighFunctionDBUtil;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class RedecompileWithParams extends GhidraScript {
	private DecompInterface di;

	private void commit(Function f) {
		if (f == null || f.isThunk() || f.isExternal()) return;
		if (f.getSignatureSource() != SourceType.DEFAULT) return;
		// the empty debug-print function: callers pass format strings that it ignores
		long ep = f.getEntryPoint().getOffset();
		if (ep == 0x1d5950L || ep == 0x1de950L) return;
		try {
			DecompileResults r = di.decompileFunction(f, 120, monitor);
			HighFunction hf = r.getHighFunction();
			if (hf == null) return;
			HighFunctionDBUtil.commitParamsToDatabase(hf, true,
				HighFunctionDBUtil.ReturnCommitOption.NO_COMMIT, SourceType.ANALYSIS);
		} catch (Exception e) {
			println("commit failed " + f.getEntryPoint() + ": " + e);
		}
	}

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		File out = new File(args[0]);
		out.mkdirs();
		List<String> addrs = new ArrayList<>();
		for (int i = 1; i < args.length; i++) {
			if (args[i].startsWith("@")) addrs.addAll(Files.readAllLines(Paths.get(args[i].substring(1))));
			else addrs.add(args[i]);
		}
		di = new DecompInterface();
		di.openProgram(currentProgram);
		List<Function> mine = new ArrayList<>();
		for (String s : addrs) {
			s = s.trim();
			if (s.isEmpty()) continue;
			Address a = toAddr(Long.parseLong(s.replaceFirst("^0x", ""), 16));
			Function f = getFunctionAt(a);
			if (f == null) {
				disassemble(a);
				new CreateFunctionCmd(a).applyTo(currentProgram);
				f = getFunctionAt(a);
			}
			if (f != null) mine.add(f);
			else println("no function at " + s);
		}
		// callees of callees first, then callees, then the functions themselves
		LinkedHashSet<Function> level1 = new LinkedHashSet<>();
		LinkedHashSet<Function> level2 = new LinkedHashSet<>();
		for (Function f : mine) level1.addAll(f.getCalledFunctions(monitor));
		for (Function f : level1) level2.addAll(f.getCalledFunctions(monitor));
		for (Function f : level2) commit(f);
		for (Function f : level1) commit(f);
		for (Function f : mine) commit(f);
		// a second pass over the callers: their own parameters may depend on committed callees
		for (Function f : level1) commit(f);
		for (Function f : mine) {
			DecompileResults r = di.decompileFunction(f, 120, monitor);
			String c = r.getDecompiledFunction() == null ? ("/* " + r.getErrorMessage() + " */")
				: r.getDecompiledFunction().getC();
			String name = String.format("%08x.c", f.getEntryPoint().getOffset());
			Files.write(Paths.get(out.getPath(), name), c.getBytes());
		}
		println("redecompiled " + mine.size() + " functions into " + out);
	}
}
