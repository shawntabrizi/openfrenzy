// Prints a function census of the current program, per memory block.
// Run headless: analyzeHeadless ... -postScript FunctionReport.java
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryBlock;
import java.util.*;

public class FunctionReport extends GhidraScript {
	@Override
	public void run() throws Exception {
		Map<String, long[]> stats = new TreeMap<>(); // block -> {count, named, bytes}
		for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
			MemoryBlock b = currentProgram.getMemory().getBlock(f.getEntryPoint());
			String name = b == null ? "?" : b.getName();
			long[] s = stats.computeIfAbsent(name, k -> new long[3]);
			s[0]++;
			if (!f.getName().startsWith("FUN_")) s[1]++;
			s[2] += f.getBody().getNumAddresses();
		}
		println(String.format("%-12s %8s %8s %12s", "block", "funcs", "named", "bytes"));
		for (Map.Entry<String, long[]> e : stats.entrySet()) {
			long[] s = e.getValue();
			println(String.format("%-12s %8d %8d %12d", e.getKey(), s[0], s[1], s[2]));
		}
	}
}
