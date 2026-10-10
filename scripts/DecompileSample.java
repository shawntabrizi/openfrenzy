// Decompiles the first function that references a string containing the
// script argument (default "CBCar_Game.cpp") and prints the C output.
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.StringDataInstance;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.Reference;

public class DecompileSample extends GhidraScript {
	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		String needle = args.length > 0 ? args[0] : "CBCar_Game.cpp";
		DecompInterface di = new DecompInterface();
		di.openProgram(currentProgram);
		for (Data d : currentProgram.getListing().getDefinedData(true)) {
			StringDataInstance s = StringDataInstance.getStringDataInstance(d);
			if (s == StringDataInstance.NULL_INSTANCE || s.getStringValue() == null) continue;
			if (!s.getStringValue().contains(needle)) continue;
			for (Reference r : getReferencesTo(d.getAddress())) {
				Function f = getFunctionContaining(r.getFromAddress());
				if (f == null) continue;
				DecompileResults res = di.decompileFunction(f, 60, monitor);
				println("=== " + f.getName() + " @ " + f.getEntryPoint() + " refs " + s.getStringValue());
				println(res.getDecompiledFunction() == null ? res.getErrorMessage() : res.getDecompiledFunction().getC());
				return;
			}
		}
		println("no function references " + needle);
	}
}
