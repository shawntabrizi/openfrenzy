// Exports every function: decompiled C to export/funcs/<addr>.c and one JSON
// line of metadata (section, size, callers, callees, strings) to export/functions.jsonl.
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

public class ExportAll extends GhidraScript {
	static String q(String s) {
		StringBuilder b = new StringBuilder("\"");
		for (char c : s.toCharArray()) {
			if (c == '"' || c == '\\') b.append('\\').append(c);
			else if (c < 0x20 || c > 0x7e) b.append(String.format("\\u%04x", (int) c));
			else b.append(c);
		}
		return b.append('"').toString();
	}

	@Override
	public void run() throws Exception {
		Path out = Paths.get(getScriptArgs()[0]);
		Files.createDirectories(out.resolve("funcs"));
		DecompInterface di = new DecompInterface();
		di.openProgram(currentProgram);
		Listing listing = currentProgram.getListing();
		try (PrintWriter meta = new PrintWriter(Files.newBufferedWriter(out.resolve("functions.jsonl")))) {
			for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
				if (monitor.isCancelled()) break;
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
				meta.println("{\"addr\":" + q(addr) + ",\"name\":" + q(f.getName()) + ",\"section\":" + q(blk == null ? "?" : blk.getName())
					+ ",\"size\":" + f.getBody().getNumAddresses() + ",\"callees\":" + toJson(callees) + ",\"callers\":" + toJson(callers)
					+ ",\"strings\":" + toJson(strs) + "}");
			}
		}
	}

	static String toJson(List<String> l) {
		StringJoiner j = new StringJoiner(",", "[", "]");
		for (String s : l) j.add(q(s));
		return j.toString();
	}
}
