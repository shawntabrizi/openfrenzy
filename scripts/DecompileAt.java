// Decompiles the functions at the given addresses (hex, no 0x) and prints C to stdout.
// Creates a function first when Ghidra has none there. Run with -readOnly: nothing is saved.
// Usage: -postScript DecompileAt.java 30f10 87400 ...
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class DecompileAt extends GhidraScript {
	@Override
	public void run() throws Exception {
		DecompInterface di = new DecompInterface();
		di.openProgram(currentProgram);
		for (String arg : getScriptArgs()) {
			Address a = toAddr(Long.parseLong(arg.replaceFirst("^0x", ""), 16));
			Function f = getFunctionAt(a);
			if (f == null) {
				disassemble(a);
				new CreateFunctionCmd(a).applyTo(currentProgram);
				f = getFunctionAt(a);
			}
			println("==== " + arg);
			if (f == null) { println("no function"); continue; }
			DecompileResults r = di.decompileFunction(f, 120, monitor);
			println(r.getDecompiledFunction() == null ? r.getErrorMessage() : r.getDecompiledFunction().getC());
		}
	}
}
