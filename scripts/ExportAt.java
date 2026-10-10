// Exports functions that Ghidra's analysis missed, in the format of ExportAll.java.
// For each address (hex, no 0x): creates a function when Ghidra has none there (in memory only:
// run with -readOnly, nothing is saved), writes <out>/funcs/<addr>.c and appends one JSON line
// to <out>/functions_extra.jsonl. scripts/map_sources.py reads functions_extra.jsonl too.
// Usage: -postScript ExportAt.java <out-dir> 1f2a10 1f2a20 ...
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.StringDataInstance;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ExportAt extends GhidraScript {
	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		Path out = Paths.get(args[0]);
		Files.createDirectories(out.resolve("funcs"));
		DecompInterface di = new DecompInterface();
		di.openProgram(currentProgram);
		Listing listing = currentProgram.getListing();
		List<Function> done = new ArrayList<>();
		Deque<Address> work = new ArrayDeque<>();
		Set<Address> queued = new HashSet<>();
		List<String> addrArgs = new ArrayList<>();
		for (int i = 1; i < args.length; i++) {
			if (args[i].startsWith("@")) {   // @file: whitespace-separated addresses
				for (String s : Files.readString(Paths.get(args[i].substring(1))).trim().split("\\s+"))
					addrArgs.add(s);
			} else {
				addrArgs.add(args[i]);
			}
		}
		for (String s : addrArgs) {
			Address a = toAddr(Long.parseLong(s.replaceFirst("^0x", ""), 16));
			if (queued.add(a)) work.add(a);
		}
		// Each new function's direct call targets that have no function become functions too.
		while (!work.isEmpty()) {
			Address a = work.poll();
			Function f = getFunctionAt(a);
			boolean created = false;
			if (f == null) {
				disassemble(a);
				new CreateFunctionCmd(a).applyTo(currentProgram);
				f = getFunctionAt(a);
				created = true;
			}
			if (f == null) { println("no function at " + a); continue; }
			if (!created && !queued.contains(a)) continue;
			done.add(f);
			for (Address ia : f.getBody().getAddresses(true)) {
				Instruction ins = listing.getInstructionAt(ia);
				if (ins == null || !ins.getFlowType().isCall()) continue;
				for (Address t : ins.getFlows()) {
					if (getFunctionAt(t) == null && queued.add(t)) work.add(t);
				}
			}
		}
		try (PrintWriter meta = new PrintWriter(new FileWriter(out.resolve("functions_extra.jsonl").toFile(), true))) {
			for (Function f : done) {
				MemoryBlock blk = currentProgram.getMemory().getBlock(f.getEntryPoint());
				String addr = f.getEntryPoint().toString();
				List<String> callees = new ArrayList<>(), callers = new ArrayList<>(), strs = new ArrayList<>();
				for (Function c : f.getCalledFunctions(monitor)) callees.add(c.getEntryPoint().toString());
				for (Function c : f.getCallingFunctions(monitor)) callers.add(c.getEntryPoint().toString());
				for (Address a : f.getBody().getAddresses(true)) {
					for (Reference r : getReferencesFrom(a)) {
						Data d = listing.getDataAt(r.getToAddress());
						if (d == null) continue;
						StringDataInstance s = StringDataInstance.getStringDataInstance(d);
						if (s != StringDataInstance.NULL_INSTANCE && s.getStringValue() != null) strs.add(s.getStringValue());
					}
				}
				DecompileResults res = di.decompileFunction(f, 60, monitor);
				String c = res.getDecompiledFunction() == null ? "/* decompile failed: " + res.getErrorMessage() + " */" : res.getDecompiledFunction().getC();
				Files.writeString(out.resolve("funcs/" + addr + ".c"), c);
				meta.println("{\"addr\":" + q(addr) + ",\"name\":" + q(f.getName()) + ",\"section\":"
					+ q(blk == null ? "?" : blk.getName()) + ",\"size\":" + f.getBody().getNumAddresses()
					+ ",\"callees\":" + toJson(callees) + ",\"callers\":" + toJson(callers)
					+ ",\"strings\":" + toJson(strs) + "}");
				println("exported " + addr + " " + f.getName());
			}
		}
	}

	static String q(String s) {
		StringBuilder b = new StringBuilder("\"");
		for (char c : s.toCharArray()) {
			if (c == '"' || c == '\\') b.append('\\').append(c);
			else if (c < 0x20 || c > 0x7e) b.append(String.format("\\u%04x", (int) c));
			else b.append(c);
		}
		return b.append('"').toString();
	}

	static String toJson(List<String> l) {
		StringJoiner j = new StringJoiner(",", "[", "]");
		for (String s : l) j.add(q(s));
		return j.toString();
	}
}
