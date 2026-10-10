// Analysis helper of the rowing_a module (not part of the build).
// Decompiles Rowing functions with better call signatures, in memory only (run with
// -readOnly: nothing is saved), and writes <out>/<addr>.c for each listed entry.
//
// Why: in the first export many x86 `thiscall` callees had no `this` (ECX) parameter and
// many callee-clean functions had too few stack parameters, so Ghidra's C dropped call
// arguments (CParticleSystem_Init(0, 800) instead of (ps, 0, 800)) and showed stack
// parameters as `unaff_retaddr`. Before decompiling, this script fixes the signature of each
// listed function and of every function they call directly:
//   - `this` in ECX: the entry code reads ECX before it writes it (or calls/returns);
//   - stack parameters: `ret N` gives N / 4 (callee-clean functions).
// Missing functions (no function at a listed entry) are created first.
//
// Usage: -postScript RowingExport.java <out-dir> @<file with hex entry addresses> | addr...
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.*;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.*;
import java.util.*;

public class RowingExport extends GhidraScript {
	Register ecx;

	boolean isEcx(Object o) {
		return o instanceof Register && ((Register) o).getBaseRegister().equals(ecx.getBaseRegister());
	}

	// true when the code at the entry reads ECX before writing it
	boolean readsEcx(Function f) {
		Listing listing = currentProgram.getListing();
		Instruction ins = listing.getInstructionAt(f.getEntryPoint());
		for (int n = 0; ins != null && n < 60; n++) {
			String m = ins.getMnemonicString().toUpperCase();
			boolean selfClear = (m.equals("XOR") || m.equals("SUB")) && ins.getNumOperands() == 2
				&& ins.getOpObjects(0).length == 1 && ins.getOpObjects(1).length == 1
				&& isEcx(ins.getOpObjects(0)[0]) && isEcx(ins.getOpObjects(1)[0]);
			boolean pushOnly = m.equals("PUSH") && ins.getOpObjects(0).length == 1 && isEcx(ins.getOpObjects(0)[0]);
			if (!selfClear && !pushOnly) {
				for (Object o : ins.getInputObjects()) if (isEcx(o)) return true;
			}
			for (Object o : ins.getResultObjects()) if (isEcx(o)) return false;
			if (m.startsWith("CALL") || m.startsWith("RET")) return false;
			if (m.equals("JMP")) {
				Address[] fl = ins.getFlows();
				if (fl.length != 1) return false;
				ins = listing.getInstructionAt(fl[0]);
				continue;
			}
			ins = ins.getNext();
		}
		return false;
	}

	// N / 4 of the first `ret N` in the body; 0 for a plain `ret`; -1 when there is none
	int stackParams(Function f) {
		Listing listing = currentProgram.getListing();
		int plain = -1;
		for (Address a : f.getBody().getAddresses(true)) {
			Instruction ins = listing.getInstructionAt(a);
			if (ins == null || !ins.getMnemonicString().toUpperCase().startsWith("RET")) continue;
			if (ins.getNumOperands() > 0 && ins.getScalar(0) != null) return (int) (ins.getScalar(0).getValue() / 4);
			plain = 0;
		}
		return plain;
	}

	void fix(Function f) throws Exception {
		if (f == null || f.isThunk() || f.isExternal()) return;
		boolean thisEcx = readsEcx(f);
		int n = stackParams(f);
		String cc = f.getCallingConventionName();
		int have = f.getParameterCount();
		boolean haveThis = "__thiscall".equals(cc) || "__fastcall".equals(cc);
		int haveStack = haveThis ? Math.max(0, have - ("__fastcall".equals(cc) ? Math.min(have, 2) : 1)) : have;
		if (thisEcx) {
			int want = n >= 0 ? n : haveStack;
			if ("__thiscall".equals(cc) && haveStack == want) return;
			List<Variable> ps = new ArrayList<>();
			Parameter[] old = f.getParameters();
			int skip = haveThis ? 1 : 0;
			for (int i = 0; i < want; i++) {
				DataType dt = Undefined4DataType.dataType;
				if (skip + i < old.length && old[skip + i].getLength() == 4) dt = old[skip + i].getDataType();
				ps.add(new ParameterImpl("param_" + (i + 2), dt, currentProgram));
			}
			f.setCallingConvention("__thiscall");
			f.replaceParameters(ps, Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.USER_DEFINED);
			println(String.format("fix %s: __thiscall + %d", f.getEntryPoint(), want));
		} else if (n > 0 && !haveThis && have != n) {
			List<Variable> ps = new ArrayList<>();
			Parameter[] old = f.getParameters();
			for (int i = 0; i < n; i++) {
				DataType dt = Undefined4DataType.dataType;
				if (i < old.length && old[i].getLength() == 4) dt = old[i].getDataType();
				ps.add(new ParameterImpl("param_" + (i + 1), dt, currentProgram));
			}
			f.setCallingConvention("__stdcall");
			f.replaceParameters(ps, Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.USER_DEFINED);
			println(String.format("fix %s: __stdcall %d", f.getEntryPoint(), n));
		}
	}

	@Override
	public void run() throws Exception {
		ecx = currentProgram.getRegister("ECX");
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
		for (long e : entries) {
			Address a = toAddr(e);
			if (getFunctionAt(a) == null) {
				disassemble(a);
				new CreateFunctionCmd(a).applyTo(currentProgram);
			}
		}
		Set<Function> todo = new LinkedHashSet<>();
		for (long e : entries) {
			Function f = getFunctionAt(toAddr(e));
			if (f == null) continue;
			todo.add(f);
			for (Function c : f.getCalledFunctions(monitor)) todo.add(c);
		}
		for (Function f : todo) {
			try { fix(f); } catch (Exception ex) { println("fix failed " + f.getEntryPoint() + ": " + ex); }
		}
		DecompInterface di = new DecompInterface();
		di.openProgram(currentProgram);
		for (long e : entries) {
			Function f = getFunctionAt(toAddr(e));
			String name = String.format("%08x", e);
			if (f == null) { println("no function at " + name); continue; }
			DecompileResults r = di.decompileFunction(f, 120, monitor);
			String c = r.decompileCompleted() ? r.getDecompiledFunction().getC() : "/* decompile failed: " + r.getErrorMessage() + " */\n";
			Files.writeString(out.resolve(name + ".c"), c);
		}
		println("exported " + entries.size());
	}
}
