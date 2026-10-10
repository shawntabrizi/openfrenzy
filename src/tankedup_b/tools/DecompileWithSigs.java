// Analysis helper of the tankedup_b module. Run with -readOnly: nothing is saved.
// Applies the prototypes of SIGFILE (lines "addr|callingconvention|C prototype", '#'
// comments) to the in-memory program, then decompiles the given functions into OUTDIR.
// Usage: -postScript DecompileWithSigs.java SIGFILE OUTDIR ADDR... (or @addrfile)
import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.app.util.cparser.C.CParserUtils;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.*;
import java.util.*;

public class DecompileWithSigs extends GhidraScript {
	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		List<String> lines = Files.readAllLines(Paths.get(args[0]));
		Path out = Paths.get(args[1]);
		Files.createDirectories(out);
		DataTypeManager dtm = currentProgram.getDataTypeManager();
		for (String line : lines) {
			line = line.trim();
			if (line.isEmpty() || line.startsWith("#")) continue;
			String[] p = line.split("\\|", 3);
			Function f = getFunctionAt(toAddr(Long.parseLong(p[0].trim(), 16)));
			if (f == null) { println("no function " + p[0]); continue; }
			try {
				FunctionDefinitionDataType sig = CParserUtils.parseSignature(
					(ghidra.app.services.DataTypeManagerService) null, currentProgram, p[2].trim());
				if (sig == null) { println("bad signature " + line); continue; }
				new ApplyFunctionSignatureCmd(f.getEntryPoint(), sig, SourceType.USER_DEFINED)
					.applyTo(currentProgram);
				f.setCallingConvention(p[1].trim());
			} catch (Exception e) {
				println("failed " + line + ": " + e);
			}
		}
		DecompInterface di = new DecompInterface();
		di.openProgram(currentProgram);
		List<String> addrs = new ArrayList<>();
		for (int i = 2; i < args.length; i++) {
			if (args[i].startsWith("@")) addrs.addAll(Files.readAllLines(Paths.get(args[i].substring(1))));
			else addrs.add(args[i]);
		}
		for (String s : addrs) {
			s = s.trim();
			if (s.isEmpty()) continue;
			Function f = getFunctionAt(toAddr(Long.parseLong(s, 16)));
			if (f == null) { println("no function " + s); continue; }
			DecompileResults r = di.decompileFunction(f, 180, monitor);
			String c = r.getDecompiledFunction() == null ? ("/* " + r.getErrorMessage() + " */")
				: r.getDecompiledFunction().getC();
			Files.write(out.resolve(String.format("%08x.c", f.getEntryPoint().getOffset())), c.getBytes());
		}
		println("decompiled " + addrs.size());
	}
}
